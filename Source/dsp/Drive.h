#pragma once

#include "DspUtils.h"

namespace octocid
{
    //==========================================================================
    // Section DRIVE (tourne à la fréquence suréchantillonnée).
    //
    //  - Overdrive  : saturation tanh asymétrique (harmoniques paires, grain
    //                 chaud), filtre de tonalité en sortie.
    //  - Distortion : modèle de circuit d'une pédale « rat » classique, à partir
    //                 des valeurs de composants publiées :
    //       * étage ampli op (LM308) non inverseur, gain 1 + Rdist / Zg avec
    //         Zg = (47 Ω + 2,2 µF) // (560 Ω + 4,7 µF), 100 pF sur Rdist (pot
    //         100 k log) : gain max vers 500 Hz, grave moins amplifié ;
    //       * produit gain-bande de l'ampli op (~330 kHz) -> le gain s'effondre
    //         dans l'aigu, d'où le grain médium ; vitesse de balayage 0,3 V/µs ;
    //         rails ±4,5 V ;
    //       * condensateur de liaison 4,7 µF + 1 kΩ (passe-haut ~34 Hz) ;
    //       * écrêtage dur par deux diodes silicium tête-bêche (résolu par Newton) ;
    //       * filtre de tonalité passif 1,5 k + pot 100 k et 3,3 nF (32 kHz..475 Hz).
    //    La fonction de transfert linéaire (ordre 4) est discrétisée par la
    //    transformée bilinéaire, en double précision.
    //
    // Aucune allocation ; coefficients recalculés tous les 16 échantillons
    // quand le Drive bouge.
    //==========================================================================
    class Drive
    {
    public:
        enum Type { Off = 0, Overdrive = 1, Distortion = 2 };

        void prepare (double sampleRate) noexcept
        {
            fs = sampleRate;
            smooth = (double) onePoleCoef (0.01f, sampleRate);
            fadeStep = 1.0 / (0.006 * sampleRate);                  // fondu 6 ms au changement de type
            slewMax = 0.3e6 / sampleRate;                            // 0,3 V/µs
            const double xc = std::exp (-2.0 * 3.141592653589793 * 34.0 / sampleRate);
            couplingA = xc; couplingB = 0.5 * (1.0 + xc);
            reset();
        }

        void reset() noexcept
        {
            for (auto& v : z) v = 0.0;
            cpX = cpY = 0.0; toneS = 0.0; slewY = 0.0;
            drv = targetDrive; tone = targetTone; mix = targetMix;
            current = target; fade = (current == Off) ? 0.0 : 1.0;
            ctrlCount = 0; lastCoefDrive = -1.0;
            updateRatCoefs();
            updateTone();
            makeupGain = makeup();
        }

        void setParams (int type, float drive01, float tone01, float mix01) noexcept
        {
            target = (Type) std::clamp (type, 0, 2);
            targetDrive = std::clamp ((double) drive01, 0.0, 1.0);
            targetTone  = std::clamp ((double) tone01, 0.0, 1.0);
            targetMix   = std::clamp ((double) mix01, 0.0, 1.0);
        }

        float process (float in) noexcept
        {
            // Changement de type : on referme le wet, on change, on rouvre
            if (target != current)
            {
                fade -= fadeStep;
                if (fade <= 0.0)
                {
                    fade = 0.0;
                    current = target;
                    updateTone();
                    makeupGain = makeup();
                    for (auto& v : z) v = 0.0;
                    cpX = cpY = toneS = slewY = 0.0;
                }
            }
            else if (current != Off && fade < 1.0)
                fade = std::min (1.0, fade + fadeStep);

            if (current == Off && fade <= 0.0)
                return in;

            drv  += (targetDrive - drv)  * smooth;
            tone += (targetTone  - tone) * smooth;
            mix  += (targetMix   - mix)  * smooth;

            if (++ctrlCount >= 16)
            {
                ctrlCount = 0;
                if (std::abs (drv - lastCoefDrive) > 1.0e-4) updateRatCoefs();
                updateTone();
                makeupGain = makeup();
            }

            const double x = in;
            double wet = current == Distortion ? processRat (x) : processOverdrive (x);

            // Filtre de tonalité (passe-bas 1 pôle TPT)
            const double v = (wet - toneS) * toneG;
            const double lp = v + toneS;
            toneS = lp + v;
            wet = lp;

            const double w = mix * fade;
            return (float) (x + w * (wet * makeupGain - x));
        }

    private:
        //======================================================================
        double processOverdrive (double x) noexcept
        {
            const double g = 1.0 + 60.0 * drv * drv;                 // jusqu'à ~36 dB
            constexpr double bias = 0.3;
            return std::tanh (g * 2.0 * x + bias) - std::tanh (bias);
        }

        double processRat (double x) noexcept
        {
            const double vin = x * 1.5;                              // niveau « guitare »

            // Étage ampli op linéaire (DF2T ordre 4)
            double y = b[0] * vin + z[0];
            z[0] = b[1] * vin - a[1] * y + z[1];
            z[1] = b[2] * vin - a[2] * y + z[2];
            z[2] = b[3] * vin - a[3] * y + z[3];
            z[3] = b[4] * vin - a[4] * y;

            // Vitesse de balayage puis rails d'alimentation
            const double dy = std::clamp (y - slewY, -slewMax, slewMax);
            slewY += dy;
            y = 4.2 * std::tanh (slewY * (1.0 / 4.2));

            // Condensateur de liaison (passe-haut ~34 Hz)
            const double hp = couplingB * (y - cpX) + couplingA * cpY;
            cpX = y; cpY = hp;

            // Diodes : (vin - v) / R = 2 Is sinh(v / nVt), résolu par Newton.
            // Départ sur une borne supérieure exacte de la solution (min de |vin| et de
            // nVt asinh(|vin| / 2RIs)) : f est convexe, Newton converge alors sans osciller.
            constexpr double R = 1000.0, Is = 2.52e-9, nVt = 1.752 * 0.02585;
            constexpr double k1 = 2.0 * R * Is, k2 = k1 / nVt;
            const double sgn = hp < 0.0 ? -1.0 : 1.0, ax = std::abs (hp);
            double v = std::min (ax, nVt * std::asinh (ax / k1));
            for (int it = 0; it < 4; ++it)
            {
                const double e = std::exp (v / nVt);
                const double f = v - ax + k1 * 0.5 * (e - 1.0 / e);
                v -= f / (1.0 + k2 * 0.5 * (e + 1.0 / e));
            }
            return sgn * v * 1.6;                                    // ~±1 en sortie d'écrêtage
        }

        double makeup() const noexcept
        {
            // Compensation calibrée au banc de test (ligne acid typique) : le son
            // distordu garde à peu près le niveau RMS du son sec, quel que soit le Drive.
            static constexpr double odDb[5]  = { 0.0, 9.5, 12.5, 12.7, 12.4 };
            static constexpr double ratDb[5] = { -0.7, 9.3, 12.7, 14.2, 14.7 };
            const double* t = current == Distortion ? ratDb : odDb;
            const double pos = drv * 4.0;
            const int i = std::min (3, (int) pos);
            const double db = t[i] + (pos - i) * (t[i + 1] - t[i]);
            const double base = current == Distortion ? 0.42 : 0.55 / (1.0 + 0.6 * drv);
            return base * std::pow (10.0, -db / 20.0);
        }

        //======================================================================
        void updateRatCoefs() noexcept
        {
            lastCoefDrive = drv;
            // Pot 100 k logarithmique
            const double Rd = 100.0e3 * (std::exp (4.6 * drv) - 1.0) / (std::exp (4.6) - 1.0);

            constexpr double C1 = 2.2e-6, C2 = 4.7e-6, Cf = 100.0e-12;
            constexpr double t1 = 47.0 * C1, t2 = 560.0 * C2;
            constexpr double wt = 2.0 * 3.141592653589793 * 330.0e3;
            const double tf = Rd * Cf;

            // D(s) = (1 + s t1)(1 + s t2)(1 + s tf), puissances croissantes
            const double d0 = 1.0;
            const double d1 = t1 + t2 + tf;
            const double d2 = t1 * t2 + t1 * tf + t2 * tf;
            const double d3 = t1 * t2 * tf;

            // N = D + Rd s ((C1 + C2) + s (C1 t2 + C2 t1))
            const double n[5] = { d0, d1 + Rd * (C1 + C2), d2 + Rd * (C1 * t2 + C2 * t1), d3, 0.0 };
            // Boucle fermée avec le produit gain-bande : Den = D + N s / wt
            const double dn[5] = { d0, d1 + n[0] / wt, d2 + n[1] / wt, d3 + n[2] / wt, n[3] / wt };

            // Transformée bilinéaire : s = K (1 - z^-1) / (1 + z^-1)
            static constexpr double basis[5][5] = {
                { 1,  4,  6,  4,  1 },     // (1+z)^4
                { 1,  2,  0, -2, -1 },     // (1-z)(1+z)^3
                { 1,  0, -2,  0,  1 },     // (1-z)^2(1+z)^2
                { 1, -2,  0,  2, -1 },     // (1-z)^3(1+z)
                { 1, -4,  6, -4,  1 } };   // (1-z)^4
            const double K = 2.0 * fs;
            double bb[5] {}, aa[5] {};
            double Ki = 1.0;
            for (int i = 0; i < 5; ++i)
            {
                for (int j = 0; j < 5; ++j)
                {
                    bb[j] += n[i]  * Ki * basis[i][j];
                    aa[j] += dn[i] * Ki * basis[i][j];
                }
                Ki *= K;
            }
            for (int j = 0; j < 5; ++j) { b[j] = bb[j] / aa[0]; a[j] = aa[j] / aa[0]; }
        }

        void updateTone() noexcept
        {
            double fc;
            if (current == Distortion)
            {
                // 1,5 k + pot 100 k (log, tourné à fond = sombre), 3,3 nF
                const double t = 1.0 - tone;
                const double Rt = 100.0e3 * (std::exp (4.6 * t) - 1.0) / (std::exp (4.6) - 1.0);
                fc = 1.0 / (2.0 * 3.141592653589793 * (1500.0 + Rt) * 3.3e-9);
            }
            else
            {
                fc = 500.0 * std::pow (36.0, tone);                  // 500 Hz .. 18 kHz
            }
            fc = std::min (fc, 0.45 * fs);
            const double g = std::tan (3.141592653589793 * fc / fs);
            toneG = g / (1.0 + g);
        }

        double fs = 88200.0;
        Type target = Off, current = Off;
        double targetDrive = 0.5, targetTone = 0.5, targetMix = 1.0;
        double drv = 0.5, tone = 0.5, mix = 1.0, smooth = 0.01;
        double fade = 0.0, fadeStep = 0.01;
        int ctrlCount = 0;
        double lastCoefDrive = -1.0;

        double b[5] {}, a[5] {}, z[4] {};
        double slewMax = 3.0, slewY = 0.0;
        double couplingA = 0.0, couplingB = 1.0, cpX = 0.0, cpY = 0.0;
        double toneG = 0.5, toneS = 0.0;
        double makeupGain = 1.0;
    };
}

#pragma once

#include "DspUtils.h"
#include <cmath>
#include <vector>

//==============================================================================
// Effets « percutant / transcendant » (code maison, aucune allocation en lecture)
//
//  IMPACT  : façonneur de transitoires (suiveur rapide / suiveur lent sur un signal
//            de détection filtré passe-haut) : l'attaque du filtre claque, le grave
//            ne fait pas « respirer » l'effet.
//  SMASH   : compression 3 bandes vers le haut ET vers le bas (esprit « OTT »),
//            filtres de Linkwitz-Riley compensés en phase : à 0 % de réduction,
//            les trois bandes se ré-additionnent sans creux ni bosse.
//  JET     : flanger (interpolation cubique, feedback saturé) ou phaser 6 étages
//            (notches étagées), stéréo à 90°, synchronisé au tempo.
//  PUMP    : pompe rythmique (ducking synchronisé, effet sidechain).
//  WIDTH   : élargisseur stéréo compatible mono (le grave reste au centre).
//
// Tous les réglages sont lissés à l'échantillon (~20 ms) : pas de « zipper » en
// automation, et un effet qu'on coupe s'éteint en douceur avant d'être contourné.
//==============================================================================
namespace octocid
{
    //==========================================================================
    // Réglage lissé (cible donnée par bloc, valeur lissée par échantillon)
    //==========================================================================
    struct SmoothedAmount
    {
        float target = 0.0f, value = 0.0f, coef = 0.001f;
        void prepare (double fs) noexcept { coef = onePoleCoef (0.020f, fs); value = target; }
        void set (float t) noexcept { target = t; }
        float next() noexcept { value += (target - value) * coef; return value; }
        bool active() const noexcept { return target > 1.0e-3f || value > 1.0e-4f; }
    };

    //==========================================================================
    // SVF TPT (Zavalishin), 2e ordre : passe-bas, passe-haut, passe-tout
    //==========================================================================
    struct Svf2
    {
        float g = 0.0f, k = 1.41421356f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f, ic1 = 0.0f, ic2 = 0.0f;
        void set (float hz, double fs, float q = 0.70710678f) noexcept
        {
            k = 1.0f / q;
            g = std::tan (3.14159265f * std::min (hz, 0.45f * (float) fs) / (float) fs);
            a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
        }
        void reset() noexcept { ic1 = ic2 = 0.0f; }
        // renvoie le passe-bas ; hp reçoit le passe-haut, bp le passe-bande
        float process (float x, float& hp, float& bp) noexcept
        {
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
            hp = x - k * v1 - v2;
            bp = v1;
            return v2;
        }
        float lowpass (float x) noexcept  { float h, b; return process (x, h, b); }
        float highpass (float x) noexcept { float h, b; process (x, h, b); return h; }
        float allpass (float x) noexcept  { float h, b; const float l = process (x, h, b); return l + h - k * b; }
    };

    //==========================================================================
    class TransientShaper
    {
    public:
        void prepare (double fs) noexcept
        {
            scHp1.set (120.0f, fs); scHp2.set (120.0f, fs);
            // Énergie lissée sur 6 ms (sinon chaque période d'une note grave ressemble à une attaque),
            // puis deux suiveurs à même relâchement mais attaques différentes (1 ms / 25 ms) :
            // l'écart n'existe qu'au début de la note.
            rmsC  = onePoleCoef (0.006f, fs);
            fastA = onePoleCoef (0.001f, fs); fastR = onePoleCoef (0.200f, fs);
            slowA = onePoleCoef (0.025f, fs); slowR = onePoleCoef (0.200f, fs);
            gSmooth = onePoleCoef (0.0006f, fs);
            amount.prepare (fs);
            reset();
        }
        void reset() noexcept { scHp1.reset(); scHp2.reset(); fast = slow = energy = 0.0f; gain = 1.0f; }
        void setAmount (float a) noexcept { amount.set (std::clamp (a, 0.0f, 1.0f)); }
        bool isActive() const noexcept { return amount.active(); }

        float process (float x) noexcept
        {
            const float amt = amount.next();
            // détection sur le haut du spectre : c'est là que se trouve le « clic » du filtre
            const float h = scHp2.highpass (scHp1.highpass (x));
            energy += (h * h - energy) * rmsC;
            const float ax = std::sqrt (energy);
            fast += (ax - fast) * (ax > fast ? fastA : fastR);
            slow += (ax - slow) * (ax > slow ? slowA : slowR);
            const float jump = std::log2 ((fast + 1.0e-5f) / (slow + 1.0e-5f));   // > 0 pendant l'attaque
            // attaque renforcée (jusqu'à +10 dB) ; le corps de la note légèrement creusé (jusqu'à -3 dB)
            const float db = jump > 0.0f ? std::min (amt * 10.0f * jump, 10.0f * amt)
                                         : std::max (amt * 3.0f * jump, -3.0f * amt);
            const float target = std::exp2 (db * 0.16609640f);       // dB -> gain
            gain += (target - gain) * gSmooth;
            return x * gain;
        }

    private:
        Svf2 scHp1, scHp2;
        SmoothedAmount amount;
        float fast = 0, slow = 0, gain = 1, energy = 0;
        float rmsC = 0, fastA = 0, fastR = 0, slowA = 0, slowR = 0, gSmooth = 0;
    };

    //==========================================================================
    class MultibandSmash
    {
    public:
        void prepare (double fs) noexcept
        {
            for (auto* f : { &lo1, &lo2, &hi1, &hi2 }) f->set (150.0f, fs);
            for (auto* f : { &mid1, &mid2, &top1, &top2 }) f->set (2500.0f, fs);
            lowAp.set (2500.0f, fs);          // compensation : la bande grave reçoit la même phase que mid + aigu
            att = onePoleCoef (0.0004f, fs); rel = onePoleCoef (0.080f, fs);
            gDown = onePoleCoef (0.0005f, fs);   // le gain baisse tout de suite (pas de crête à l'attaque)
            gUp   = onePoleCoef (0.090f, fs);    // et remonte doucement (pas de pompage)
            amount.prepare (fs);
            reset();
        }
        void reset() noexcept
        {
            for (auto* f : { &lo1, &lo2, &hi1, &hi2, &mid1, &mid2, &top1, &top2, &lowAp }) f->reset();
            for (auto& e : env) e = 0.0f;
            for (auto& gg : gainDb) gg = 0.0f;
        }
        void setAmount (float a) noexcept { amount.set (std::clamp (a, 0.0f, 1.0f)); }
        bool isActive() const noexcept { return amount.active(); }

        // (tests) somme des 3 bandes sans compression : doit être plate en amplitude
        float crossoverOnly (float x) noexcept
        {
            return lowAp.allpass (lo2.lowpass (lo1.lowpass (x)))
                 + [&] { const float rest = hi2.highpass (hi1.highpass (x));
                         return mid2.lowpass (mid1.lowpass (rest)) + top2.highpass (top1.highpass (rest)); }();
        }

        float process (float x) noexcept
        {
            const float amt = amount.next();
            // Linkwitz-Riley 4e ordre (deux Butterworth en cascade) à 150 Hz et 2,5 kHz
            const float low  = lowAp.allpass (lo2.lowpass (lo1.lowpass (x)));
            const float rest = hi2.highpass (hi1.highpass (x));
            const float mid  = mid2.lowpass (mid1.lowpass (rest));
            const float top  = top2.highpass (top1.highpass (rest));
            const float bands[3] = { low, mid, top };

            float out = 0.0f;
            for (int b = 0; b < 3; ++b)
            {
                const float ax = std::abs (bands[b]);
                env[b] += (ax - env[b]) * (ax > env[b] ? att : rel);
                const float lvl = 20.0f * std::log10 (env[b] + 1.0e-7f);
                float gdb = 0.0f;
                if (lvl > kDownT)  gdb -= (lvl - kDownT) * (1.0f - 1.0f / kDownR);           // vers le bas
                else if (lvl < kUpT && lvl > kGate)                                            // vers le haut
                    gdb += std::min ((kUpT - lvl) * (1.0f - 1.0f / kUpR), kMaxUp);
                gdb = (gdb + kMakeup[b]) * amt;
                gainDb[b] += (gdb - gainDb[b]) * (gdb < gainDb[b] ? gDown : gUp);
                out += bands[b] * std::exp2 (gainDb[b] * 0.16609640f);
            }
            return out;
        }

    private:
        static constexpr float kDownT = -26.0f, kDownR = 4.0f, kUpT = -38.0f, kUpR = 3.0f, kMaxUp = 12.0f, kGate = -70.0f;
        static constexpr float kMakeup[3] = { 5.0f, 7.0f, 9.0f };   // un peu plus d'aigu : brillance « OTT »
        Svf2 lo1, lo2, hi1, hi2, mid1, mid2, top1, top2, lowAp;
        SmoothedAmount amount;
        float env[3] {}, gainDb[3] {}, att = 0, rel = 0, gDown = 0, gUp = 0;
    };

    //==========================================================================
    class JetModulator
    {
    public:
        void prepare (double sampleRate) noexcept
        {
            fs = (float) sampleRate;
            const int n = (int) (0.02 * sampleRate) + 8;       // 20 ms max
            for (auto& b : buf) b.assign ((size_t) n, 0.0f);
            mix.prepare (sampleRate); fb.prepare (sampleRate);
            dcCoef = onePoleCoef (0.05f, sampleRate);          // ~3 Hz : la boucle ne dérive pas
            reset();
        }
        void reset() noexcept
        {
            for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f);
            w = 0; fbL = fbR = dcL = dcR = 0.0f; counter = 0;
            for (auto& ch : ap) for (auto& z : ch) z = 0.0f;
        }
        void setParams (int mode, float mix01, float feedback01) noexcept
        {
            phaser = mode == 1;
            mix.set (std::clamp (mix01, 0.0f, 1.0f));
            fb.set (std::clamp (feedback01, 0.0f, 0.95f));
        }
        bool isActive() const noexcept { return mix.active(); }

        // phase01 : position dans le cycle (0..1), donnée par le tempo de l'hôte ou en roue libre
        void process (float x, float phase01, float& l, float& r) noexcept
        {
            const float m = mix.next(), f = fb.next();
            const float twoPi = 6.28318531f;
            const float mL = 0.5f + 0.5f * std::sin (twoPi * phase01);
            const float mR = 0.5f + 0.5f * std::sin (twoPi * (phase01 + 0.25f));   // 90° : mouvement stéréo
            // boucle de feedback : saturation douce + coupe du continu
            dcL += (fbL - dcL) * dcCoef; dcR += (fbR - dcR) * dcCoef;
            const float inL = x + f * fastTanh (fbL - dcL), inR = x + f * fastTanh (fbR - dcR);
            float wl, wr;
            if (! phaser)
            {
                const int n = (int) buf[0].size();
                buf[0][(size_t) w] = inL;
                buf[1][(size_t) w] = inR;
                wl = readHermite (buf[0], (float) w - (0.35f + 6.0f * mL * mL) * 0.001f * fs, n);   // balayage plus lent en bas
                wr = readHermite (buf[1], (float) w - (0.35f + 6.0f * mR * mR) * 0.001f * fs, n);
                w = (w + 1) % n;
            }
            else
            {
                if ((counter++ & 7) == 0)   // coefficients recalculés toutes les 8 échantillons
                {
                    setPhaserCoefs (0, 150.0f * std::exp2 (4.5f * mL));
                    setPhaserCoefs (1, 150.0f * std::exp2 (4.5f * mR));
                }
                wl = allpassChain (0, inL);
                wr = allpassChain (1, inR);
            }
            fbL = wl; fbR = wr;
            const float norm = 1.0f / (1.0f + 0.6f * m);
            l = (x + m * wl) * norm;
            r = (x + m * wr) * norm;
        }

    private:
        // Interpolation cubique (Hermite 4 points) : pas d'assourdissement des aigus au balayage
        static float readHermite (const std::vector<float>& b, float pos, int n) noexcept
        {
            while (pos < 1.0f) pos += (float) n;
            const int i1 = (int) pos; const float t = pos - (float) i1;
            const float y0 = b[(size_t) ((i1 - 1) % n)], y1 = b[(size_t) (i1 % n)];
            const float y2 = b[(size_t) ((i1 + 1) % n)], y3 = b[(size_t) ((i1 + 2) % n)];
            const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
            return ((c3 * t + c2) * t + c1) * t + y1;
        }
        void setPhaserCoefs (int ch, float hz) noexcept
        {
            static constexpr float spread[6] = { 1.0f, 1.45f, 2.1f, 3.0f, 4.3f, 6.2f };   // notches étagées
            for (int s = 0; s < 6; ++s)
            {
                const float t = std::tan (3.14159265f * std::min (hz * spread[s], 0.45f * fs) / fs);
                apA[ch][s] = (t - 1.0f) / (t + 1.0f);
            }
        }
        float allpassChain (int ch, float x) noexcept
        {
            for (int s = 0; s < 6; ++s)
            {
                const float a = apA[ch][s];
                const float y = a * x + ap[ch][s];
                ap[ch][s] = x - a * y;
                x = y;
            }
            return x;
        }
        std::vector<float> buf[2];
        int w = 0; unsigned counter = 0;
        float fs = 48000.0f, fbL = 0, fbR = 0, dcL = 0, dcR = 0, dcCoef = 0;
        float ap[2][6] {}, apA[2][6] {};
        SmoothedAmount mix, fb;
        bool phaser = false;
    };

    //==========================================================================
    class Pumper
    {
    public:
        void prepare (double fs) noexcept { sm = onePoleCoef (0.0015f, fs); g = 1.0f; depth.prepare (fs); }
        void setDepth (float d) noexcept { depth.set (std::clamp (d, 0.0f, 1.0f)); }
        bool isActive() const noexcept { return depth.active(); }
        // phase01 : position dans le temps de pompe (0 = le temps)
        float gain (float phase01) noexcept
        {
            float duck;
            if (phase01 < 0.03f)      duck = phase01 / 0.03f;                       // descente rapide sans clic
            else if (phase01 < 0.65f) { const float t = 1.0f - (phase01 - 0.03f) / 0.62f; duck = t * t; }
            else                      duck = 0.0f;
            const float target = 1.0f - depth.next() * 0.85f * duck;
            g += (target - g) * sm;
            return g;
        }
    private:
        SmoothedAmount depth;
        float g = 1, sm = 0;
    };

    //==========================================================================
    class StereoWidener
    {
    public:
        void prepare (double fs) noexcept
        {
            hp1.set (220.0f, fs); hp2.set (220.0f, fs);
            amount.prepare (fs);
            reset();
        }
        void reset() noexcept { hp1.reset(); hp2.reset(); for (auto& z : za) z = 0.0f; for (auto& z : zb) z = 0.0f; }
        void setAmount (float a) noexcept { amount.set (std::clamp (a, 0.0f, 1.0f)); }
        bool isActive() const noexcept { return amount.active(); }

        void process (float& l, float& r) noexcept
        {
            const float amt = amount.next();
            const float m = 0.5f * (l + r);
            float s = 0.5f * (l - r);
            // deux chaînes passe-tout différentes : composante décorrélée du centre
            static constexpr float ca[4] = { 0.62f, -0.31f, 0.47f, -0.55f };
            static constexpr float cb[4] = { -0.58f, 0.36f, -0.44f, 0.51f };
            float xa = m, xb = m;
            for (int i = 0; i < 4; ++i)
            {
                float y = ca[i] * xa + za[i]; za[i] = xa - ca[i] * y; xa = y;
                y = cb[i] * xb + zb[i]; zb[i] = xb - cb[i] * y; xb = y;
            }
            const float d = hp2.highpass (hp1.highpass (0.5f * (xa - xb)));   // grave gardé au centre
            s = s * (1.0f + 1.0f * amt) + d * 4.5f * amt;   // même un son mono s'ouvre nettement
            l = m + s; r = m - s;
        }
    private:
        Svf2 hp1, hp2;
        SmoothedAmount amount;
        float za[4] {}, zb[4] {};
    };
}

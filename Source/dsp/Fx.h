#pragma once

#include "DspUtils.h"
#include <vector>

namespace octocid
{
    //==========================================================================
    // PUNCH : compresseur feed-forward (conception de Giannoulis, Massberg &
    // Reiss, « Digital Dynamic Range Compressor Design », JAES 2012) :
    // détection en dB, genou doux, détecteur de crête lissé à deux vitesses.
    // Une attaque de quelques ms laisse passer le claquement des notes, puis le
    // corps est tassé : les accents ressortent, la ligne devient dense et forte.
    // Le réglage Punch baisse le seuil ; le gain de compensation suit.
    //==========================================================================
    class PunchCompressor
    {
    public:
        void prepare (double sampleRate) noexcept
        {
            fs = sampleRate;
            releaseCoef = std::exp (-1.0f / (0.07f * (float) sampleRate));    // 70 ms
            smooth = onePoleCoef (0.01f, sampleRate);
            setAttackMs (10.0f);
            reset();
        }

        void reset() noexcept { yL = 0.0f; amt = targetAmt; }

        void setAmount (float amount01) noexcept { targetAmt = std::clamp (amount01, 0.0f, 1.0f); }

        void setAttackMs (float ms) noexcept
        {
            ms = std::clamp (ms, 0.1f, 50.0f);
            if (std::abs (ms - attackMs) > 1.0e-3f)
            {
                attackMs = ms;
                attackCoef = std::exp (-1.0f / (ms * 0.001f * (float) fs));
            }
        }

        bool isActive() const noexcept { return amt > 1.0e-4f || targetAmt > 1.0e-4f; }

        float process (float x) noexcept
        {
            amt += (targetAmt - amt) * smooth;

            const float threshold = -10.0f - 26.0f * amt;     // dBFS
            constexpr float ratio = 4.0f, knee = 8.0f;

            const float xg = 20.0f * std::log10 (std::abs (x) + 1.0e-9f);
            const float over = xg - threshold;
            float yg;
            if (2.0f * over < -knee)              yg = xg;
            else if (2.0f * std::abs (over) <= knee)
            {
                const float t = over + 0.5f * knee;
                yg = xg + (1.0f / ratio - 1.0f) * t * t / (2.0f * knee);
            }
            else                                  yg = threshold + over / ratio;

            const float xl = xg - yg;              // réduction voulue (dB, >= 0)
            yL = xl > yL ? attackCoef * yL + (1.0f - attackCoef) * xl
                         : releaseCoef * yL + (1.0f - releaseCoef) * xl;

            const float makeup = 12.0f * amt;      // calibré au test sur les lignes de démo
            return x * std::pow (10.0f, (makeup - yL) * 0.05f);
        }

    private:
        double fs = 44100.0;
        float targetAmt = 0.0f, amt = 0.0f, smooth = 0.01f;
        float attackMs = -1.0f, attackCoef = 0.0f, releaseCoef = 0.0f, yL = 0.0f;
    };

    //==========================================================================
    // DELAY ping-pong synchronisé au tempo. Dans la boucle : passe-haut 250 Hz
    // (le grave reste sec et serré), passe-bas 5 kHz (répétitions qui
    // s'assombrissent) et saturation douce, façon bande. Le temps glisse
    // doucement quand il change (pas de clic).
    //==========================================================================
    class PingPongDelay
    {
    public:
        void prepare (double sampleRate, double maxSeconds = 2.5) noexcept
        {
            fs = sampleRate;
            size = (int) (sampleRate * maxSeconds) + 4;
            bufL.assign ((size_t) size, 0.0f);
            bufR.assign ((size_t) size, 0.0f);
            hpG = std::tan (kPi * 250.0f / (float) sampleRate);
            lpG = std::tan (kPi * 5000.0f / (float) sampleRate);
            timeSmooth = onePoleCoef (0.08f, sampleRate);
            smooth = onePoleCoef (0.01f, sampleRate);
            reset();
        }

        void reset() noexcept
        {
            std::fill (bufL.begin(), bufL.end(), 0.0f);
            std::fill (bufR.begin(), bufR.end(), 0.0f);
            write = 0; hpS[0] = hpS[1] = lpS[0] = lpS[1] = 0.0f;
            curDelay = targetDelay; mix = targetMix; fb = targetFb;
        }

        void setParams (float seconds, float feedback01, float mix01) noexcept
        {
            targetDelay = std::clamp (seconds * (float) fs, 4.0f, (float) (size - 4));
            targetFb  = std::clamp (feedback01, 0.0f, 0.92f);
            targetMix = std::clamp (mix01, 0.0f, 1.0f);
        }

        bool isActive() const noexcept { return mix > 1.0e-4f || targetMix > 1.0e-4f; }

        void process (float& l, float& r) noexcept
        {
            curDelay += (targetDelay - curDelay) * timeSmooth;
            mix += (targetMix - mix) * smooth;
            fb  += (targetFb  - fb)  * smooth;

            const float dl = read (bufL, curDelay);
            const float dr = read (bufR, curDelay);

            // L reçoit l'entrée + le retour de R ; R reçoit le retour de L : ping-pong
            const float inMono = 0.5f * (l + r);
            bufL[(size_t) write] = loop (inMono + fb * dr, 0);
            bufR[(size_t) write] = loop (fb * dl, 1);
            if (++write >= size) write = 0;

            l += mix * dl;
            r += mix * dr;
        }

    private:
        float read (const std::vector<float>& b, float delay) const noexcept
        {
            float pos = (float) write - delay;
            if (pos < 0.0f) pos += (float) size;
            const int i0 = (int) pos;
            const float fr = pos - (float) i0;
            const int i1 = i0 + 1 >= size ? 0 : i0 + 1;
            return b[(size_t) i0] + fr * (b[(size_t) i1] - b[(size_t) i0]);
        }

        float loop (float x, int ch) noexcept
        {
            // passe-haut puis passe-bas (TPT 1 pôle), puis saturation douce
            float v = (x - hpS[ch]) * (hpG / (1.0f + hpG));
            const float hpLow = v + hpS[ch];
            hpS[ch] = hpLow + v;
            x -= hpLow;

            v = (x - lpS[ch]) * (lpG / (1.0f + lpG));
            const float lp = v + lpS[ch];
            lpS[ch] = lp + v;
            return fastTanh (lp * 1.2f) * (1.0f / 1.2f);
        }

        double fs = 44100.0;
        std::vector<float> bufL, bufR;
        int size = 1, write = 0;
        float hpG = 0.0f, lpG = 0.0f, hpS[2] {}, lpS[2] {};
        float targetDelay = 1000.0f, curDelay = 1000.0f, timeSmooth = 0.001f;
        float targetFb = 0.3f, fb = 0.3f, targetMix = 0.0f, mix = 0.0f, smooth = 0.01f;
    };

    //==========================================================================
    // REVERB plate : topologie de Griesinger décrite par Dattorro (« Effect
    // Design Part 1 », JAES 1997) — 4 allpass de diffusion en entrée, puis une
    // boucle de deux branches (allpass modulé, délai, amortissement, allpass,
    // délai). Délais et prises de sortie stéréo de l'article, mis à l'échelle de
    // la fréquence d'échantillonnage. Structure aussi utilisée par les reverbs
    // de Mutable Instruments (Émilie Gillet, licence MIT), qui ont servi de
    // référence. Coupe-grave à l'entrée : la basse reste sèche, seul le haut
    // de la ligne acid s'ouvre dans l'espace.
    //==========================================================================
    class PlateReverb
    {
    public:
        void prepare (double sampleRate) noexcept
        {
            fs = sampleRate;
            const double k = sampleRate / 29761.0;
            auto len = [k] (double n) { return (int) std::ceil (n * k) + 1; };

            const int inLen[4] = { len (142), len (107), len (379), len (277) };
            for (int i = 0; i < 4; ++i) inAp[i].init (inLen[i]);
            apA.init (len (672 + 32));  dA1.init (len (4453));  apA2.init (len (1800));  dA2.init (len (3720));
            apB.init (len (908 + 32));  dB1.init (len (4217));  apB2.init (len (2656));  dB2.init (len (3163));
            pre.init (len (0.012 * 29761.0));

            apALen = (float) (672 * k);  apBLen = (float) (908 * k);
            excursion = (float) (16.0 * k);
            for (int i = 0; i < 4; ++i) inLenF[i] = inLen[i] - 1;

            auto tap = [k] (double n) { return (int) std::round (n * k); };
            tL[0] = tap (266);  tL[1] = tap (2974); tL[2] = tap (1913); tL[3] = tap (1996);
            tL[4] = tap (1990); tL[5] = tap (187);  tL[6] = tap (1066);
            tR[0] = tap (353);  tR[1] = tap (3627); tR[2] = tap (1228); tR[3] = tap (2673);
            tR[4] = tap (2111); tR[5] = tap (335);  tR[6] = tap (121);

            hpG = std::tan (kPi * 350.0f / (float) sampleRate);
            bwCoef = 1.0f - std::exp (-2.0f * kPi * 9000.0f / (float) sampleRate);
            lfoInc = 1.0f / (float) sampleRate;
            smooth = onePoleCoef (0.02f, sampleRate);
            reset();
        }

        void reset() noexcept
        {
            for (auto* d : { &inAp[0], &inAp[1], &inAp[2], &inAp[3], &apA, &dA1, &apA2, &dA2, &apB, &dB1, &apB2, &dB2, &pre })
                d->clear();
            hpS = bwS = dampA = dampB = 0.0f; lfoPhase = 0.0f;
            mix = targetMix; decay = targetDecay; damp = targetDamp;
        }

        // decay01 : 0..1 (queue courte à très longue), tone01 : 0 = sombre, 1 = brillant
        void setParams (float mix01, float decay01, float tone01) noexcept
        {
            targetMix   = std::clamp (mix01, 0.0f, 1.0f);
            targetDecay = 0.25f + 0.72f * std::clamp (decay01, 0.0f, 1.0f);
            const float fc = 1500.0f * std::pow (8.0f, std::clamp (tone01, 0.0f, 1.0f));   // 1,5..12 kHz
            targetDamp = std::exp (-2.0f * kPi * fc / (float) fs);
        }

        bool isActive() const noexcept { return mix > 1.0e-4f || targetMix > 1.0e-4f; }

        void process (float& l, float& r) noexcept
        {
            mix += (targetMix - mix) * smooth;
            decay += (targetDecay - decay) * smooth;
            damp += (targetDamp - damp) * smooth;

            // Entrée mono : coupe-grave 350 Hz, largeur de bande, pré-délai
            float x = 0.5f * (l + r);
            float v = (x - hpS) * (hpG / (1.0f + hpG));
            const float low = v + hpS;
            hpS = low + v;
            x -= low;
            bwS += (x - bwS) * bwCoef;
            pre.push (bwS);
            x = pre.tap (pre.length() - 1);

            // Diffusion d'entrée
            x = inAp[0].allpass (x, 0.75f, inLenF[0]);
            x = inAp[1].allpass (x, 0.75f, inLenF[1]);
            x = inAp[2].allpass (x, 0.625f, inLenF[2]);
            x = inAp[3].allpass (x, 0.625f, inLenF[3]);

            // LFO de modulation (~1 Hz), en quadrature sur les deux branches
            lfoPhase += lfoInc;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            const float mA = excursion * std::sin (2.0f * kPi * lfoPhase);
            const float mB = excursion * std::cos (2.0f * kPi * lfoPhase);

            // Branche A (nourrie par la fin de la branche B)
            float a = x + decay * dB2.tap (dB2.length() - 1);
            a = apA.allpassFrac (a, -0.7f, apALen + mA);
            dA1.push (a);
            float aOut = dA1.tap (dA1.length() - 1);
            dampA = aOut * (1.0f - damp) + dampA * damp;
            a = apA2.allpass (dampA * decay, 0.5f, apA2.length() - 1);
            dA2.push (a);

            // Branche B (nourrie par la fin de la branche A)
            float b = x + decay * dA2.tap (dA2.length() - 1);
            b = apB.allpassFrac (b, -0.7f, apBLen + mB);
            dB1.push (b);
            float bOut = dB1.tap (dB1.length() - 1);
            dampB = bOut * (1.0f - damp) + dampB * damp;
            b = apB2.allpass (dampB * decay, 0.5f, apB2.length() - 1);
            dB2.push (b);

            // Prises de sortie (Dattorro)
            const float wl = 0.6f * ( dB1.tap (tL[0]) + dB1.tap (tL[1]) - apB2.tap (tL[2]) + dB2.tap (tL[3])
                                    - dA1.tap (tL[4]) - apA2.tap (tL[5]) - dA2.tap (tL[6]));
            const float wr = 0.6f * ( dA1.tap (tR[0]) + dA1.tap (tR[1]) - apA2.tap (tR[2]) + dA2.tap (tR[3])
                                    - dB1.tap (tR[4]) - apB2.tap (tR[5]) - dB2.tap (tR[6]));
            l += mix * wl;
            r += mix * wr;
        }

    private:
        // Ligne à retard circulaire simple (tap(n) = échantillon écrit il y a n pas)
        struct Line
        {
            std::vector<float> b;
            int w = 0, n = 1;
            void init (int len) { n = std::max (2, len); b.assign ((size_t) n, 0.0f); w = 0; }
            void clear() { std::fill (b.begin(), b.end(), 0.0f); w = 0; }
            int length() const noexcept { return n; }
            void push (float x) noexcept { b[(size_t) w] = x; if (++w >= n) w = 0; }
            float tap (int d) const noexcept
            {
                int i = w - 1 - std::min (d, n - 1);
                if (i < 0) i += n;
                return b[(size_t) i];
            }
            float tapFrac (float d) const noexcept
            {
                d = std::clamp (d, 0.0f, (float) (n - 2));
                const int i0 = (int) d;
                const float fr = d - (float) i0;
                return tap (i0) + fr * (tap (i0 + 1) - tap (i0));
            }
            // Allpass de Schroeder sur d échantillons
            float allpass (float x, float g, int d) noexcept
            {
                const float delayed = tap (d - 1);
                const float v = x - g * delayed;
                push (v);
                return delayed + g * v;
            }
            float allpassFrac (float x, float g, float d) noexcept
            {
                const float delayed = tapFrac (d - 1.0f);
                const float v = x - g * delayed;
                push (v);
                return delayed + g * v;
            }
        };

        double fs = 44100.0;
        Line inAp[4], apA, dA1, apA2, dA2, apB, dB1, apB2, dB2, pre;
        int inLenF[4] {};
        int tL[7] {}, tR[7] {};
        float apALen = 0.0f, apBLen = 0.0f, excursion = 0.0f;
        float hpG = 0.0f, hpS = 0.0f, bwCoef = 0.5f, bwS = 0.0f, dampA = 0.0f, dampB = 0.0f;
        float lfoPhase = 0.0f, lfoInc = 0.0f;
        float targetMix = 0.0f, mix = 0.0f, targetDecay = 0.5f, decay = 0.5f, targetDamp = 0.3f, damp = 0.3f, smooth = 0.01f;
    };
}

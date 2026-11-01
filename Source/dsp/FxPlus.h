#pragma once

#include "DspUtils.h"
#include <cmath>
#include <vector>

//==============================================================================
// Effets « percutant / transcendant » (code maison, sans allocation en lecture)
//
//  IMPACT  : façonneur de transitoires (suiveur rapide / suiveur lent, à la manière
//            des « transient designers ») : renforce l'attaque de chaque note.
//  SMASH   : compression 3 bandes vers le haut ET vers le bas (esprit « OTT ») :
//            les détails remontent, les pics sont tenus, la ligne devient dense.
//  JET     : flanger ou phaser stéréo, synchronisable au tempo : le mouvement
//            « qui serpente » des lignes acid.
//  PUMP    : pompe rythmique (ducking synchronisé, effet sidechain).
//  WIDTH   : élargisseur stéréo compatible mono (le grave reste au centre).
//==============================================================================
namespace octocid
{
    //==========================================================================
    // SVF TPT (Zavalishin) : passe-bas / passe-haut 2e ordre, Butterworth
    //==========================================================================
    struct Svf2
    {
        float g = 0.0f, k = 1.41421356f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f, ic1 = 0.0f, ic2 = 0.0f;
        void set (float hz, double fs) noexcept
        {
            g = std::tan (3.14159265f * std::min (hz, 0.45f * (float) fs) / (float) fs);
            a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
        }
        void reset() noexcept { ic1 = ic2 = 0.0f; }
        // renvoie le passe-bas ; hp reçoit le passe-haut
        float process (float x, float& hp) noexcept
        {
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
            hp = x - k * v1 - v2;
            return v2;
        }
    };

    //==========================================================================
    class TransientShaper
    {
    public:
        void prepare (double fs) noexcept
        {
            fastA = onePoleCoef (0.0005f, fs); fastR = onePoleCoef (0.030f, fs);
            slowA = onePoleCoef (0.020f, fs);  slowR = onePoleCoef (0.060f, fs);
            gSmooth = onePoleCoef (0.0008f, fs);
            reset();
        }
        void reset() noexcept { fast = slow = 0.0f; gain = 1.0f; }
        void setAmount (float a) noexcept { amount = std::clamp (a, 0.0f, 1.0f); }
        bool isActive() const noexcept { return amount > 1.0e-3f; }

        float process (float x) noexcept
        {
            const float ax = std::abs (x);
            fast += (ax - fast) * (ax > fast ? fastA : fastR);
            slow += (ax - slow) * (ax > slow ? slowA : slowR);
            const float ratio = (fast + 1.0e-6f) / (slow + 1.0e-6f);
            // jusqu'à +12 dB sur l'attaque, proportionnel au « saut » d'enveloppe
            const float boostDb = std::clamp (amount * 8.0f * std::log2 (std::max (ratio, 1.0f)), 0.0f, 9.0f * amount);
            const float target = std::exp2 (boostDb * 0.16609640f);   // dB -> gain (log2(10)/20)
            gain += (target - gain) * gSmooth;
            return x * gain;
        }

    private:
        float fast = 0, slow = 0, gain = 1, amount = 0;
        float fastA = 0, fastR = 0, slowA = 0, slowR = 0, gSmooth = 0;
    };

    //==========================================================================
    class MultibandSmash
    {
    public:
        void prepare (double fs) noexcept
        {
            for (auto* f : { &lo1, &lo2, &hi1, &hi2 }) f->set (150.0f, fs);
            for (auto* f : { &mid1, &mid2, &top1, &top2 }) f->set (2500.0f, fs);
            att = onePoleCoef (0.0004f, fs); rel = onePoleCoef (0.080f, fs);
            gDown = onePoleCoef (0.0005f, fs);   // le gain baisse tout de suite (pas de crête à l'attaque)
            gUp   = onePoleCoef (0.090f, fs);    // et remonte doucement (pas de pompage)
            reset();
        }
        void reset() noexcept
        {
            for (auto* f : { &lo1, &lo2, &hi1, &hi2, &mid1, &mid2, &top1, &top2 }) f->reset();
            for (auto& e : env) e = 0.0f;
            for (auto& gg : gainDb) gg = 0.0f;
        }
        void setAmount (float a) noexcept { amount = std::clamp (a, 0.0f, 1.0f); }
        bool isActive() const noexcept { return amount > 1.0e-3f; }

        float process (float x) noexcept
        {
            // Linkwitz-Riley 4e ordre (deux Butterworth en cascade) : 150 Hz et 2,5 kHz
            float h1, h2, d;
            float low = lo2.process (lo1.process (x, d), d);
            hi1.process (x, h1); hi2.process (h1, h2);
            const float rest = h2;
            float t1, t2;
            float mid = mid2.process (mid1.process (rest, d), d);
            top1.process (rest, t1); top2.process (t1, t2);
            float bands[3] = { low, mid, t2 };

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
                gdb = gdb * amount + kMakeup[b] * amount;
                gainDb[b] += (gdb - gainDb[b]) * (gdb < gainDb[b] ? gDown : gUp);
                out += bands[b] * std::exp2 (gainDb[b] * 0.16609640f);
            }
            return out;
        }

    private:
        static constexpr float kDownT = -26.0f, kDownR = 4.0f, kUpT = -38.0f, kUpR = 3.0f, kMaxUp = 12.0f, kGate = -70.0f;
        static constexpr float kMakeup[3] = { 5.0f, 7.0f, 9.0f };   // un peu plus d'aigu : brillance « OTT »
        Svf2 lo1, lo2, hi1, hi2, mid1, mid2, top1, top2;
        float env[3] {}, gainDb[3] {}, att = 0, rel = 0, gDown = 0, gUp = 0, amount = 0;
    };

    //==========================================================================
    class JetModulator
    {
    public:
        void prepare (double sampleRate) noexcept
        {
            fs = (float) sampleRate;
            const int n = (int) (0.02 * sampleRate) + 4;       // 20 ms max
            for (auto& b : buf) b.assign ((size_t) n, 0.0f);
            reset();
        }
        void reset() noexcept
        {
            for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f);
            w = 0; fbL = fbR = 0.0f;
            for (auto& ch : ap) for (auto& z : ch) z = 0.0f;
        }
        void setParams (int mode, float mix01, float feedback01) noexcept
        {
            phaser = mode == 1; mix = std::clamp (mix01, 0.0f, 1.0f); fb = std::clamp (feedback01, 0.0f, 0.95f);
        }
        bool isActive() const noexcept { return mix > 1.0e-3f; }

        // phase01 : position dans le cycle du LFO (0..1), fournie par l'hôte ou en roue libre
        void process (float x, float phase01, float& l, float& r) noexcept
        {
            const float twoPi = 6.28318531f;
            const float mL = 0.5f + 0.5f * std::sin (twoPi * phase01);
            const float mR = 0.5f + 0.5f * std::sin (twoPi * (phase01 + 0.25f));   // 90° : mouvement stéréo
            float wl, wr;
            if (! phaser)
            {
                const float dL = (0.4f + 6.0f * mL) * 0.001f * fs, dR = (0.4f + 6.0f * mR) * 0.001f * fs;
                const int n = (int) buf[0].size();
                buf[0][(size_t) w] = x + fb * fbL;
                buf[1][(size_t) w] = x + fb * fbR;
                wl = readFrac (buf[0], (float) w - dL, n);
                wr = readFrac (buf[1], (float) w - dR, n);
                fbL = fastTanh (wl); fbR = fastTanh (wr);
                w = (w + 1) % n;
            }
            else
            {
                wl = allpassChain (0, x + fb * fbL, 180.0f * std::exp2 (4.2f * mL));
                wr = allpassChain (1, x + fb * fbR, 180.0f * std::exp2 (4.2f * mR));
                fbL = fastTanh (wl); fbR = fastTanh (wr);
            }
            const float norm = 1.0f / (1.0f + 0.6f * mix);
            l = (x + mix * wl) * norm;
            r = (x + mix * wr) * norm;
        }

    private:
        static float readFrac (const std::vector<float>& b, float pos, int n) noexcept
        {
            while (pos < 0.0f) pos += (float) n;
            const int i0 = (int) pos; const float f = pos - (float) i0;
            return b[(size_t) (i0 % n)] * (1.0f - f) + b[(size_t) ((i0 + 1) % n)] * f;
        }
        float allpassChain (int ch, float x, float hz) noexcept
        {
            const float t = std::tan (3.14159265f * std::min (hz, 0.45f * fs) / fs);
            const float a = (t - 1.0f) / (t + 1.0f);
            for (int s = 0; s < 6; ++s)
            {
                const float y = a * x + ap[ch][s];
                ap[ch][s] = x - a * y;
                x = y;
            }
            return x;
        }
        std::vector<float> buf[2];
        int w = 0;
        float fs = 48000.0f, mix = 0, fb = 0, fbL = 0, fbR = 0;
        float ap[2][6] {};
        bool phaser = false;
    };

    //==========================================================================
    class Pumper
    {
    public:
        void prepare (double fs) noexcept { sm = onePoleCoef (0.0015f, fs); g = 1.0f; }
        void setDepth (float d) noexcept { depth = std::clamp (d, 0.0f, 1.0f); }
        bool isActive() const noexcept { return depth > 1.0e-3f; }
        // phase01 : position dans le temps de pompe (0 = le temps)
        float gain (float phase01) noexcept
        {
            float duck;
            if (phase01 < 0.03f)      duck = phase01 / 0.03f;                       // descente rapide sans clic
            else if (phase01 < 0.65f) { const float t = 1.0f - (phase01 - 0.03f) / 0.62f; duck = t * t; }
            else                      duck = 0.0f;
            const float target = 1.0f - depth * 0.85f * duck;
            g += (target - g) * sm;
            return g;
        }
    private:
        float depth = 0, g = 1, sm = 0;
    };

    //==========================================================================
    class StereoWidener
    {
    public:
        void prepare (double fs) noexcept
        {
            hp1.set (220.0f, fs); hp2.set (220.0f, fs);
            reset();
        }
        void reset() noexcept { hp1.reset(); hp2.reset(); for (auto& z : za) z = 0.0f; for (auto& z : zb) z = 0.0f; }
        void setAmount (float a) noexcept { amount = std::clamp (a, 0.0f, 1.0f); }
        bool isActive() const noexcept { return amount > 1.0e-3f; }

        void process (float& l, float& r) noexcept
        {
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
            float h, h2;
            hp1.process (0.5f * (xa - xb), h);
            hp2.process (h, h2);                          // grave gardé au centre
            s = s * (1.0f + 1.0f * amount) + h2 * 1.4f * amount;
            l = m + s; r = m - s;
        }
    private:
        Svf2 hp1, hp2;
        float za[4] {}, zb[4] {}, amount = 0;
    };
}

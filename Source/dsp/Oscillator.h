#pragma once

#include "DspUtils.h"

namespace octocid
{
    //==========================================================================
    // Oscillateur acid : dent de scie anti-aliasée (polyBLEP) et carré dérivé
    // de la dent de scie par un waveshaper façon transistor (comme l'original :
    // fronts légèrement arrondis, pas un carré parfait).
    //==========================================================================
    class AcidOscillator
    {
    public:
        void prepare (double sampleRate) noexcept
        {
            // Bloqueur de DC ~8 Hz (le carré à rapport cyclique décalé produit du DC)
            dcR = std::exp (-2.0f * kPi * 8.0f / (float) sampleRate);
            reset();
        }

        void reset() noexcept { phase = 0.0f; dcX = dcY = 0.0f; }

        // inc = fréquence / sampleRate ; squareMix : 0 = saw, 1 = square
        float process (float inc, float squareMix) noexcept
        {
            float saw = 2.0f * phase - 1.0f;
            saw -= polyBlep (phase, inc);

            phase += inc;
            if (phase >= 1.0f)
                phase -= 1.0f;

            float out = -saw;   // rampe descendante, comme l'original

            if (squareMix > 0.0001f)
            {
                // Saturation forte de la rampe : transition douce sur ~10 % de la période,
                // léger biais pour un rapport cyclique légèrement asymétrique.
                const float sq = 0.72f * fastTanh (12.0f * saw + 0.35f);
                out += squareMix * (sq - out);
            }

            // Bloqueur de DC
            const float y = out - dcX + dcR * dcY;
            dcX = out;
            dcY = y;
            return y;
        }

    private:
        static float polyBlep (float t, float dt) noexcept
        {
            if (t < dt)        { t /= dt;          return t + t - t * t - 1.0f; }
            if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
            return 0.0f;
        }

        float phase = 0.0f;
        float dcR = 0.999f, dcX = 0.0f, dcY = 0.0f;
    };
}

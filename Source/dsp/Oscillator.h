#pragma once

#include "DspUtils.h"

namespace octocid
{
    //==========================================================================
    // Oscillateur acid :
    //  - dent de scie descendante anti-aliasée (polyBLEP),
    //  - carré façon 303 : sur l'original il est obtenu en saturant très fort la
    //    rampe avec un léger décalage (rapport cyclique ~47 %). On le génère
    //    directement comme une impulsion polyBLEP à 47 %, ce qui donne la même
    //    forme sans l'aliasing qu'introduirait la saturation.
    //==========================================================================
    class AcidOscillator
    {
    public:
        static constexpr float kSquareDuty = 0.53f;   // partie haute avant inversion -> 47 % après

        void prepare (double) noexcept { reset(); }
        void reset() noexcept { phase = 0.0f; }
        void resetPhase() noexcept { phase = 0.0f; }

        // inc = fréquence / sampleRate ; squareMix : 0 = saw, 1 = square
        float process (float inc, float squareMix) noexcept
        {
            const float t = phase;

            float saw = 2.0f * t - 1.0f;
            saw -= polyBlep (t, inc);

            float out = -saw;   // polarité de l'original

            if (squareMix > 0.0001f)
            {
                float sq = t < kSquareDuty ? 1.0f : -1.0f;
                sq += polyBlep (t, inc);
                float t2 = t + 1.0f - kSquareDuty;
                if (t2 >= 1.0f) t2 -= 1.0f;
                sq -= polyBlep (t2, inc);
                out += squareMix * (-sq - out);
            }

            phase += inc;
            if (phase >= 1.0f)
                phase -= 1.0f;

            return out;
        }

    private:
        static float polyBlep (float t, float dt) noexcept
        {
            if (t < dt)        { t /= dt;              return t + t - t * t - 1.0f; }
            if (t > 1.0f - dt) { t = (t - 1.0f) / dt;  return t * t + t + t + 1.0f; }
            return 0.0f;
        }

        float phase = 0.0f;
    };
}

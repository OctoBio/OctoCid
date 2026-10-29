#pragma once

#include "DspUtils.h"

namespace octocid
{
    //==========================================================================
    // Oscillateur acid :
    //  - dent de scie descendante anti-aliasée (polyBLEP),
    //  - carré façon 303 : sur l'original, la rampe passe dans un transistor
    //    saturé avec un décalage. Forme mesurée par Open303 (MIT) :
    //        sq = tanh(F * (2p - 1) + O), F = 36,9 dB (~70), O = 4,37
    //    Un front est une rampe douce (la saturation), l'autre est dur : seul
    //    celui-ci reçoit une correction polyBLEP. Le DC (~O/F) est retiré.
    //==========================================================================
    class AcidOscillator
    {
    public:
        static constexpr float kSqF  = 69.984200f;          // 10^(36,9/20)
        static constexpr float kSqO  = 4.37f;
        static constexpr float kSqDC = kSqO / kSqF;         // moyenne de la forme

        void prepare (double) noexcept { reset(); }
        void reset() noexcept { phase = 0.0f; }
        void resetPhase() noexcept { phase = 0.0f; }

        // inc = fréquence / sampleRate ; squareMix : 0 = saw, 1 = square
        float process (float inc, float squareMix) noexcept
        {
            const float t = phase;
            const float ramp = 2.0f * t - 1.0f;

            float out = -(ramp - polyBlep (t, inc));   // dent de scie, polarité de l'original

            if (squareMix > 0.0001f)
            {
                // front dur de -2 au retour de phase -> correction polyBLEP
                const float sq = std::tanh (kSqF * ramp + kSqO) - polyBlep (t, inc) - kSqDC;
                out += squareMix * (sq - out);
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

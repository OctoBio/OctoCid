#pragma once

#include "DspUtils.h"
#include <cstdint>

namespace octocid
{
    //==========================================================================
    // LFO unique : Sine, Triangle, Square, S&H. Sortie bipolaire -1..+1.
    // Les formes à sauts (Square, S&H) sont adoucies (~2 ms) pour éviter les clics.
    // La phase peut être recalée sur la position du DAW (mode synchronisé).
    //==========================================================================
    class Lfo
    {
    public:
        enum Shape { Sine = 0, Triangle = 1, Square = 2, SampleHold = 3 };

        void prepare (double sampleRate) noexcept
        {
            invFs = 1.0f / (float) sampleRate;
            slew = onePoleCoef (0.002f, sampleRate);
            reset();
        }

        void reset() noexcept { phase = 0.0f; out = 0.0f; held = 0.0f; }

        void setRate (float hz) noexcept { inc = std::clamp (hz, 0.0f, 50.0f) * invFs; }
        void setShape (int s) noexcept { shape = (Shape) std::clamp (s, 0, 3); }

        // Recale la phase (0..1) sur la position du transport
        void syncPhase (float p) noexcept
        {
            p -= std::floor (p);
            if (shape == SampleHold && p < phase)
                held = nextRandom();
            phase = p;
        }

        float process() noexcept
        {
            phase += inc;
            if (phase >= 1.0f)
            {
                phase -= std::floor (phase);
                held = nextRandom();
            }

            float target;
            switch (shape)
            {
                case Sine:       target = std::sin (2.0f * kPi * phase); break;
                case Triangle:   target = 1.0f - 4.0f * std::abs (phase - 0.5f); break;
                case Square:     target = phase < 0.5f ? 1.0f : -1.0f; break;
                case SampleHold:
                default:         target = held; break;
            }

            if (shape == Sine || shape == Triangle)
                out = target;
            else
                out += (target - out) * slew;
            return out;
        }

    private:
        float nextRandom() noexcept
        {
            rng = rng * 1664525u + 1013904223u;
            return (float) (rng >> 8) * (2.0f / 16777216.0f) - 1.0f;
        }

        float invFs = 1.0f / 44100.0f, inc = 0.0f, phase = 0.0f, out = 0.0f, held = 0.0f, slew = 0.01f;
        Shape shape = Sine;
        uint32_t rng = 0x12345678u;
    };
}

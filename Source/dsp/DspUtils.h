#pragma once

#include <cmath>
#include <algorithm>

namespace octocid
{
    constexpr float kPi = 3.14159265358979f;

    // tanh rapide (Padé), précis à ~1 % et borné à ±1
    inline float fastTanh (float x) noexcept
    {
        if (x > 3.0f)  return 1.0f;
        if (x < -3.0f) return -1.0f;
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    // Coefficient d'un lissage exponentiel (one-pole) pour une constante de temps donnée
    inline float onePoleCoef (float tauSeconds, double sampleRate) noexcept
    {
        return 1.0f - std::exp (-1.0f / (tauSeconds * (float) sampleRate));
    }

    inline float midiToHz (float note) noexcept
    {
        return 440.0f * std::exp2 ((note - 69.0f) * (1.0f / 12.0f));
    }
}

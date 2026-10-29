#pragma once

#include "DspUtils.h"

namespace octocid
{
    //==========================================================================
    // Filtre passe-bas 4 pôles « diode ladder » de la bassline acid,
    // discrétisé en ZDF/TPT (intégrateurs trapézoïdaux, boucle résolue sans délai).
    //
    // Modèle linéarisé des 4 condensateurs couplés (le dernier vaut C/2) :
    //   y1' = wc (u  - 2 y1 + y2)
    //   y2' = wc (y1 - 2 y2 + y3)
    //   y3' = wc (y2 - 2 y3 + y4)
    //   y4' = 2 wc (y3 - y4)
    // Cette topologie a exactement la même fonction de transfert que le modèle
    // « TB_303 » de mystran & kunn utilisé par Open303 (vérifié numériquement) :
    // auto-oscillation à k = 17, pic de résonance à sqrt(2) * wc.
    //
    // Le filtre est quasi linéaire, comme l'original : la résonance reste
    // « liquide ». Un limiteur doux n'intervient qu'à fort niveau pour garder la
    // résonance maximale sous contrôle. Passe-haut 150 Hz dans la réaction :
    // la résonance s'affaiblit dans le grave (valeur Open303).
    //==========================================================================
    class DiodeLadder
    {
    public:
        static constexpr float kSelfOsc = 17.0f;

        void prepare (double sampleRate) noexcept
        {
            fs = (float) sampleRate;
            hpG = std::tan (kPi * 150.0f / fs);
            maxWcHz = 0.42f * fs * 0.70710678f;
            reset();
        }

        void reset() noexcept
        {
            for (auto& v : s) v = 0.0f;
            hpS = 0.0f;
        }

        // x : entrée (~±1), cutoffHz : fréquence du pic, k : 0..17, makeup : gain de sortie
        float process (float x, float cutoffHz, float k, float makeup) noexcept
        {
            const float wcHz = std::min (cutoffHz * 0.70710678f, maxWcHz);
            const float g = std::tan (kPi * wcHz / fs);

            // Système tridiagonal M y = s + g e1 u, avec M = I - g A
            const float b = 1.0f + 2.0f * g;
            const float a2 = -g, a3 = -g, a4 = -2.0f * g;
            const float c1 = -g, c2 = -g, c3 = -g;

            // Élimination de Thomas, deux seconds membres : P (états) et Q (entrée u)
            const float m1 = 1.0f / b;
            const float cp1 = c1 * m1;
            const float dP1 = s[0] * m1, dQ1 = g * m1;

            const float m2 = 1.0f / (b - a2 * cp1);
            const float cp2 = c2 * m2;
            const float dP2 = (s[1] - a2 * dP1) * m2, dQ2 = (-a2 * dQ1) * m2;

            const float m3 = 1.0f / (b - a3 * cp2);
            const float cp3 = c3 * m3;
            const float dP3 = (s[2] - a3 * dP2) * m3, dQ3 = (-a3 * dQ2) * m3;

            const float m4 = 1.0f / (b - a4 * cp3);
            const float P4 = (s[3] - a4 * dP3) * m4, Q4 = (-a4 * dQ3) * m4;
            const float P3 = dP3 - cp3 * P4,        Q3 = dQ3 - cp3 * Q4;
            const float P2 = dP2 - cp2 * P3,        Q2 = dQ2 - cp2 * Q3;
            const float P1 = dP1 - cp1 * P2,        Q1 = dQ1 - cp1 * Q2;

            // Passe-haut TPT dans la réaction : hp = ha * y4 + hc
            const float ha = 1.0f / (1.0f + hpG);
            const float hc = -hpS * ha;

            // Résolution exacte de la boucle linéaire, puis limiteur doux (inactif à niveau normal)
            const float uLin = (x - k * (ha * P4 + hc)) / (1.0f + k * ha * Q4);
            const float u = kLimit * fastTanh (uLin * (1.0f / kLimit));

            const float y1 = P1 + Q1 * u;
            const float y2 = P2 + Q2 * u;
            const float y3 = P3 + Q3 * u;
            const float y4 = P4 + Q4 * u;

            s[0] = 2.0f * y1 - s[0];
            s[1] = 2.0f * y2 - s[1];
            s[2] = 2.0f * y3 - s[2];
            s[3] = 2.0f * y4 - s[3];

            const float lp = (hpG * y4 + hpS) * ha;
            hpS = 2.0f * lp - hpS;

            return y4 * makeup;
        }

    private:
        static constexpr float kLimit = 2.5f;

        float fs = 44100.0f;
        float s[4] {};
        float hpG = 0.0f, hpS = 0.0f;
        float maxWcHz = 8000.0f;
    };
}

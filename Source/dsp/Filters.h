#pragma once

#include "DspUtils.h"

namespace octocid
{
    //==========================================================================
    // Petits filtres utilitaires (TPT = précis même à très basse fréquence
    // relative, donc utilisables à la fréquence suréchantillonnée).
    //==========================================================================

    // Passe-haut 1 pôle TPT
    class OnePoleHighpass
    {
    public:
        void prepare (double sampleRate, float cutoffHz) noexcept
        {
            const float g = std::tan (kPi * cutoffHz / (float) sampleRate);
            G = g / (1.0f + g);
            s = 0.0f;
        }
        void reset() noexcept { s = 0.0f; }
        float process (float x) noexcept
        {
            const float v = (x - s) * G;
            const float lp = v + s;
            s = lp + v;
            return x - lp;
        }
    private:
        float G = 0.0f, s = 0.0f;
    };

    // Passe-bas 2 pôles (SVF TPT), Q = 0.707 : Butterworth
    class SvfLowpass
    {
    public:
        void prepare (double sampleRate, float cutoffHz, float q = 0.70710678f) noexcept
        {
            g = std::tan (kPi * cutoffHz / (float) sampleRate);
            k = 1.0f / q;
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
            ic1 = ic2 = 0.0f;
        }
        void reset (float value = 0.0f) noexcept { ic1 = 0.0f; ic2 = value; }
        float process (float x) noexcept
        {
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            return v2;
        }
    private:
        float g = 0.0f, k = 1.414f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f, ic1 = 0.0f, ic2 = 0.0f;
    };

    //==========================================================================
    // Façonnage du grave en sortie, à la fréquence de l'hôte (double précision,
    // car ces filtres travaillent à quelques Hz). Valeurs mesurées par le projet
    // Open303 (c) 2009 Robin Schmidt, licence MIT :
    //   passe-tout 14 Hz, passe-haut 24 Hz, coupe-bande 7,5 Hz (largeur 4,7 oct).
    //==========================================================================
    class ToneShaper
    {
    public:
        void prepare (double sampleRate) noexcept
        {
            const double twoPi = 6.283185307179586;

            // Passe-haut 1 pôle 24,167 Hz
            const double xh = std::exp (-twoPi * 24.167 / sampleRate);
            hpB0 = 0.5 * (1.0 + xh); hpB1 = -hpB0; hpA1 = xh;

            // Passe-tout 1 pôle 14,008 Hz
            const double t = std::tan (3.141592653589793 * 14.008 / sampleRate);
            apC = (t - 1.0) / (t + 1.0);

            // Coupe-bande RBJ 7,5164 Hz, largeur 4,7 octaves
            const double w = twoPi * 7.5164 / sampleRate;
            const double sn = std::sin (w), cs = std::cos (w);
            const double alpha = sn * std::sinh (0.5 * std::log (2.0) * 4.7 * w / sn);
            const double sc = 1.0 / (1.0 + alpha);
            nB0 = sc; nB1 = -2.0 * cs * sc; nB2 = sc;
            nA1 = 2.0 * cs * sc; nA2 = (alpha - 1.0) * sc;
            reset();
        }

        void reset() noexcept { hpX = hpY = apX = apY = nX1 = nX2 = nY1 = nY2 = 0.0; }

        float process (float in) noexcept
        {
            double x = in;

            const double ap = apC * x + apX - apC * apY;      // passe-tout
            apX = x; apY = ap; x = ap;

            const double hp = hpB0 * x + hpB1 * hpX + hpA1 * hpY;   // passe-haut
            hpX = x; hpY = hp; x = hp;

            const double n = nB0 * x + nB1 * nX1 + nB2 * nX2 + nA1 * nY1 + nA2 * nY2;  // coupe-bande
            nX2 = nX1; nX1 = x; nY2 = nY1; nY1 = n;

            return (float) n;
        }

    private:
        double hpB0 = 1, hpB1 = 0, hpA1 = 0, hpX = 0, hpY = 0;
        double apC = 0, apX = 0, apY = 0;
        double nB0 = 1, nB1 = 0, nB2 = 0, nA1 = 0, nA2 = 0, nX1 = 0, nX2 = 0, nY1 = 0, nY2 = 0;
    };
}

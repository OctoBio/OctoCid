#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>

//==============================================================================
// LFO de modulation (assignable à n'importe quel potard).
//
// Inspiré de ce que les utilisateurs demandent sur les forums (Vital, Bitwig,
// Arturia, Sonic Charge) :
//   * modes de départ : Libre, Note (repart à chaque note détachée), Synchro
//     (calé sur la position du DAW : identique à chaque lecture, repart quand on
//     boucle ou qu'on déplace la lecture), Un coup (un seul cycle, comme une
//     enveloppe), Aléatoire (phase au hasard à chaque note) ;
//   * phase de départ, délai PUIS fondu d'entrée ;
//   * lissage continu : il ne repart jamais de zéro quand le LFO redémarre
//     (pas de saut, contrairement au défaut signalé sur Vital) ;
//   * bipolaire / unipolaire ; 7 formes dont deux aléatoires.
// Calculé par tranches (64 échantillons) : sortie dans [-1, 1] (ou [0, 1]).
//==============================================================================
namespace octocid
{
    namespace ModSync
    {
        // Durée d'un cycle en noires (0 = libre)
        inline constexpr int count = 17;
        inline const char* names[count] = { "Hz", "4 mes.", "2 mes.", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
                                            "1/2.", "1/4.", "1/8.", "1/16.", "1/2T", "1/4T", "1/8T", "1/16T" };
        inline constexpr double beats[count] = { 0.0, 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
                                                 3.0, 1.5, 0.75, 0.375, 4.0 / 3.0, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0 };
    }

    class ModLfo
    {
    public:
        enum Shape { Sine, Triangle, SawUp, SawDown, Square, SampleHold, SmoothRandom, Custom };
        enum Mode  { Free, NoteRetrig, TransportSync, OneShot, RandomPhase };

        struct Settings
        {
            int shape = Sine, mode = Free, sync = 0;
            float rateHz = 2.0f, phase01 = 0.0f, delaySec = 0.0f, fadeSec = 0.0f, smooth01 = 0.0f;
            bool unipolar = false;
            const float* table = nullptr;   // forme dessinée (1024 valeurs -1..1) pour Custom
            int tableSize = 0;
        };

        void reset (uint32_t seed) noexcept
        {
            rng = seed * 2654435761u + 12345u;
            phase = 0.0; sinceNote = 1.0e9; out = 0.0f; held = false;
            prevRand = nextRandom(); curRand = nextRandom(); lastCycle = 0;
        }

        // Une note détachée vient de commencer
        void noteOn (const Settings& s) noexcept
        {
            sinceNote = 0.0;
            if (s.mode == NoteRetrig || s.mode == OneShot) { phase = s.phase01; held = false; }
            else if (s.mode == RandomPhase) phase = uniform();
        }

        // Avance d'une tranche ; ppq / bpm / playing : position du DAW au début de la tranche
        float advance (const Settings& s, int numSamples, double fs, double ppq, double bpm, bool playing) noexcept
        {
            const double dt = numSamples / fs;
            const double beats = s.sync > 0 && s.sync < ModSync::count ? ModSync::beats[s.sync] : 0.0;
            const double hz = beats > 0.0 ? bpm / 60.0 / beats : (double) std::max (0.001f, s.rateHz);

            double p;
            if (s.mode == TransportSync && playing)
            {
                // phase déduite de la position : même résultat à chaque lecture, repart en boucle
                const double cycles = beats > 0.0 ? ppq / beats : ppq * 60.0 / bpm * hz;
                p = cycles - std::floor (cycles) + s.phase01;
                phase = p;
            }
            else
            {
                if (s.mode == OneShot && held) p = s.phase01 + 0.999999;   // tient la valeur de fin de cycle
                else
                {
                    phase += hz * dt;
                    if (s.mode == OneShot && phase - s.phase01 >= 1.0) { held = true; phase = s.phase01 + 0.999999; }
                    p = phase + (s.mode == Free ? s.phase01 : 0.0);
                }
            }
            if (phase > 1.0e6) phase -= std::floor (phase);

            const double cyc = std::floor (p);
            const float x = (float) (p - cyc);                 // position dans le cycle 0..1
            pos = x;
            if ((long long) cyc != lastCycle) { lastCycle = (long long) cyc; prevRand = curRand; curRand = nextRandom(); }

            float v;   // -1..1
            switch (s.shape)
            {
                case Triangle:     v = x < 0.5f ? 4.0f * x - 1.0f : 3.0f - 4.0f * x; break;
                case SawUp:        v = 2.0f * x - 1.0f; break;
                case SawDown:      v = 1.0f - 2.0f * x; break;
                case Square:       v = x < 0.5f ? 1.0f : -1.0f; break;
                case SampleHold:   v = curRand; break;
                case Custom:
                    if (s.table != nullptr && s.tableSize > 1)
                    {
                        const float fi = x * (float) s.tableSize;
                        const int i0 = std::min ((int) fi, s.tableSize - 1);
                        const int i1 = (i0 + 1) % s.tableSize;
                        v = s.table[i0] + (s.table[i1] - s.table[i0]) * (fi - (float) i0);
                    }
                    else v = 0.0f;
                    break;
                case SmoothRandom: { const float t = 0.5f - 0.5f * std::cos (3.14159265f * x); v = prevRand + (curRand - prevRand) * t; break; }
                default:           v = std::sin (6.28318531f * x); break;
            }
            if (s.unipolar) v = 0.5f * (v + 1.0f);

            // délai puis fondu, comptés depuis la dernière note détachée
            float amp = 1.0f;
            if (s.delaySec > 0.0f || s.fadeSec > 0.0f)
            {
                const double t = sinceNote - s.delaySec;
                amp = t <= 0.0 ? 0.0f : (s.fadeSec > 0.0f ? (float) std::min (1.0, t / s.fadeSec) : 1.0f);
            }
            sinceNote += dt;
            v *= amp;

            // lissage : one-pole continu (jamais remis à zéro)
            if (s.smooth01 > 0.001f)
            {
                const double tau = 0.002 + 0.4 * s.smooth01 * s.smooth01;
                out += (v - out) * (float) (1.0 - std::exp (-dt / tau));
            }
            else out = v;
            return out;
        }

        float value() const noexcept { return out; }
        float cyclePosition() const noexcept { return pos; }   // position affichée (phase comprise)

    private:
        float uniform() noexcept { rng = rng * 1664525u + 1013904223u; return (float) (rng >> 8) / 16777216.0f; }
        float nextRandom() noexcept { return 2.0f * uniform() - 1.0f; }

        double phase = 0.0, sinceNote = 1.0e9;
        float out = 0.0f, pos = 0.0f, prevRand = 0.0f, curRand = 0.0f;
        long long lastCycle = 0;
        uint32_t rng = 1;
        bool held = false;
    };
}

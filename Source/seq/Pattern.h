#pragma once

#include <cstdint>
#include <algorithm>

namespace octocid
{
    //==========================================================================
    // Un pas de séquenceur, façon bassline acid :
    //  note    : 0..11 (Do..Si)
    //  octave  : -1 (Down), 0, +1 (Up)
    //  accent, slide (le slide glisse vers le pas suivant)
    //  gate    : Note (joue), Rest (silence), Tie (prolonge la note précédente)
    //==========================================================================
    enum class Gate : uint8_t { Note = 0, Rest = 1, Tie = 2 };

    struct Step
    {
        uint8_t note = 0;
        int8_t  octave = 0;
        bool    accent = false;
        bool    slide = false;
        Gate    gate = Gate::Note;

        bool operator== (const Step& o) const noexcept
        {
            return note == o.note && octave == o.octave && accent == o.accent && slide == o.slide && gate == o.gate;
        }
    };

    struct Pattern
    {
        static constexpr int kMaxSteps = 32;
        Step steps[kMaxSteps] {};
        int length = 16;

        bool operator== (const Pattern& o) const noexcept
        {
            if (length != o.length) return false;
            for (int i = 0; i < kMaxSteps; ++i)
                if (! (steps[i] == o.steps[i])) return false;
            return true;
        }
    };

    struct PatternBank
    {
        static constexpr int kNumPatterns = 8;    // A..H
        Pattern patterns[kNumPatterns] {};
    };

    //==========================================================================
    // Gammes pour le Randomize (intervalles en demi-tons depuis la tonique)
    //==========================================================================
    struct Scale { const char* name; int count; int degrees[12]; };

    inline const Scale kScales[] =
    {
        { "Mineure",       7, { 0, 2, 3, 5, 7, 8, 10 } },
        { "Phrygien",      7, { 0, 1, 3, 5, 7, 8, 10 } },
        { "Dorien",        7, { 0, 2, 3, 5, 7, 9, 10 } },
        { "Mineure harm.", 7, { 0, 2, 3, 5, 7, 8, 11 } },
        { "Penta mineure", 5, { 0, 3, 5, 7, 10 } },
        { "Majeure",       7, { 0, 2, 4, 5, 7, 9, 11 } },
        { "Mixolydien",    7, { 0, 2, 4, 5, 7, 9, 10 } },
        { "Chromatique",  12, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } },
    };
    inline constexpr int kNumScales = (int) (sizeof (kScales) / sizeof (kScales[0]));

    //==========================================================================
    // Outils d'édition (fil de l'interface uniquement)
    //==========================================================================
    namespace PatternTools
    {
        // Générateur pseudo-aléatoire simple et reproductible
        struct Rng
        {
            uint32_t s;
            explicit Rng (uint32_t seed) : s (seed ? seed : 0x9e3779b9u) {}
            uint32_t next() noexcept { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
            float unit() noexcept { return (float) (next() >> 8) * (1.0f / 16777216.0f); }
            int below (int n) noexcept { return (int) (unit() * (float) n) % std::max (1, n); }
        };

        // Randomize avec verrouillage de gamme. Densités 0..1.
        inline void randomize (Pattern& p, int root, int scaleIndex, float accentDensity, float slideDensity,
                               float restDensity, uint32_t seed)
        {
            Rng rng (seed);
            const Scale& sc = kScales[std::clamp (scaleIndex, 0, kNumScales - 1)];
            for (int i = 0; i < p.length; ++i)
            {
                Step s;
                // La tonique revient souvent : c'est ce qui rend une ligne acid « accrocheuse »
                const int degree = rng.unit() < 0.35f ? 0 : sc.degrees[rng.below (sc.count)];
                s.note = (uint8_t) ((root + degree) % 12);
                const float o = rng.unit();
                s.octave = (int8_t) (o < 0.15f ? -1 : (o > 0.80f ? 1 : 0));
                s.accent = rng.unit() < accentDensity;
                s.slide  = rng.unit() < slideDensity;
                const float g = rng.unit();
                s.gate = i == 0 ? Gate::Note : (g < restDensity ? Gate::Rest : (g < restDensity + 0.06f ? Gate::Tie : Gate::Note));
                p.steps[i] = s;
            }
        }

        inline void shift (Pattern& p, int dir) noexcept
        {
            const int n = std::max (1, p.length);
            Step tmp[Pattern::kMaxSteps];
            for (int i = 0; i < n; ++i)
                tmp[((i + dir) % n + n) % n] = p.steps[i];
            for (int i = 0; i < n; ++i)
                p.steps[i] = tmp[i];
        }

        // Transposition d'un demi-ton ; l'octave suit, bornée à Down..Up
        inline void transpose (Pattern& p, int semis) noexcept
        {
            for (auto& s : p.steps)
            {
                int abs = s.octave * 12 + s.note + semis;
                abs = std::clamp (abs, -12, 23);
                s.octave = (int8_t) ((abs + 12) / 12 - 1);
                s.note = (uint8_t) ((abs + 12) % 12);
            }
        }

        inline void clear (Pattern& p) noexcept
        {
            const int len = p.length;
            p = Pattern{};
            p.length = len;
            for (auto& s : p.steps) s.gate = Gate::Rest;
        }

        //======================================================================
        // Patterns de démo (A..H), une ligne acid différente par case.
        // Notation : note, octave, accent, slide, gate ('n' note, '-' rest, 't' tie)
        //======================================================================
        struct Def { int note, oct; bool acc, sld; char gate; };

        inline Pattern make (const Def* d, int n)
        {
            Pattern p;
            p.length = n;
            for (int i = 0; i < n; ++i)
            {
                p.steps[i].note = (uint8_t) d[i].note;
                p.steps[i].octave = (int8_t) d[i].oct;
                p.steps[i].accent = d[i].acc;
                p.steps[i].slide = d[i].sld;
                p.steps[i].gate = d[i].gate == '-' ? Gate::Rest : (d[i].gate == 't' ? Gate::Tie : Gate::Note);
            }
            return p;
        }

        inline PatternBank demoBank()
        {
            PatternBank b;
            // A : ligne classique en Do mineur
            const Def a[16] = { {0,0,1,0,'n'},{0,0,0,0,'n'},{0,1,0,1,'n'},{10,0,0,0,'n'},{0,0,1,0,'n'},{0,0,0,0,'-'},{3,0,0,1,'n'},{5,0,1,0,'n'},
                                {0,0,0,0,'n'},{0,1,1,1,'n'},{3,1,1,0,'n'},{0,0,0,0,'n'},{7,0,0,1,'n'},{0,0,1,0,'n'},{10,0,1,0,'n'},{0,1,0,0,'n'} };
            // B : rebond d'octaves
            const Def bb[16] = { {0,-1,1,0,'n'},{0,0,0,0,'n'},{0,-1,0,0,'n'},{0,0,1,0,'n'},{0,-1,0,1,'n'},{0,0,0,0,'n'},{3,0,1,0,'n'},{0,-1,0,0,'n'},
                                 {0,0,0,0,'n'},{0,-1,1,0,'n'},{0,0,0,1,'n'},{7,0,0,0,'n'},{0,-1,0,0,'n'},{0,0,1,0,'n'},{10,-1,0,1,'n'},{0,0,0,0,'n'} };
            // C : phrygien qui serpente
            const Def c[16] = { {0,0,1,0,'n'},{1,0,0,1,'n'},{0,0,0,0,'n'},{0,0,0,0,'-'},{7,0,1,0,'n'},{8,0,0,1,'n'},{7,0,0,0,'n'},{0,0,0,0,'n'},
                                {0,1,1,1,'n'},{1,1,0,0,'n'},{0,0,0,0,'n'},{0,0,0,0,'t'},{3,0,1,0,'n'},{1,0,0,1,'n'},{0,0,0,0,'n'},{0,0,1,0,'n'} };
            // D : glides longs
            const Def d[16] = { {0,0,1,1,'n'},{7,0,0,1,'n'},{0,1,0,0,'n'},{0,0,0,0,'t'},{10,0,1,1,'n'},{3,0,0,0,'n'},{0,0,0,0,'t'},{0,0,0,0,'-'},
                                {5,0,1,1,'n'},{0,1,0,1,'n'},{10,0,0,0,'n'},{0,0,0,0,'t'},{7,0,1,1,'n'},{3,1,0,0,'n'},{0,0,0,0,'t'},{0,0,0,0,'-'} };
            // E : techno martelée (accents denses)
            const Def e[16] = { {0,0,1,0,'n'},{0,0,1,0,'n'},{0,1,0,0,'n'},{0,0,1,0,'n'},{3,0,1,0,'n'},{0,0,0,0,'n'},{0,1,1,0,'n'},{0,0,1,0,'n'},
                                {0,0,1,0,'n'},{10,-1,0,0,'n'},{0,0,1,0,'n'},{0,1,1,1,'n'},{0,0,1,0,'n'},{7,0,0,0,'n'},{0,0,1,0,'n'},{3,0,1,0,'n'} };
            // F : syncopes et silences
            const Def f[16] = { {0,0,1,0,'n'},{0,0,0,0,'-'},{0,0,0,0,'-'},{7,0,0,1,'n'},{0,1,1,0,'n'},{0,0,0,0,'-'},{3,0,0,0,'n'},{0,0,0,0,'-'},
                                {0,0,1,0,'n'},{0,0,0,0,'-'},{10,0,0,1,'n'},{0,1,1,0,'n'},{0,0,0,0,'-'},{5,0,0,0,'n'},{3,0,1,1,'n'},{0,0,0,0,'n'} };
            // G : lead aigu (dorien)
            const Def g[16] = { {0,1,1,0,'n'},{2,1,0,1,'n'},{3,1,0,0,'n'},{0,1,0,0,'n'},{9,0,1,0,'n'},{10,0,0,1,'n'},{0,1,1,0,'n'},{0,1,0,0,'t'},
                                {7,0,1,0,'n'},{9,0,0,1,'n'},{10,0,0,0,'n'},{2,1,1,0,'n'},{3,1,0,1,'n'},{2,1,0,0,'n'},{0,1,1,0,'n'},{10,0,0,0,'n'} };
            // H : basse ronde lente (rap / trap)
            const Def h[16] = { {0,-1,1,0,'n'},{0,0,0,0,'t'},{0,0,0,0,'t'},{0,0,0,0,'-'},{0,-1,0,1,'n'},{3,-1,0,0,'n'},{0,0,0,0,'t'},{0,0,0,0,'-'},
                                {7,-1,1,0,'n'},{0,0,0,0,'t'},{5,-1,0,1,'n'},{3,-1,0,0,'n'},{0,0,0,0,'t'},{0,0,0,0,'-'},{10,-1,0,1,'n'},{0,-1,0,0,'n'} };
            b.patterns[0] = make (a, 16);  b.patterns[1] = make (bb, 16);
            b.patterns[2] = make (c, 16);  b.patterns[3] = make (d, 16);
            b.patterns[4] = make (e, 16);  b.patterns[5] = make (f, 16);
            b.patterns[6] = make (g, 16);  b.patterns[7] = make (h, 16);
            return b;
        }
    }
}

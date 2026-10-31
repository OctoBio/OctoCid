#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

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

        //======================================================================
        // GÉNÉRATEUR façon bassline acid (inspiré du principe de TB-3PO) :
        // densité bipolaire -7..+7.
        //   -7 : quasi que la tonique, beaucoup de notes (ligne martelée)
        //    0 : toute la gamme, ligne aérée
        //   +7 : toute la gamme, ligne pleine
        // Règles « 303 » : la tonique domine, premier pas joué et accentué,
        // accents plutôt sur le temps et les contretemps, slides seulement vers
        // une note différente, octave haute surtout sur les accents.
        // Densités d'accents / slides 0..1 (0,5 = dosage classique).
        //======================================================================
        inline void generate (Pattern& p, int root, int scaleIndex, int density,
                              float accentAmount, float slideAmount, uint32_t seed)
        {
            Rng rng (seed);
            const Scale& sc = kScales[std::clamp (scaleIndex, 0, kNumScales - 1)];
            const int d = std::clamp (density, -7, 7);
            const float gateProb = 0.55f + 0.38f * (float) std::abs (d) / 7.0f;
            const float poolFrac = std::clamp ((float) (d + 7) / 7.0f, 0.0f, 1.0f);
            const int pool = 1 + (int) std::lround (poolFrac * (float) (sc.count - 1));

            // Degrés disponibles : la tonique, puis la quinte et la tierce en priorité
            int order[12] {};
            int n = 0;
            auto addDegree = [&] (int semis)
            {
                for (int i = 0; i < sc.count && n < pool; ++i)
                    if (sc.degrees[i] == semis)
                    {
                        for (int j = 0; j < n; ++j) if (order[j] == semis) return;
                        order[n++] = semis;
                    }
            };
            addDegree (0); addDegree (7); addDegree (3); addDegree (4); addDegree (10); addDegree (5);
            for (int i = 0; i < sc.count && n < pool; ++i) addDegree (sc.degrees[i]);

            const int len = std::max (1, p.length);
            for (int i = 0; i < len; ++i)
            {
                Step s;
                const bool downbeat = i % 4 == 0, offbeat = i % 4 == 2;
                const float gp = i == 0 ? 1.0f : gateProb + (downbeat ? 0.1f : 0.0f);
                const float g = rng.unit();
                s.gate = g < gp ? Gate::Note : (g < gp + 0.07f && i > 0 ? Gate::Tie : Gate::Rest);

                const int deg = (rng.unit() < 0.42f || n == 1) ? 0 : order[1 + rng.below (std::max (1, n - 1))];
                s.note = (uint8_t) ((root + deg) % 12);

                float acc = 0.18f + (i == 0 ? 0.5f : 0.0f) + (offbeat ? 0.17f : 0.0f) + (downbeat ? 0.1f : 0.0f);
                acc *= 2.0f * std::clamp (accentAmount, 0.0f, 1.0f);
                s.accent = s.gate == Gate::Note && rng.unit() < acc;

                const float o = rng.unit();
                s.octave = (int8_t) (o < (s.accent ? 0.28f : 0.12f) ? 1 : (o > 0.92f && deg == 0 ? -1 : 0));

                s.slide = rng.unit() < 0.22f * 2.0f * std::clamp (slideAmount, 0.0f, 1.0f);
                p.steps[i] = s;
            }

            // Règles de propreté : un slide doit mener à une note différente
            for (int i = 0; i < len; ++i)
            {
                auto& s = p.steps[i];
                const auto& nx = p.steps[(i + 1) % len];
                if (s.slide && (nx.gate != Gate::Note || (nx.note == s.note && nx.octave == s.octave)))
                    s.slide = false;
                if (s.gate == Gate::Rest)
                    s.slide = s.accent = false;
            }
        }

        // Garde l'ancien nom (densités directes) pour compatibilité
        inline void randomize (Pattern& p, int root, int scaleIndex, float accentDensity, float slideDensity,
                               float /*restDensity*/, uint32_t seed)
        {
            generate (p, root, scaleIndex, 2, accentDensity, slideDensity, seed);
        }

        //======================================================================
        // Lignes indépendantes : décaler seulement les notes, les accents, les
        // slides ou les gates. Décaler les accents d'un pas change souvent tout.
        //======================================================================
        enum class Lane { Notes, Accents, Slides, Gates };

        inline void shiftLane (Pattern& p, Lane lane, int dir) noexcept
        {
            const int n = std::max (1, p.length);
            Step tmp[Pattern::kMaxSteps];
            for (int i = 0; i < n; ++i)
                tmp[((i + dir) % n + n) % n] = p.steps[i];
            for (int i = 0; i < n; ++i)
            {
                switch (lane)
                {
                    case Lane::Notes:   p.steps[i].note = tmp[i].note; p.steps[i].octave = tmp[i].octave; break;
                    case Lane::Accents: p.steps[i].accent = tmp[i].accent; break;
                    case Lane::Slides:  p.steps[i].slide = tmp[i].slide; break;
                    case Lane::Gates:   p.steps[i].gate = tmp[i].gate; break;
                    default: break;
                }
            }
        }

        // Ramène chaque note sur la gamme choisie (degré le plus proche)
        inline void quantize (Pattern& p, int root, int scaleIndex) noexcept
        {
            const Scale& sc = kScales[std::clamp (scaleIndex, 0, kNumScales - 1)];
            for (auto& s : p.steps)
            {
                const int rel = ((int) s.note - root + 12) % 12;
                int best = 0, bestDist = 99;
                for (int i = 0; i < sc.count; ++i)
                {
                    const int dd = std::abs (sc.degrees[i] - rel);
                    const int dist = std::min (dd, 12 - dd);
                    if (dist < bestDist) { bestDist = dist; best = sc.degrees[i]; }
                }
                s.note = (uint8_t) ((root + best) % 12);
            }
        }

        //======================================================================
        // VARIER : petites transformations d'un pattern existant (la bonne
        // méthode d'après les développeurs d'émulations 303 : faire évoluer
        // une ligne plutôt qu'en tirer une nouvelle au hasard).
        // amount 0..1 : nombre et ampleur des transformations.
        //======================================================================
        inline void mutate (Pattern& p, int root, int scaleIndex, float amount, uint32_t seed)
        {
            Rng rng (seed);
            const Scale& sc = kScales[std::clamp (scaleIndex, 0, kNumScales - 1)];
            const int len = std::max (1, p.length);
            const int ops = 1 + (int) std::lround (std::clamp (amount, 0.0f, 1.0f) * 3.0f);

            for (int k = 0; k < ops; ++k)
            {
                switch (rng.below (7))
                {
                    case 0: shiftLane (p, Lane::Accents, rng.unit() < 0.5f ? -1 : 1); break;
                    case 1: shiftLane (p, Lane::Slides,  rng.unit() < 0.5f ? -1 : 1); break;
                    case 2:   // inverse l'ordre des notes, garde le rythme
                    {
                        for (int i = 0; i < len / 2; ++i)
                        {
                            std::swap (p.steps[i].note, p.steps[len - 1 - i].note);
                            std::swap (p.steps[i].octave, p.steps[len - 1 - i].octave);
                        }
                        break;
                    }
                    case 3:   // échange des moitiés (notes seulement)
                    {
                        const int h = len / 2;
                        for (int i = 0; i < h; ++i)
                        {
                            std::swap (p.steps[i].note, p.steps[i + h].note);
                            std::swap (p.steps[i].octave, p.steps[i + h].octave);
                        }
                        break;
                    }
                    case 4:   // quelques notes changent, dans la gamme
                    {
                        const int changes = 1 + (int) (amount * (float) len / 6.0f);
                        for (int c = 0; c < changes; ++c)
                        {
                            auto& s = p.steps[rng.below (len)];
                            s.note = (uint8_t) ((root + sc.degrees[rng.below (sc.count)]) % 12);
                        }
                        break;
                    }
                    case 5:   // une octave bascule
                    {
                        auto& s = p.steps[rng.below (len)];
                        s.octave = (int8_t) (s.octave == 0 ? (rng.unit() < 0.7f ? 1 : -1) : 0);
                        break;
                    }
                    default:  // un accent ou un slide apparaît / disparaît
                    {
                        auto& s = p.steps[rng.below (len)];
                        if (rng.unit() < 0.5f) s.accent = ! s.accent; else s.slide = ! s.slide;
                        break;
                    }
                }
            }

            for (int i = 0; i < len; ++i)
            {
                auto& s = p.steps[i];
                const auto& nx = p.steps[(i + 1) % len];
                if (s.slide && nx.gate != Gate::Note && nx.gate != Gate::Tie)
                    s.slide = false;
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

#pragma once

#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <juce_core/juce_core.h>

//==============================================================================
// Forme de LFO dessinée (façon Serum / Vital) : des points reliés par des segments
// courbés. Chaque point porte la courbure du segment qui le suit.
//  - x : 0..1 (premier point à 0, dernier à 1, non déplaçables en x)
//  - y : 0..1 (1 = haut)
//  - curve : -1..1 (0 = droit ; > 0 bombé vers le haut, < 0 creusé vers le bas)
// Deux points peuvent avoir le même x : marche verticale (portes, escaliers).
// Le moteur lit une table de 1024 valeurs calculée à chaque modification.
//==============================================================================
namespace octocid
{
    struct LfoShape
    {
        static constexpr int maxPoints = 64;
        static constexpr int tableSize = 1024;
        struct Point { float x = 0.0f, y = 0.0f, curve = 0.0f; };

        std::array<Point, maxPoints> pts {};
        int count = 0;

        LfoShape()   // sinus par défaut (sans passer par preset(), qui construit des LfoShape)
        {
            constexpr float b = 0.29f;
            pts[0] = { 0.0f, 0.5f, b }; pts[1] = { 0.25f, 1.0f, b }; pts[2] = { 0.5f, 0.5f, -b };
            pts[3] = { 0.75f, 0.0f, -b }; pts[4] = { 1.0f, 0.5f, 0.0f };
            count = 5;
        }

        // courbure -> déformation du segment (bombé vers le haut quand curve > 0)
        static float warp (float t, float curve, bool rising) noexcept
        {
            if (std::abs (curve) < 1.0e-4f) return t;
            const float e = std::exp2 ((rising ? -curve : curve) * 3.5f);
            return std::pow (juce::jlimit (0.0f, 1.0f, t), e);
        }

        float eval (float x) const noexcept
        {
            if (count <= 0) return 0.5f;
            if (x <= pts[0].x) return pts[0].y;
            for (int i = 0; i < count - 1; ++i)
            {
                const auto& a = pts[(size_t) i];
                const auto& b = pts[(size_t) i + 1];
                if (x <= b.x)
                {
                    if (b.x - a.x < 1.0e-6f) return b.y;
                    const float t = (x - a.x) / (b.x - a.x);
                    return a.y + (b.y - a.y) * warp (t, a.curve, b.y > a.y);
                }
            }
            return pts[(size_t) count - 1].y;
        }

        // table bipolaire -1..1
        void render (float* table) const noexcept
        {
            for (int i = 0; i < tableSize; ++i)
                table[i] = 2.0f * eval ((float) i / (float) tableSize) - 1.0f;
        }

        void sortAndClamp() noexcept
        {
            std::stable_sort (pts.begin(), pts.begin() + count, [] (const Point& a, const Point& b) { return a.x < b.x; });
            for (int i = 0; i < count; ++i)
            {
                auto& p = pts[(size_t) i];
                p.x = juce::jlimit (0.0f, 1.0f, p.x);
                p.y = juce::jlimit (0.0f, 1.0f, p.y);
                p.curve = juce::jlimit (-1.0f, 1.0f, p.curve);
            }
            if (count > 0) { pts[0].x = 0.0f; pts[(size_t) count - 1].x = 1.0f; }
        }

        int add (float x, float y, float curve = 0.0f)
        {
            if (count >= maxPoints) return -1;
            int at = count;
            for (int i = 0; i < count; ++i) if (pts[(size_t) i].x > x) { at = i; break; }
            if (at == 0) at = 1;                      // jamais avant le premier point
            if (at >= count) at = count - 1;          // ni après le dernier
            for (int i = count; i > at; --i) pts[(size_t) i] = pts[(size_t) i - 1];
            pts[(size_t) at] = { x, y, curve };
            ++count;
            return at;
        }

        void remove (int i)
        {
            if (i <= 0 || i >= count - 1) return;     // les deux bouts restent
            for (int k = i; k < count - 1; ++k) pts[(size_t) k] = pts[(size_t) k + 1];
            --count;
        }

        void flipVertical() noexcept { for (int i = 0; i < count; ++i) { pts[(size_t) i].y = 1.0f - pts[(size_t) i].y; pts[(size_t) i].curve = -pts[(size_t) i].curve; } }

        void mirror()
        {
            LfoShape m; m.count = count;
            for (int i = 0; i < count; ++i)
            {
                const auto& p = pts[(size_t) (count - 1 - i)];
                m.pts[(size_t) i] = { 1.0f - p.x, p.y, 0.0f };
            }
            // la courbure d'un segment passe au segment miroir, inversée dans le temps
            for (int i = 0; i < count - 1; ++i)
                m.pts[(size_t) i].curve = -pts[(size_t) (count - 2 - i)].curve;
            m.sortAndClamp();
            *this = m;
        }

        // Pinceau : remplace les cases de la grille [c0, c1] par des marches (ou des rampes descendantes)
        void paintSteps (int gridX, const std::vector<float>& cells, bool ramps)
        {
            LfoShape s; s.count = 0;
            const int n = juce::jlimit (1, maxPoints / 2, gridX);
            for (int i = 0; i < n; ++i)
            {
                const float v = cells[(size_t) i], x0 = (float) i / (float) n, x1 = (float) (i + 1) / (float) n;
                s.pts[(size_t) s.count++] = { x0, v, ramps ? 0.35f : 0.0f };
                s.pts[(size_t) s.count++] = { x1, ramps ? 0.0f : v, 0.0f };
            }
            *this = s;
            sortAndClamp();
        }

        //--------------------------------------------------------------------------
        juce::String toString() const
        {
            juce::String s;
            for (int i = 0; i < count; ++i)
            {
                const auto& p = pts[(size_t) i];
                s << juce::String (p.x, 4) << " " << juce::String (p.y, 4) << " " << juce::String (p.curve, 3);
                if (i < count - 1) s << ";";
            }
            return s;
        }

        static bool fromString (const juce::String& text, LfoShape& out)
        {
            LfoShape s; s.count = 0;
            for (auto& tok : juce::StringArray::fromTokens (text, ";", ""))
            {
                auto v = juce::StringArray::fromTokens (tok.trim(), " ", "");
                if (v.size() < 2 || s.count >= maxPoints) continue;
                s.pts[(size_t) s.count++] = { v[0].getFloatValue(), v[1].getFloatValue(), v.size() > 2 ? v[2].getFloatValue() : 0.0f };
            }
            if (s.count < 2) return false;
            s.sortAndClamp();
            out = s;
            return true;
        }

        //--------------------------------------------------------------------------
        // Formes prêtes
        static constexpr int numPresets = 16;
        static const char* presetName (int i)
        {
            static const char* n[numPresets] = { "Sinus", "Triangle", "Scie montante", "Scie descendante", "Carré",
                                                 "Pompe (sidechain)", "Rampe expo", "Rebond", "Montée lente", "Escalier 4",
                                                 "Escalier 8", "Porte trance 1/16", "Wobble", "Acid stab", "Double bosse", "Vague asymétrique" };
            return n[juce::jlimit (0, numPresets - 1, i)];
        }

        static LfoShape fromList (std::initializer_list<Point> l)
        {
            LfoShape s; s.count = 0;
            for (auto& p : l) if (s.count < maxPoints) s.pts[(size_t) s.count++] = p;
            s.sortAndClamp();
            return s;
        }

        static LfoShape preset (int i)
        {
            constexpr float b = 0.29f;   // courbure qui imite un quart de sinus (t^0.5)
            switch (i)
            {
                case 1:  return fromList ({ { 0, 0 }, { 0.5f, 1 }, { 1, 0 } });
                case 2:  return fromList ({ { 0, 0 }, { 1, 1 } });
                case 3:  return fromList ({ { 0, 1 }, { 1, 0 } });
                case 4:  return fromList ({ { 0, 1 }, { 0.5f, 1 }, { 0.5f, 0 }, { 1, 0 } });
                case 5:  return fromList ({ { 0, 0, 0.45f }, { 0.7f, 1 }, { 1, 1 } });
                case 6:  return fromList ({ { 0, 1, -0.8f }, { 1, 0 } });
                case 7:  return fromList ({ { 0, 1, -0.6f }, { 0.35f, 0, 0.6f }, { 0.55f, 0.55f, -0.6f }, { 0.75f, 0, 0.6f }, { 0.87f, 0.25f, -0.6f }, { 1, 0 } });
                case 8:  return fromList ({ { 0, 0, -0.5f }, { 1, 1 } });
                case 9:  return fromList ({ { 0, 0 }, { 0.25f, 0 }, { 0.25f, 0.33f }, { 0.5f, 0.33f }, { 0.5f, 0.66f }, { 0.75f, 0.66f }, { 0.75f, 1 }, { 1, 1 } });
                case 10:
                {
                    LfoShape s; s.count = 0;
                    for (int k = 0; k < 8; ++k)
                    {
                        const float v = (float) k / 7.0f;
                        s.pts[(size_t) s.count++] = { (float) k / 8.0f, v };
                        s.pts[(size_t) s.count++] = { (float) (k + 1) / 8.0f, v };
                    }
                    s.sortAndClamp();
                    return s;
                }
                case 11:
                {
                    static const int gate[16] = { 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0 };
                    LfoShape s; s.count = 0;
                    for (int k = 0; k < 16; ++k)
                    {
                        const float v = gate[k] ? 1.0f : 0.0f;
                        s.pts[(size_t) s.count++] = { (float) k / 16.0f, v };
                        s.pts[(size_t) s.count++] = { (float) (k + 1) / 16.0f, v };
                    }
                    s.sortAndClamp();
                    return s;
                }
                case 12: return fromList ({ { 0, 0.5f, b }, { 0.125f, 1, b }, { 0.25f, 0.5f, -b }, { 0.375f, 0, -b }, { 0.5f, 0.5f, b },
                                            { 0.625f, 0.85f, b }, { 0.75f, 0.5f, -b }, { 0.875f, 0.15f, -b }, { 1, 0.5f } });
                case 13: return fromList ({ { 0, 1, -0.9f }, { 0.4f, 0.1f }, { 1, 0 } });
                case 14: return fromList ({ { 0, 0, b }, { 0.25f, 1, b }, { 0.5f, 0, b }, { 0.75f, 0.7f, b }, { 1, 0 } });
                case 15: return fromList ({ { 0, 0.5f, b }, { 0.15f, 1, -0.3f }, { 0.6f, 0, 0.4f }, { 1, 0.5f } });
                default: return fromList ({ { 0, 0.5f, b }, { 0.25f, 1, b }, { 0.5f, 0.5f, -b }, { 0.75f, 0, -b }, { 1, 0.5f } });
            }
        }
    };
}

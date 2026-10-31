#pragma once

#include <JuceHeader.h>

//==============================================================================
// Palette active de l'interface. Trois variantes à comparer avant de figer ;
// le choix est une préférence de l'utilisateur (commune à toutes les instances).
//==============================================================================
namespace OctoColours
{
    inline juce::Colour background { 0xff2b2826 };   // fond
    inline juce::Colour panel      { 0xff36312d };   // panneaux
    inline juce::Colour panelHi    { 0xff433d38 };   // relief des panneaux, capuchons
    inline juce::Colour cream      { 0xffefe6d2 };   // éléments (texte, potards)
    inline juce::Colour slime      { 0xff9be22d };   // couleur d'accent unique
    inline juce::Colour dim        { 0xff6b655e };   // éléments éteints
    inline juce::Colour ink        { 0xff1c1a18 };   // contours « cartoon »

    struct Theme
    {
        const char* name;
        juce::uint32 background, panel, panelHi, cream, slime, dim, ink;
    };

    inline const Theme themes[] =
    {
        // 1. Slime : anthracite chaud, crème, vert bave (proposition de départ)
        { "Slime (anthracite chaud)", 0xff2b2826, 0xff36312d, 0xff433d38, 0xffefe6d2, 0xff9be22d, 0xff6b655e, 0xff1c1a18 },
        // 2. Néon : gris-bleu nuit, blanc cassé froid, vert acide fluo
        { "Neon (nuit froide)",       0xff1d2023, 0xff272b2f, 0xff32373c, 0xffe8ecee, 0xffb4ff39, 0xff5d656c, 0xff111315 },
        // 3. Papier : clair façon BD, encre anthracite, vert profond
        { "Papier (BD claire)",       0xffe8dfcb, 0xffdcd1b9, 0xffcfc3a8, 0xff2b2826, 0xff5aa61a, 0xff9a8f7a, 0xff2b2826 },
        // 4. Jaune acid house
        { "Jaune acide",              0xfff2cf1d, 0xfff6eedb, 0xffebe0c4, 0xff2b2826, 0xff3e9b16, 0xffa39a80, 0xff2b2826 },
        // 5. Nuit acide : fond sombre, panneaux papier, vert néon
        { "Nuit acide",               0xff1f2420, 0xffefe6cf, 0xffe2d7bb, 0xff2b2826, 0xff58b11b, 0xff9a8f7a, 0xff1a1917 },
        // 6. Vert acide : fond vert vif, panneaux papier
        { "Vert acide",               0xff9ad62a, 0xfff3ecd8, 0xffe6dcc2, 0xff2b2826, 0xff3f8f12, 0xff9a8f7a, 0xff2b2826 },
    };
    inline constexpr int numThemes = (int) (sizeof (themes) / sizeof (themes[0]));
    inline int currentTheme = 0;

    inline void applyTheme (int index)
    {
        currentTheme = juce::jlimit (0, numThemes - 1, index);
        const auto& t = themes[currentTheme];
        background = juce::Colour (t.background);
        panel      = juce::Colour (t.panel);
        panelHi    = juce::Colour (t.panelHi);
        cream      = juce::Colour (t.cream);
        slime      = juce::Colour (t.slime);
        dim        = juce::Colour (t.dim);
        ink        = juce::Colour (t.ink);
    }

    // Texte posé sur la couleur d'accent (lisible dans les trois palettes)
    inline juce::Colour onSlime() { return currentTheme >= 2 ? juce::Colour (0xfff4efe2) : background; }
}

//==============================================================================
// Préférences de l'utilisateur (palette, zoom), communes à toutes les instances
//==============================================================================
namespace OctoPrefs
{
    // Lecture / écriture à la demande (pas d'objet statique : déchargement du plugin sans risque)
    inline juce::PropertiesFile::Options options()
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "OctoCid";
        o.filenameSuffix = ".settings";
        o.folderName = "OctoBio";
        o.osxLibrarySubFolder = "Application Support";
        return o;
    }

    inline int getInt (const juce::String& key, int fallback)
    {
        juce::PropertiesFile f (options());
        return f.getIntValue (key, fallback);
    }

    inline void setInt (const juce::String& key, int value)
    {
        juce::PropertiesFile f (options());
        f.setValue (key, value);
        f.saveIfNeeded();
    }
}

//==============================================================================
// Style général de l'interface (arrondis, contours, ombres, titres, potards).
// Plusieurs directions à comparer ; le choix est une préférence utilisateur.
//==============================================================================
namespace OctoLook
{
    struct Look
    {
        const char* name;
        int   knob;       // variante de potard « tête de poulet » (KnobStyles::chicken)
        float panelR;     // arrondi des panneaux
        float ctrlR;      // arrondi des boutons / menus / sélecteurs
        float outline;    // épaisseur des contours
        int   shadow;     // 0 douce, 1 décalée « impression », 2 aucune
        bool  screws;     // vis aux coins des panneaux (façon rack)
        int   title;      // 0 simple, 1 étiquette pleine, 2 gravé souligné, 3 bulle, 4 épuré + filet, 5 pastille
        float alpha = -1.0f;        // opacité des panneaux (-1 = celle du décor)
        float seqAlpha = -1.0f;     // opacité du panneau séquenceur
        float outlineAlpha = 1.0f;  // opacité des contours
        bool  lightOutline = false; // contour clair (verre) au lieu de l'encre
    };

    inline const Look looks[] =
    {
        { "Atelier (impression)", 1,  6.0f, 4.0f, 2.0f, 1, false, 1 },
        { "Rack vintage",         3,  2.0f, 3.0f, 1.5f, 2, true,  2 },
        { "BD bulles",            5, 16.0f, 10.0f, 2.5f, 1, false, 3 },
        { "Epure A - verre clair",    5, 6.0f, 5.0f, 1.0f, 2, false, 4, 0.50f, 0.86f, 0.55f, true  },
        { "Epure B - trait d'encre",  5, 4.0f, 4.0f, 1.2f, 2, false, 4, 0.58f, 0.88f, 0.70f, false },
        { "Epure C - sans contour",   5, 8.0f, 6.0f, 0.0f, 0, false, 5, 0.45f, 0.86f, 0.0f,  false },
        { "Epure D - BD allege",      5, 8.0f, 7.0f, 2.0f, 1, false, 3, 0.50f, 0.88f, 1.0f,  false },
        { "Epure E - tres transparent", 5, 5.0f, 4.0f, 1.0f, 2, false, 4, 0.32f, 0.80f, 0.6f, true  },
    };
    inline constexpr int numLooks = (int) (sizeof (looks) / sizeof (looks[0]));
    inline int current = 2;
    inline const Look& get() { return looks[juce::jlimit (0, numLooks - 1, current)]; }
    inline void apply (int index) { current = juce::jlimit (0, numLooks - 1, index); }

    // Rectangle arrondi avec l'ombre et le contour du style courant
    inline void shape (juce::Graphics& g, juce::Rectangle<float> r, float radius, juce::Colour fill, float outlineScale = 1.0f,
                       const juce::Point<float>* frostOrigin = nullptr);
    inline void shapeImpl (juce::Graphics& g, juce::Rectangle<float> r, float radius, juce::Colour fill, float outlineScale,
                           const juce::Point<float>* frostOrigin, bool (*frostFn) (juce::Graphics&, const juce::Path&, juce::Point<float>))
    {
        using namespace OctoColours;
        const auto& L = get();
        if (L.shadow == 0)
        {
            g.setColour (ink.withAlpha (0.4f));
            g.fillRoundedRectangle (r.translated (0.0f, 2.0f), radius);
        }
        else if (L.shadow == 1)
        {
            g.setColour (ink);
            g.fillRoundedRectangle (r.translated (2.5f * outlineScale, 2.5f * outlineScale), radius);
        }
        if (frostOrigin != nullptr && frostFn != nullptr)
        {
            juce::Path p; p.addRoundedRectangle (r, radius);
            frostFn (g, p, *frostOrigin);
        }
        g.setColour (fill);
        g.fillRoundedRectangle (r, radius);
        if (L.outline > 0.0f && L.outlineAlpha > 0.0f)
        {
            g.setColour ((L.lightOutline ? juce::Colour (0xfffff8ec) : ink).withAlpha (L.outlineAlpha));
            g.drawRoundedRectangle (r, radius, L.outline * juce::jmax (1.0f, outlineScale));
        }
    }
}

//==============================================================================
// Décor de fond (éclairs, trames, rayures) : donne du caractère « acid »
//==============================================================================
namespace OctoDecor
{
    struct Decor
    {
        const char* name;
        int   theme;       // palette associée
        int   bolts;       // 0 aucun, 1 vert, 2 encre, 3 néon lumineux, 4 crème
        bool  dots;        // trame de points
        bool  stripes;     // rayures diagonales
        bool  speed;       // lignes de vitesse horizontales
        float panelAlpha;  // opacité des panneaux (le décor transparaît)
        const char* image = nullptr;   // fond illustré (bolts == 6)
    };

    // Fond illustré et sa version floutée (verre dépoli sous les panneaux)
    inline juce::Image bgImage, bgBlur;

    inline const Decor decors[] =
    {
        { "Papier + eclairs",         2, 1, false, false, false, 1.0f  },
        { "Papier + eclairs + trame", 2, 1, true,  false, false, 0.93f },
        { "Jaune acide + eclairs",    3, 2, false, false, false, 1.0f  },
        { "Vert acide + rayures",     5, 4, false, true,  false, 0.95f },
        { "Nuit acide + neon",        4, 3, false, false, false, 1.0f  },
        { "Papier + vitesse",         2, 1, false, false, true,  0.95f },
        { "Papier + grand eclair",    2, 5, false, false, false, 0.84f },
        { "Jaune + grand eclair",     3, 5, true,  false, false, 0.86f },
        { "Acide liquide",            2, 6, false, false, false, 0.62f, "acide" },
        { "Orage acide",              2, 6, false, false, false, 0.70f, "orage" },
        { "Acide + orage",            2, 6, false, false, false, 0.64f, "acide_orage" },
        { "Portail violet",           2, 6, false, false, false, 0.66f, "portail_violet" },
    };
    inline constexpr int numDecors = (int) (sizeof (decors) / sizeof (decors[0]));
    inline constexpr int defaultDecor = 11;
    inline int current = defaultDecor;
    inline const Decor& get() { return decors[juce::jlimit (0, numDecors - 1, current)]; }
    inline void apply (int index) { current = juce::jlimit (0, numDecors - 1, index); }

    // Éclair horizontal en zigzag, de gauche à droite
    inline juce::Path bolt (float x0, float x1, float y, float amp, int seed)
    {
        juce::Random rnd (seed);
        juce::Path p;
        p.startNewSubPath (x0, y);
        float x = x0; bool up = true;
        while (x < x1)
        {
            x += 26.0f + rnd.nextFloat() * 46.0f;
            const float yy = y + (up ? -1.0f : 1.0f) * amp * (0.5f + rnd.nextFloat() * 0.6f);
            p.lineTo (juce::jmin (x, x1), yy);
            up = ! up;
        }
        return p;
    }

    // Dessine le décor sur toute la zone de l'éditeur ; « area » est en coordonnées éditeur,
    // « origin » décale pour un composant enfant (tiroir).
    inline void paint (juce::Graphics& g, juce::Rectangle<float> area, juce::Point<float> origin = {})
    {
        using namespace OctoColours;
        const auto& D = get();
        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::translation (-origin.x, -origin.y));
        const auto paper = juce::Colour (0xfff6efdf);

        if (D.bolts == 6 && bgImage.isValid())
        {
            g.drawImage (bgImage, { 0.0f, 0.0f, 1000.0f, 620.0f }, juce::RectanglePlacement::stretchToFit);
            return;
        }
        if (D.stripes)
        {
            g.setColour (juce::Colours::white.withAlpha (0.16f));
            for (float x = -area.getHeight(); x < area.getRight(); x += 34.0f)
            {
                juce::Path st;
                st.addQuadrilateral (x, area.getBottom(), x + 14.0f, area.getBottom(),
                                     x + 14.0f + area.getHeight(), area.getY(), x + area.getHeight(), area.getY());
                g.fillPath (st);
            }
        }
        if (D.dots)
        {
            g.setColour (ink.withAlpha (0.10f));
            for (float y = area.getY(); y < area.getBottom(); y += 9.0f)
                for (float x = area.getX() + (std::fmod (y, 18.0f) < 9.0f ? 0.0f : 4.5f); x < area.getRight(); x += 9.0f)
                    g.fillEllipse (x, y, 2.6f, 2.6f);
        }
        if (D.speed)
        {
            juce::Random rnd (7);
            for (int i = 0; i < 40; ++i)
            {
                const float y = area.getY() + rnd.nextFloat() * area.getHeight();
                const float x = area.getX() + rnd.nextFloat() * area.getWidth() * 0.7f;
                const float len = 60.0f + rnd.nextFloat() * 260.0f;
                g.setColour ((i % 3 == 0 ? slime : ink).withAlpha (0.18f));
                g.fillRoundedRectangle (x, y, len, 2.0f + rnd.nextFloat() * 2.5f, 1.5f);
            }
        }
        if (D.bolts == 5)
        {
            // Un grand éclair qui traverse toute l'interface, visible à travers les panneaux
            juce::Path p;
            p.startNewSubPath (area.getX() - 20.0f, 150.0f);
            const float pts[][2] = { {170, 300}, {150, 250}, {380, 420}, {350, 360}, {610, 520}, {580, 440}, {820, 600}, {790, 520}, {1030, 640} };
            for (auto& q : pts) p.lineTo (q[0], q[1] - 40.0f);
            p.lineTo (1030.0f, 560.0f);
            for (int i = 8; i >= 0; --i) p.lineTo (pts[i][0] + 10.0f, pts[i][1] - 120.0f + (i % 2 ? 10.0f : 0.0f));
            p.lineTo (area.getX() - 20.0f, 60.0f);
            p.closeSubPath();
            g.setColour (ink.withAlpha (0.8f));
            g.fillPath (p, juce::AffineTransform::translation (6.0f, 6.0f));
            g.setColour (slime);
            g.fillPath (p);
            g.setColour (ink);
            g.strokePath (p, juce::PathStrokeType (3.0f));
            // petits éclairs dans la barre du haut
            auto b = bolt (area.getX() - 10.0f, area.getRight() + 10.0f, 26.0f, 9.0f, 31);
            g.setColour (slime); g.strokePath (b, juce::PathStrokeType (3.0f));
        }
        else if (D.bolts > 0)
        {
            const float ys[] = { 26.0f, 210.0f, 342.0f, 612.0f };
            const float amps[] = { 9.0f, 5.0f, 5.0f, 5.0f };
            for (int i = 0; i < 4; ++i)
            {
                auto p = bolt (area.getX() - 10.0f, area.getRight() + 10.0f, ys[i], amps[i], 31 + i * 17);
                const juce::PathStrokeType thick (8.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::square);
                const juce::PathStrokeType thin (4.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::square);
                switch (D.bolts)
                {
                    case 1: g.setColour (ink.withAlpha (0.55f)); g.strokePath (p, thick); g.setColour (slime); g.strokePath (p, thin); break;
                    case 2: g.setColour (ink); g.strokePath (p, thin); break;
                    case 3:
                        g.setColour (slime.withAlpha (0.18f)); g.strokePath (p, juce::PathStrokeType (16.0f));
                        g.setColour (slime.withAlpha (0.35f)); g.strokePath (p, juce::PathStrokeType (9.0f));
                        g.setColour (slime.brighter (0.6f)); g.strokePath (p, juce::PathStrokeType (3.0f));
                        break;
                    default: g.setColour (ink); g.strokePath (p, thick); g.setColour (paper); g.strokePath (p, thin); break;
                }
            }
        }
    }
}

namespace OctoDecor
{
    // Verre dépoli : dessine le fond flouté dans la forme donnée (coordonnées du composant)
    inline bool frost (juce::Graphics& g, const juce::Path& shape, juce::Point<float> origin)
    {
        if (get().bolts != 6 || ! bgBlur.isValid()) return false;
        juce::Graphics::ScopedSaveState ss (g);
        const auto xf = juce::AffineTransform::scale (1000.0f / (float) bgBlur.getWidth(), 620.0f / (float) bgBlur.getHeight())
                            .translated (-origin.x, -origin.y);
        g.setFillType (juce::FillType (bgBlur, xf));
        g.fillPath (shape);
        return true;
    }
}

inline void OctoLook::shape (juce::Graphics& g, juce::Rectangle<float> r, float radius, juce::Colour fill, float outlineScale,
                             const juce::Point<float>* frostOrigin)
{
    shapeImpl (g, r, radius, fill, outlineScale, frostOrigin, &OctoDecor::frost);
}

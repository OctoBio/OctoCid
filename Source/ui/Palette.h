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
    inline juce::Colour onSlime() { return currentTheme == 2 ? juce::Colour (0xfff4efe2) : background; }
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
        int   title;      // 0 simple, 1 étiquette pleine, 2 gravé souligné, 3 bulle
    };

    inline const Look looks[] =
    {
        { "Atelier (impression)", 1,  6.0f, 4.0f, 2.0f, 1, false, 1 },
        { "Rack vintage",         3,  2.0f, 3.0f, 1.5f, 2, true,  2 },
        { "BD bulles",            5, 16.0f, 10.0f, 2.5f, 1, false, 3 },
    };
    inline constexpr int numLooks = (int) (sizeof (looks) / sizeof (looks[0]));
    inline int current = 0;
    inline const Look& get() { return looks[juce::jlimit (0, numLooks - 1, current)]; }
    inline void apply (int index) { current = juce::jlimit (0, numLooks - 1, index); }

    // Rectangle arrondi avec l'ombre et le contour du style courant
    inline void shape (juce::Graphics& g, juce::Rectangle<float> r, float radius, juce::Colour fill, float outlineScale = 1.0f)
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
        g.setColour (fill);
        g.fillRoundedRectangle (r, radius);
        g.setColour (ink);
        g.drawRoundedRectangle (r, radius, L.outline * outlineScale);
    }
}

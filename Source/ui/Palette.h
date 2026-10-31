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

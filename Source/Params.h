#pragma once

#include <JuceHeader.h>

//==============================================================================
// Identifiants des paramètres (version 1).
// Ne JAMAIS renommer un ID existant : ça casse l'automation des projets.
//==============================================================================
namespace ParamIDs
{
    // Panneau principal
    inline const juce::ParameterID tune      { "tune",      1 };
    inline const juce::ParameterID cutoff    { "cutoff",    1 };
    inline const juce::ParameterID resonance { "resonance", 1 };
    inline const juce::ParameterID envMod    { "envmod",    1 };
    inline const juce::ParameterID decay     { "decay",     1 };
    inline const juce::ParameterID accent    { "accent",    1 };
    inline const juce::ParameterID waveform  { "waveform",  1 };
    inline const juce::ParameterID volume    { "volume",    1 };

    inline const juce::ParameterID accentThreshold { "accentthr", 1 };
    inline const juce::ParameterID quality         { "quality",   1 };

    // Slime Mods (Devil Fish)
    inline const juce::ParameterID slideTime   { "slidetime",   1 };
    inline const juce::ParameterID sweepSpeed  { "sweepspeed",  1 };
    inline const juce::ParameterID slidePunch  { "slidepunch",  1 };
    inline const juce::ParameterID accentDecay { "accentdecay", 1 };
    inline const juce::ParameterID softAttack  { "softattack",  1 };
    inline const juce::ParameterID filterTrack { "filtertrack", 1 };
    inline const juce::ParameterID filterFm    { "filterfm",    1 };
    inline const juce::ParameterID filterDrive { "filterdrive", 1 };
    inline const juce::ParameterID subOsc      { "subosc",      1 };
    inline const juce::ParameterID subLevel    { "sublevel",    1 };
    inline const juce::ParameterID muffler     { "muffler",     1 };
    inline const juce::ParameterID analog      { "analog",      1 };

    // Drive
    inline const juce::ParameterID driveType { "drivetype", 1 };
    inline const juce::ParameterID driveAmt  { "drive",     1 };
    inline const juce::ParameterID driveTone { "drivetone", 1 };
    inline const juce::ParameterID driveMix  { "drivemix",  1 };
    inline const juce::ParameterID drivePos  { "drivepos",  1 };

    // Punch + FX
    inline const juce::ParameterID punch       { "punch",       1 };
    inline const juce::ParameterID punchAttack { "punchattack", 1 };
    inline const juce::ParameterID delayMix    { "dlymix",      1 };
    inline const juce::ParameterID delayTime   { "dlytime",     1 };
    inline const juce::ParameterID delayFb     { "dlyfb",       1 };
    inline const juce::ParameterID reverbMix   { "revmix",      1 };
    inline const juce::ParameterID reverbDecay { "revdecay",    1 };
    inline const juce::ParameterID reverbTone  { "revtone",     1 };

    // Séquenceur
    inline const juce::ParameterID seqOn      { "seqon",      1 };
    inline const juce::ParameterID seqPattern { "seqpattern", 1 };
    inline const juce::ParameterID seqRes     { "seqres",     1 };
    inline const juce::ParameterID seqSwing   { "seqswing",   1 };
    inline const juce::ParameterID seqGate    { "seqgate",    1 };

    // Interface
    inline const juce::ParameterID snailAnim  { "snailanim",  1 };

    // LFO
    inline const juce::ParameterID lfoRate  { "lforate",  1 };
    inline const juce::ParameterID lfoSync  { "lfosync",  1 };
    inline const juce::ParameterID lfoDepth { "lfodepth", 1 };
    inline const juce::ParameterID lfoShape { "lfoshape", 1 };
    inline const juce::ParameterID lfoDest  { "lfodest",  1 };

    // Effets « percutant / transcendant »
    inline const juce::ParameterID impact   { "impact",   1 };
    inline const juce::ParameterID smash    { "smash",    1 };
    inline const juce::ParameterID jetMix   { "jetmix",   1 };
    inline const juce::ParameterID jetMode  { "jetmode",  1 };
    inline const juce::ParameterID jetRate  { "jetrate",  1 };
    inline const juce::ParameterID jetFb    { "jetfb",    1 };
    inline const juce::ParameterID pump     { "pump",     1 };
    inline const juce::ParameterID pumpRate { "pumprate", 1 };
    inline const juce::ParameterID width    { "width",    1 };
}

namespace JetSync
{
    // Durée d'un cycle de Jet, en noires
    inline const juce::StringArray names { "4 mes.", "2 mes.", "1 mes.", "1/2", "1/4", "1/8" };
    inline constexpr double beats[] = { 16.0, 8.0, 4.0, 2.0, 1.0, 0.5 };
}

namespace PumpSync
{
    inline const juce::StringArray names { "1/4", "1/8", "1/2", "1/4." };
    inline constexpr double beats[] = { 1.0, 0.5, 2.0, 1.5 };
}

namespace SeqRes
{
    inline const juce::StringArray names { "1/16", "1/8", "1/32", "1/16T", "1/8T" };
    inline constexpr double beats[] = { 0.25, 0.5, 0.125, 1.0 / 6.0, 1.0 / 3.0 };
}

namespace DelaySync
{
    inline const juce::StringArray names { "1/16", "1/8T", "1/8", "1/8.", "1/4T", "1/4", "1/4.", "1/2" };
    inline constexpr double beats[] = { 0.25, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0 };
}

namespace LfoSync
{
    // Durée d'un cycle en noires (0 = libre)
    inline const juce::StringArray names { "Libre", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
                                           "1/4.", "1/8.", "1/16.", "1/4T", "1/8T", "1/16T" };
    inline constexpr double beats[] = { 0.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
                                        1.5, 0.75, 0.375, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0 };
}

//==============================================================================
// Modulation : 3 LFO assignables et 8 emplacements (source -> destination, quantité)
//==============================================================================
namespace ModParams
{
    inline constexpr int numLfos = 3, numSlots = 8;

    inline juce::String lfoId (int lfo, const char* what) { return "l" + juce::String (lfo + 1) + what; }   // ex. "l1rate"
    inline juce::String slotId (int slot, const char* what) { return "m" + juce::String (slot + 1) + what; } // ex. "m1amt"

    inline const juce::StringArray shapeNames { "Sinus", "Triangle", "Scie +", "Scie -", "Carre", "S&H", "Aleatoire", "Dessin" };
    inline const juce::StringArray modeNames  { "Libre", "Note", "Synchro", "Un coup", "Aleatoire" };
    inline const juce::StringArray sourceNames { "-", "LFO 1", "LFO 2", "LFO 3" };

    // Destinations : tous les potards (les sélecteurs à choix ne sont pas modulables)
    struct Dest { const char* id; const char* name; };
    inline const Dest dests[] = {
        { "",            "-" },
        { "tune",        "Tune" },        { "cutoff",      "Cutoff" },       { "resonance",   "Resonance" },
        { "envmod",      "Env Mod" },     { "decay",       "Decay" },        { "accent",      "Accent" },
        { "volume",      "Volume" },
        { "slidetime",   "Slide Time" },  { "slidepunch",  "Slide Punch" },  { "accentdecay", "Acc. Decay" },
        { "softattack",  "Soft Attack" }, { "filtertrack", "F. Track" },     { "filterfm",    "Filter FM" },
        { "filterdrive", "F. Drive" },    { "sublevel",    "Sub Level" },    { "analog",      "Analog" },
        { "drive",       "Drive" },       { "drivetone",   "Drive Tone" },   { "drivemix",    "Drive Mix" },
        { "punch",       "Punch" },       { "punchattack", "Punch Attaque" },
        { "dlymix",      "Delay" },       { "dlyfb",       "Delay Fb" },
        { "revmix",      "Reverb" },      { "revdecay",    "Rev. Decay" },   { "revtone",     "Rev. Tone" },
        { "impact",      "Impact" },      { "smash",       "Smash" },        { "jetmix",      "Jet Mix" },
        { "jetfb",       "Jet Fb" },      { "pump",        "Pump" },         { "width",       "Width" },
        { "l1rate",      "LFO 1 Vitesse" }, { "l2rate",    "LFO 2 Vitesse" }, { "l3rate",     "LFO 3 Vitesse" },
        { "seqgate",     "Seq Gate" },
    };
    inline constexpr int numDests = (int) (sizeof (dests) / sizeof (dests[0]));
    inline juce::StringArray destNames()
    {
        juce::StringArray a;
        for (auto& d : dests) a.add (juce::String::fromUTF8 (d.name));
        return a;
    }
    inline int destIndex (const juce::String& paramId)
    {
        for (int i = 1; i < numDests; ++i) if (paramId == dests[i].id) return i;
        return 0;
    }
}


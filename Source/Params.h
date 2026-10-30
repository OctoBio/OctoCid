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

    // Drive
    inline const juce::ParameterID driveType { "drivetype", 1 };
    inline const juce::ParameterID driveAmt  { "drive",     1 };
    inline const juce::ParameterID driveTone { "drivetone", 1 };
    inline const juce::ParameterID driveMix  { "drivemix",  1 };
    inline const juce::ParameterID drivePos  { "drivepos",  1 };

    // LFO
    inline const juce::ParameterID lfoRate  { "lforate",  1 };
    inline const juce::ParameterID lfoSync  { "lfosync",  1 };
    inline const juce::ParameterID lfoDepth { "lfodepth", 1 };
    inline const juce::ParameterID lfoShape { "lfoshape", 1 };
    inline const juce::ParameterID lfoDest  { "lfodest",  1 };
}

namespace LfoSync
{
    // Durée d'un cycle en noires (0 = libre)
    inline const juce::StringArray names { "Libre", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
                                           "1/4.", "1/8.", "1/16.", "1/4T", "1/8T", "1/16T" };
    inline constexpr double beats[] = { 0.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
                                        1.5, 0.75, 0.375, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0 };
}

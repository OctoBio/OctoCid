#pragma once

#include <JuceHeader.h>

//==============================================================================
// Identifiants des paramètres (version 1).
// Ne JAMAIS renommer un ID existant : ça casse l'automation des projets.
//==============================================================================
namespace ParamIDs
{
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
}

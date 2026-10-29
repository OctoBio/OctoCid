#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ParamIDs
{
    // Version 1 des IDs : ne JAMAIS les renommer ensuite (casse l'automation des projets)
    static const juce::ParameterID tune      { "tune",      1 };
    static const juce::ParameterID cutoff    { "cutoff",    1 };
    static const juce::ParameterID resonance { "resonance", 1 };
    static const juce::ParameterID envMod    { "envmod",    1 };
    static const juce::ParameterID decay     { "decay",     1 };
    static const juce::ParameterID accent    { "accent",    1 };
    static const juce::ParameterID waveform  { "waveform",  1 };
    static const juce::ParameterID volume    { "volume",    1 };
}

//==============================================================================
OctoCidAudioProcessor::OctoCidAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "OctoCidState", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout OctoCidAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = AudioParameterFloatAttributes().withLabel ("%");

    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::tune, "Tune",
        NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("st")));

    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::cutoff,    "Cutoff",    NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::resonance, "Resonance", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::envMod,    "Env Mod",   NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::decay,     "Decay",     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::accent,    "Accent",    NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, pct));

    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::waveform, "Waveform",
        StringArray { "Saw", "Square" }, 0));

    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::volume, "Volume",
        NormalisableRange<float> (-48.0f, 6.0f, 0.1f), -6.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    return layout;
}

//==============================================================================
void OctoCidAudioProcessor::prepareToPlay (double, int)
{
    // Étape 2 : préparation du moteur (oversampling, smoothers, voix...)
}

bool OctoCidAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void OctoCidAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // Aucune allocation, aucun verrou : on lit juste le MIDI pour l'indicateur.
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            lastNote.store (msg.getNoteNumber(), std::memory_order_relaxed);
        else if (msg.isNoteOff() && msg.getNoteNumber() == lastNote.load (std::memory_order_relaxed))
            lastNote.store (-1, std::memory_order_relaxed);
    }

    // Squelette : sortie silencieuse (le moteur sonore arrive à l'étape 2)
    buffer.clear();
}

//==============================================================================
juce::AudioProcessorEditor* OctoCidAudioProcessor::createEditor()
{
    return new OctoCidAudioProcessorEditor (*this);
}

//==============================================================================
void OctoCidAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OctoCidAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OctoCidAudioProcessor();
}

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Éditeur provisoire (étape 1) : fenêtre à la taille finale (1000 x 620),
// fond anthracite, titre et indicateur MIDI. La vraie interface arrive à l'étape 5.
//==============================================================================
class OctoCidAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit OctoCidAudioProcessorEditor (OctoCidAudioProcessor&);
    ~OctoCidAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    void timerCallback() override;

    OctoCidAudioProcessor& processor;
    int displayedNote = -1;
    juce::Rectangle<int> ledArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctoCidAudioProcessorEditor)
};

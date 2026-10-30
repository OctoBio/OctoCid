#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Éditeur provisoire (étape 2) : potards JUCE standards pour régler le son
// pendant la mise au point du moteur. La vraie interface arrive à l'étape 5.
//==============================================================================
class OctoCidAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit OctoCidAudioProcessorEditor (OctoCidAudioProcessor&);
    ~OctoCidAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Choice
    {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    void setupKnob (Knob&, const juce::ParameterID&, const juce::String& name);
    void setupChoice (Choice&, const juce::ParameterID&, const juce::String& name);

    OctoCidAudioProcessor& processor;
    juce::LookAndFeel_V4 lookAndFeel;

    Knob tune, cutoff, resonance, envMod, decay, accent, volume, accentThr;
    Knob slideTime, slidePunch;
    Choice waveform, quality, sweepSpeed;

    int displayedNote = -1;
    bool displayedAccent = false;
    juce::Rectangle<int> ledArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctoCidAudioProcessorEditor)
};

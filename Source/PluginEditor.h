#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/SequencerView.h"

//==============================================================================
// Éditeur provisoire : potards JUCE standards, rangés par sections, pour régler
// le son pendant la mise au point. La vraie interface arrive à l'étape 5.
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

    // Un contrôle = un libellé + un potard ou un menu
    struct Control
    {
        juce::Label label;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> box;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> boxAttachment;
    };

    struct Section
    {
        juce::String title;
        juce::Rectangle<int> bounds;
        bool slime = false;
        std::vector<std::vector<Control*>> rows;
    };

    Control* knob (const juce::ParameterID&, const juce::String& name);
    Control* choice (const juce::ParameterID&, const juce::String& name);
    void layoutRow (juce::Rectangle<int> area, const std::vector<Control*>& row);
    void refreshPresetBox();
    void setPage (bool sequencer);

    OctoCidAudioProcessor& processor;
    juce::LookAndFeel_V4 lookAndFeel;

    std::vector<std::unique_ptr<Control>> controls;
    std::vector<Section> sections;

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::Label presetInfo;
    juce::TextButton soundTab { "SON" }, seqTab { "SEQUENCEUR" };
    std::unique_ptr<SequencerView> seqView;
    bool showSeq = false;

    int displayedNote = -1;
    bool displayedAccent = false;
    int displayedProgram = -1;
    juce::Rectangle<int> ledArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctoCidAudioProcessorEditor)
};

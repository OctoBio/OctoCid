#pragma once

#include <JuceHeader.h>

//==============================================================================
// OctoCid — étape 1 : squelette.
// Le processeur expose déjà l'APVTS (paramètres du panneau principal) pour que
// l'automation et la sauvegarde d'état soient en place dès le départ.
// Le DSP (osc, filtre diode ladder, enveloppes...) arrive à l'étape 2.
//==============================================================================
class OctoCidAudioProcessor : public juce::AudioProcessor
{
public:
    OctoCidAudioProcessor();
    ~OctoCidAudioProcessor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==========================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Init"; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    // Indicateur MIDI pour vérifier que les notes arrivent (lu par l'UI, sans verrou)
    std::atomic<int> lastNote { -1 };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctoCidAudioProcessor)
};

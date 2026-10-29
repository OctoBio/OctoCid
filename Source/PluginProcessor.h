#pragma once

#include <JuceHeader.h>
#include "Params.h"
#include "dsp/AcidVoice.h"

//==============================================================================
// OctoCid — étape 2 : moteur sonore.
// Toute la voix (oscillateur, enveloppes, filtre) tourne à la fréquence
// suréchantillonnée (2x en Normal, 4x en High), puis est redescendue.
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
    double getTailLengthSeconds() const override { return 0.1; }

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

    // Retours audio -> UI (atomiques, sans verrou)
    std::atomic<int>  lastNote { -1 };
    std::atomic<bool> lastAccent { false };

private:
    void handleMidi (const juce::MidiMessage& msg) noexcept;
    void updateVoiceParams() noexcept;
    void renderChunk (float* out, int numSamples, const juce::MidiBuffer& midi, int startSample) noexcept;

    octocid::AcidVoice voice;
    octocid::ToneShaper toneShaper;

    // Oversampling : index 0 = 2x (Normal), 1 = 4x (High)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplers[2];
    int currentQuality = 0;

    juce::AudioBuffer<float> monoBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGain;
    double hostSampleRate = 44100.0;
    int maxBlockSize = 512;

    // Pointeurs directs vers les paramètres (lecture atomique, sans recherche)
    std::atomic<float>* pTune = nullptr;
    std::atomic<float>* pCutoff = nullptr;
    std::atomic<float>* pResonance = nullptr;
    std::atomic<float>* pEnvMod = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pAccent = nullptr;
    std::atomic<float>* pWaveform = nullptr;
    std::atomic<float>* pVolume = nullptr;
    std::atomic<float>* pAccentThr = nullptr;
    std::atomic<float>* pQuality = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctoCidAudioProcessor)
};

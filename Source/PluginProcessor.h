#pragma once

#include <JuceHeader.h>
#include "Params.h"
#include "dsp/AcidVoice.h"
#include "dsp/Drive.h"

//==============================================================================
// OctoCid — moteur sonore + Drive + LFO + Slime Mods + presets d'usine.
// La voix et le Drive tournent à la fréquence suréchantillonnée (2x Normal,
// 4x High), puis le signal est redescendu et façonné dans le grave.
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
    // Presets d'usine (exposés au DAW comme programmes)
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}
    juce::String getProgramDescription (int index) const;

    // Applique un preset même s'il est déjà sélectionné (choix explicite dans l'interface)
    void applyProgram (int index);

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
    void updateParams() noexcept;
    void updateLfoSync() noexcept;
    void renderChunk (float* out, int numSamples, const juce::MidiBuffer& midi, int startSample) noexcept;
    std::atomic<float>* raw (const juce::ParameterID& id) { return apvts.getRawParameterValue (id.getParamID()); }

    static constexpr float kKnee = 0.708f, kCeiling = 0.989f;   // -3 dBFS / -0,1 dBFS

    octocid::AcidVoice voice;
    octocid::Drive drive;
    octocid::ToneShaper toneShaper;

    // Oversampling : index 0 = 2x (Normal), 1 = 4x (High)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplers[2];
    int currentQuality = 0;

    juce::AudioBuffer<float> monoBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGain;
    double hostSampleRate = 44100.0;
    int maxBlockSize = 512;
    int currentProgram = 0;
    bool drivePreVca = false;

    struct RawParams
    {
        std::atomic<float> *tune, *cutoff, *resonance, *envMod, *decay, *accent, *waveform, *volume,
                           *accentThr, *quality,
                           *slideTime, *sweepSpeed, *slidePunch, *accentDecay, *softAttack,
                           *filterTrack, *filterFm, *filterDrive, *subOsc, *subLevel, *muffler,
                           *driveType, *driveAmt, *driveTone, *driveMix, *drivePos,
                           *lfoRate, *lfoSync, *lfoDepth, *lfoShape, *lfoDest;
    } p {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctoCidAudioProcessor)
};

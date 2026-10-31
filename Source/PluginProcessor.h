#pragma once

#include <JuceHeader.h>
#include "Params.h"
#include "dsp/AcidVoice.h"
#include "dsp/Drive.h"
#include "dsp/Fx.h"
#include "seq/Sequencer.h"
#include "seq/PatternIO.h"

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
    double getTailLengthSeconds() const override { return 8.0; }   // queues de delay et de reverb

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
    std::atomic<int>  seqStep { -1 };            // pas en cours (-1 : arrêté)
    std::atomic<int>  seqActivePattern { 0 };    // pattern qui joue
    std::atomic<int>  seqQueuedPattern { 0 };    // pattern qui jouera au prochain cycle
    std::atomic<int>  seqTranspose { 0 };

    //==========================================================================
    // Patterns (fil de l'interface). Chaque modification est transmise au
    // thread audio par une file sans verrou.
    octocid::Pattern getPattern (int index) const;
    void setPattern (int index, const octocid::Pattern& pattern);
    octocid::PatternBank getBank() const;
    void setBank (const octocid::PatternBank& bank);
    double getStepBeats() const;

    int editorPage = 0;   // page affichée par l'éditeur (0 = son, 1 = séquenceur), non sauvegardée

private:
    void handleEvent (const octocid::NoteEvent& ev) noexcept;
    void updateParams() noexcept;
    void updateLfoSync() noexcept;
    void buildEvents (const juce::MidiBuffer& midi, int numSamples) noexcept;
    void drainPatternFifo() noexcept;
    void renderChunk (float* out, int numSamples, int startSample) noexcept;
    std::atomic<float>* raw (const juce::ParameterID& id) { return apvts.getRawParameterValue (id.getParamID()); }

    static constexpr float kKnee = 0.708f, kCeiling = 0.989f;   // -3 dBFS / -0,1 dBFS

    // Limiteur de sécurité : transparent sous -3 dBFS, plafond à -0,1 dBFS
    static float safetyLimit (float x) noexcept
    {
        const float ax = std::abs (x);
        if (ax <= kKnee)
            return x;
        return std::copysign (kKnee + (kCeiling - kKnee) * std::tanh ((ax - kKnee) / (kCeiling - kKnee)), x);
    }

    octocid::AcidVoice voice;
    octocid::Drive drive;
    octocid::ToneShaper toneShaper;
    octocid::PunchCompressor punch;
    octocid::PingPongDelay delay;
    octocid::PlateReverb reverb;

    // Oversampling : index 0 = 2x (Normal), 1 = 4x (High)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplers[2];
    int currentQuality = 0;

    juce::AudioBuffer<float> monoBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGain;
    double hostSampleRate = 44100.0;
    int maxBlockSize = 512;
    int currentProgram = 0;
    bool drivePreVca = false;

    // Séquenceur
    octocid::StepSequencer sequencer;
    octocid::NoteEventList events;
    int eventCursor = 0;
    bool wasSeqOn = false, wasPlaying = false;
    int lastPatternParam = -1;
    double internalPpq = 0.0;

    struct PatternMsg { int index = 0; octocid::Pattern pattern; };
    static constexpr int kFifoSize = 128;
    juce::AbstractFifo patternFifo { kFifoSize };
    std::array<PatternMsg, kFifoSize> fifoData {};
    octocid::PatternBank masterBank;             // copie de référence (fil de l'interface)
    juce::SpinLock bankLock;
    void pushPattern (int index, const octocid::Pattern& pattern);

    struct RawParams
    {
        std::atomic<float> *tune, *cutoff, *resonance, *envMod, *decay, *accent, *waveform, *volume,
                           *accentThr, *quality,
                           *slideTime, *sweepSpeed, *slidePunch, *accentDecay, *softAttack,
                           *filterTrack, *filterFm, *filterDrive, *subOsc, *subLevel, *muffler,
                           *driveType, *driveAmt, *driveTone, *driveMix, *drivePos,
                           *punchAmt, *punchAttack, *delayMix, *delayTime, *delayFb, *reverbMix, *reverbDecay, *reverbTone,
                           *seqOn, *seqPattern, *seqRes, *seqSwing,
                           *lfoRate, *lfoSync, *lfoDepth, *lfoShape, *lfoDest;
    } p {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctoCidAudioProcessor)
};

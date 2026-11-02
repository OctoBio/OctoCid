#pragma once

#include <JuceHeader.h>
#include "Presets.h"
#include <bitset>
#include "Params.h"
#include "dsp/AcidVoice.h"
#include "dsp/Drive.h"
#include "dsp/Fx.h"
#include "dsp/FxPlus.h"
#include "dsp/ModLfo.h"
#include "dsp/LfoShape.h"
#include <array>
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

    // Presets utilisateur : fichiers .ocpreset dans Documents/OctoBio/OctoCid/Presets
    // (après les presets d'usine dans la liste des programmes)
    static juce::File userPresetFolder();
    void rescanUserPresets();
    bool saveUserPreset (const juce::String& name);       // remplace s'il existe déjà
    bool deleteUserPreset (int programIndex);
    bool isUserPreset (int programIndex) const { return programIndex >= FactoryPresets::count; }

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    // Retours audio -> UI (atomiques, sans verrou)
    std::atomic<int>  lastNote { -1 };
    std::atomic<bool> lastAccent { false };
    // Animation de l'escargot (lecture seule côté interface)
    std::atomic<int>   noteCount { 0 };     // incrémenté à chaque note jouée
    std::atomic<bool>  lastSlide { false }; // la dernière note était glissée
    std::atomic<float> lfoView { 0.0f };    // valeur du LFO (-1..1)
    // Modulation, pour l'interface : sortie et position des 3 LFO, décalage de chaque destination
    std::array<std::atomic<float>, ModParams::numLfos> lfoValueView {}, lfoPhaseView {};
    std::array<std::atomic<float>, ModParams::numDests> modOffsetView {};

    // Formes dessinées des LFO (fil de l'interface) ; setLfoShape les transmet au thread audio
    const octocid::LfoShape& getLfoShape (int lfo) const { return lfoShapes[(size_t) juce::jlimit (0, ModParams::numLfos - 1, lfo)]; }
    void setLfoShape (int lfo, const octocid::LfoShape& shape);
    std::atomic<int> lfoShapeVersion { 0 };     // change à chaque modification (pour l'interface)
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
    double getGate() const;

    // Enregistrement pas à pas : les notes jouées au clavier MIDI sont envoyées à l'interface
    struct RecNote { int note = 0; int velocity = 0; bool legato = false; };
    std::atomic<bool> stepRecord { false };
    bool popRecordedNote (RecNote& out);

    int editorPage = 0;
    int editorWidth = 0;  // largeur de la fenêtre (0 = préférence globale), sauvegardée avec le projet   // page affichée par l'éditeur (0 = son, 1 = séquenceur), non sauvegardée

private:
    void handleEvent (const octocid::NoteEvent& ev) noexcept;
    void updateParams() noexcept;
    void updateVoiceParams() noexcept;
    void updateFxParams() noexcept;
    double blockBpm = 120.0;

    // Modulation (3 LFO, 8 emplacements)
    void setupModulation();
    void migrateOldLfo();
    void resetModulation();
    void computeModulation (int numSamples, double ppq, bool playing) noexcept;
    float modValueIndex (int destIndex, std::atomic<float>* raw) const noexcept;
    float modValue (std::atomic<float>* raw) const noexcept;
    void publishModulationView() noexcept;
    void processFx (float* left, float* right, int start, int n, double ppqStart, double dPpq) noexcept;
    struct DestRef { juce::RangedAudioParameter* param = nullptr; std::atomic<float>* raw = nullptr; };
    struct LfoRaw { std::atomic<float> *shape, *rate, *sync, *mode, *phase, *delay, *fade, *smooth, *pol; };
    struct SlotRaw { std::atomic<float> *src, *dst, *amt; };
    std::array<DestRef, ModParams::numDests> modDest {};
    std::array<LfoRaw, ModParams::numLfos> lfoRaw {};
    std::array<SlotRaw, ModParams::numSlots> slotRaw {};
    std::array<octocid::ModLfo, ModParams::numLfos> modLfo {};
    // formes : copie de l'interface -> table « en attente » (verrou bref) -> table audio
    std::array<octocid::LfoShape, ModParams::numLfos> lfoShapes {};
    std::array<std::array<float, octocid::LfoShape::tableSize>, ModParams::numLfos> pendingTables {}, audioTables {};
    std::array<std::atomic<bool>, ModParams::numLfos> tableDirty {};
    juce::SpinLock tableLock;
    void pullLfoTables() noexcept;
    void writeShapesTo (juce::ValueTree& tree) const;
    void readShapesFrom (const juce::ValueTree& tree);
    std::array<float, ModParams::numLfos> lfoValue {};
    std::array<bool, ModParams::numLfos> lfoUsed {};
    std::array<int, ModParams::numLfos> lfoRateDest {};
    std::array<float, ModParams::numDests> modOffset {};
    bool anyModulation = false;
    int cutoffDest = 0;
    int lastModNoteCount = 0;
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
    octocid::TransientShaper impactFx;
    octocid::MultibandSmash smashFx;
    octocid::JetModulator jetFx;
    octocid::Pumper pumpFx;
    octocid::StereoWidener widthFx;
    double fxPpq = 0.0;
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
    struct UserPreset { juce::String name; juce::File file; };
    std::vector<UserPreset> userPresets;
    juce::CriticalSection userPresetLock;
    juce::String pendingUserPresetName;   // rappel d'état : nom du preset utilisateur courant
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

    static constexpr int kRecFifoSize = 256;
    juce::AbstractFifo recFifo { kRecFifoSize };
    std::array<RecNote, kRecFifoSize> recData {};
    std::bitset<128> heldKeys;          // touches du clavier MIDI enfoncées (pour détecter le legato)
    void pushRecorded (int note, int velocity, bool legato) noexcept;

    struct RawParams
    {
        std::atomic<float> *tune, *cutoff, *resonance, *envMod, *decay, *accent, *waveform, *volume,
                           *accentThr, *quality,
                           *slideTime, *sweepSpeed, *slidePunch, *accentDecay, *softAttack,
                           *filterTrack, *filterFm, *filterDrive, *subOsc, *subLevel, *muffler,
                           *driveType, *driveAmt, *driveTone, *driveMix, *drivePos,
                           *punchAmt, *punchAttack, *delayMix, *delayTime, *delayFb, *reverbMix, *reverbDecay, *reverbTone,
                           *seqOn, *seqPattern, *seqRes, *seqSwing, *seqGate, *analog,
                           *lfoRate, *lfoSync, *lfoDepth, *lfoShape, *lfoDest,
                           *impact, *smash, *jetMix, *jetMode, *jetRate, *jetFb, *pump, *pumpRate, *width;
    } p {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctoCidAudioProcessor)
};

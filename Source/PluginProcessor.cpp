#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ui/Palette.h"
#include "Presets.h"

//==============================================================================
OctoCidAudioProcessor::OctoCidAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "OctoCidState", createParameterLayout())
{
    using namespace ParamIDs;
    rescanUserPresets();
    p.tune = raw (tune);           p.cutoff = raw (cutoff);       p.resonance = raw (resonance);
    p.envMod = raw (envMod);       p.decay = raw (decay);         p.accent = raw (accent);
    p.waveform = raw (waveform);   p.volume = raw (volume);
    p.accentThr = raw (accentThreshold);                          p.quality = raw (quality);
    p.slideTime = raw (slideTime); p.sweepSpeed = raw (sweepSpeed); p.slidePunch = raw (slidePunch);
    p.accentDecay = raw (accentDecay); p.softAttack = raw (softAttack);
    p.filterTrack = raw (filterTrack); p.filterFm = raw (filterFm); p.filterDrive = raw (filterDrive);
    p.subOsc = raw (subOsc);       p.subLevel = raw (subLevel);   p.muffler = raw (muffler);
    p.driveType = raw (driveType); p.driveAmt = raw (driveAmt);   p.driveTone = raw (driveTone);
    p.driveMix = raw (driveMix);   p.drivePos = raw (drivePos);
    p.punchAmt = raw (ParamIDs::punch); p.punchAttack = raw (punchAttack);
    p.delayMix = raw (delayMix);   p.delayTime = raw (delayTime); p.delayFb = raw (delayFb);
    p.reverbMix = raw (reverbMix); p.reverbDecay = raw (reverbDecay); p.reverbTone = raw (reverbTone);
    p.lfoRate = raw (lfoRate);     p.lfoSync = raw (lfoSync);     p.lfoDepth = raw (lfoDepth);
    p.lfoShape = raw (lfoShape);   p.lfoDest = raw (lfoDest);
    p.seqOn = raw (seqOn);         p.seqPattern = raw (seqPattern); p.seqRes = raw (seqRes); p.seqSwing = raw (seqSwing);
    p.seqGate = raw (seqGate);     p.analog = raw (analog);
    p.impact = raw (impact); p.smash = raw (smash); p.jetMix = raw (jetMix); p.jetMode = raw (jetMode);
    p.jetRate = raw (jetRate); p.jetFb = raw (jetFb); p.pump = raw (ParamIDs::pump); p.pumpRate = raw (pumpRate);
    p.width = raw (ParamIDs::width);
    setupModulation();

    // 8 patterns de démo au départ
    masterBank = octocid::PatternTools::demoBank();
    sequencer.setBank (masterBank);
}

juce::AudioProcessorValueTreeState::ParameterLayout OctoCidAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = AudioParameterFloatAttributes().withLabel ("%");
    auto ms  = AudioParameterFloatAttributes().withLabel ("ms");
    auto percent = [&] (const ParameterID& id, const juce::String& name, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (id, name, NormalisableRange<float> (0.0f, 100.0f, 0.1f), def, pct));
    };
    auto skewed = [] (float lo, float hi, float centre, float step)
    {
        NormalisableRange<float> r (lo, hi, step);
        r.setSkewForCentre (centre);
        return r;
    };

    // --- Panneau principal (ordre d'origine conservé)
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::tune, "Tune",
        NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f, AudioParameterFloatAttributes().withLabel ("st")));
    percent (ParamIDs::cutoff,    "Cutoff",    50.0f);
    percent (ParamIDs::resonance, "Resonance", 50.0f);
    percent (ParamIDs::envMod,    "Env Mod",   50.0f);
    percent (ParamIDs::decay,     "Decay",     50.0f);
    percent (ParamIDs::accent,    "Accent",    50.0f);
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::waveform, "Waveform", StringArray { "Saw", "Square" }, 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::volume, "Volume",
        NormalisableRange<float> (-48.0f, 6.0f, 0.1f), 1.0f, AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<AudioParameterInt> (ParamIDs::accentThreshold, "Accent Velocity", 1, 127, 100));
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::quality, "Quality", StringArray { "Normal (2x)", "High (4x)" }, 0));

    // --- Slime Mods (à leur défaut : son d'origine)
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::slideTime, "Slide Time",
        NormalisableRange<float> (60.0f, 360.0f, 1.0f), 60.0f, ms));
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::sweepSpeed, "Sweep Speed", StringArray { "Slow", "Normal", "Fast" }, 1));
    percent (ParamIDs::slidePunch, "Slide Punch", 0.0f);
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::accentDecay, "Accent Decay", skewed (30.0f, 3000.0f, 300.0f, 1.0f), 200.0f, ms));
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::softAttack, "Soft Attack", skewed (0.3f, 30.0f, 3.0f, 0.01f), 3.0f, ms));
    percent (ParamIDs::filterTrack, "Filter Tracking", 0.0f);
    percent (ParamIDs::filterFm,    "Filter FM",       0.0f);
    percent (ParamIDs::filterDrive, "Filter Drive",    0.0f);
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::subOsc, "Sub Osc", StringArray { "Off", "-1 oct", "-2 oct" }, 0));
    percent (ParamIDs::subLevel, "Sub Level", 50.0f);
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::muffler, "Muffler", StringArray { "Off", "1", "2" }, 0));
    percent (ParamIDs::analog, "Analog", 0.0f);

    // --- Drive
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::driveType, "Drive Type", StringArray { "Off", "Overdrive", "Distortion" }, 0));
    percent (ParamIDs::driveAmt,  "Drive",      50.0f);
    percent (ParamIDs::driveTone, "Drive Tone", 60.0f);
    percent (ParamIDs::driveMix,  "Drive Mix", 100.0f);
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::drivePos, "Drive Position", StringArray { "Apres", "Avant" }, 0));

    // --- Punch + FX (tous à zéro par défaut : son sec)
    percent (ParamIDs::punch, "Punch", 0.0f);
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::punchAttack, "Punch Attack", skewed (1.0f, 40.0f, 10.0f, 0.1f), 10.0f, ms));
    percent (ParamIDs::delayMix, "Delay Mix", 0.0f);
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::delayTime, "Delay Time", DelaySync::names, 3));
    percent (ParamIDs::delayFb, "Delay Feedback", 35.0f);
    percent (ParamIDs::reverbMix, "Reverb Mix", 0.0f);
    percent (ParamIDs::reverbDecay, "Reverb Decay", 35.0f);
    percent (ParamIDs::reverbTone, "Reverb Tone", 50.0f);

    // --- Effets (page EFFETS ; tous à zéro par défaut)
    percent (ParamIDs::impact, "Impact", 0.0f);
    percent (ParamIDs::smash,  "Smash", 0.0f);
    percent (ParamIDs::jetMix, "Jet Mix", 0.0f);
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::jetMode, "Jet Mode", StringArray { "Flanger", "Phaser" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::jetRate, "Jet Rate", JetSync::names, 2));
    percent (ParamIDs::jetFb,  "Jet Feedback", 50.0f);
    percent (ParamIDs::pump,   "Pump", 0.0f);
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::pumpRate, "Pump Rate", PumpSync::names, 0));
    percent (ParamIDs::width,  "Width", 0.0f);

    // --- 3 LFO de modulation + 8 emplacements d'assignation
    for (int i = 0; i < ModParams::numLfos; ++i)
    {
        const juce::String n = "LFO " + juce::String (i + 1) + " ";
        auto id = [i] (const char* w) { return juce::ParameterID { ModParams::lfoId (i, w), 1 }; };
        layout.add (std::make_unique<AudioParameterChoice> (id ("shape"), n + "Shape", ModParams::shapeNames, 0));
        layout.add (std::make_unique<AudioParameterFloat> (id ("rate"), n + "Rate", skewed (0.02f, 30.0f, 2.0f, 0.01f), 2.0f,
                                                           AudioParameterFloatAttributes().withLabel ("Hz")));
        juce::StringArray syncNames; for (auto* nm : octocid::ModSync::names) syncNames.add (juce::String::fromUTF8 (nm));
        layout.add (std::make_unique<AudioParameterChoice> (id ("sync"), n + "Sync", syncNames, 0));
        layout.add (std::make_unique<AudioParameterChoice> (id ("mode"), n + "Mode", ModParams::modeNames, 0));
        layout.add (std::make_unique<AudioParameterFloat> (id ("phase"), n + "Phase", NormalisableRange<float> (0.0f, 360.0f, 1.0f), 0.0f,
                                                           AudioParameterFloatAttributes().withLabel (juce::String::fromUTF8 ("°"))));
        layout.add (std::make_unique<AudioParameterFloat> (id ("delay"), n + "Delay", skewed (0.0f, 4000.0f, 500.0f, 1.0f), 0.0f, ms));
        layout.add (std::make_unique<AudioParameterFloat> (id ("fade"), n + "Fade", skewed (0.0f, 4000.0f, 500.0f, 1.0f), 0.0f, ms));
        percent (id ("smooth"), n + "Smooth", 0.0f);
        layout.add (std::make_unique<AudioParameterChoice> (id ("pol"), n + "Polarity", StringArray { "Bipolaire", "Unipolaire" }, 0));
    }
    for (int k = 0; k < ModParams::numSlots; ++k)
    {
        const juce::String n = "Mod " + juce::String (k + 1) + " ";
        auto id = [k] (const char* w) { return juce::ParameterID { ModParams::slotId (k, w), 1 }; };
        layout.add (std::make_unique<AudioParameterChoice> (id ("src"), n + "Source", ModParams::sourceNames, 0));
        layout.add (std::make_unique<AudioParameterChoice> (id ("dst"), n + "Destination", ModParams::destNames(), 0));
        layout.add (std::make_unique<AudioParameterFloat> (id ("amt"), n + "Amount", NormalisableRange<float> (-100.0f, 100.0f, 0.01f), 0.0f, pct));
    }

    // --- Séquenceur
    layout.add (std::make_unique<AudioParameterBool> (ParamIDs::seqOn, "Seq On", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::seqPattern, "Seq Pattern",
        StringArray { "A", "B", "C", "D", "E", "F", "G", "H" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::seqRes, "Seq Resolution", SeqRes::names, 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::seqSwing, "Seq Swing",
        NormalisableRange<float> (0.0f, 75.0f, 0.1f), 0.0f, pct));
    layout.add (std::make_unique<AudioParameterBool> (ParamIDs::snailAnim, "Snail Animation", true));
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::seqGate, "Seq Gate",
        NormalisableRange<float> (10.0f, 95.0f, 0.1f), 50.0f, pct));

    // --- LFO
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::lfoRate, "LFO Rate", skewed (0.05f, 20.0f, 2.0f, 0.01f), 2.0f,
        AudioParameterFloatAttributes().withLabel ("Hz")));
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::lfoSync, "LFO Sync", LfoSync::names, 0));
    percent (ParamIDs::lfoDepth, "LFO Depth", 0.0f);
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::lfoShape, "LFO Shape", StringArray { "Sine", "Triangle", "Square", "S&H" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::lfoDest, "LFO Dest", StringArray { "Cutoff", "Resonance", "Pitch" }, 0));

    return layout;
}

//==============================================================================
void OctoCidAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    hostSampleRate = sampleRate;
    maxBlockSize = juce::jmax (1, samplesPerBlock);

    for (int i = 0; i < 2; ++i)
    {
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>> (
            1, (size_t) (i + 1), juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        oversamplers[i]->initProcessing ((size_t) maxBlockSize);
    }

    monoBuffer.setSize (1, maxBlockSize, false, true, false);

    punch.prepare (sampleRate);
    impactFx.prepare (sampleRate); smashFx.prepare (sampleRate); jetFx.prepare (sampleRate);
    pumpFx.prepare (sampleRate); widthFx.prepare (sampleRate);
    fxPpq = 0.0;
    resetModulation();
    delay.prepare (sampleRate);
    reverb.prepare (sampleRate);

    currentQuality = juce::jlimit (0, 1, (int) p.quality->load());
    const double osRate = sampleRate * (double) oversamplers[currentQuality]->getOversamplingFactor();
    voice.prepare (osRate);
    drive.prepare (osRate);
    updateParams();
    voice.reset();
    drive.reset();
    punch.reset();
    impactFx.reset(); smashFx.reset(); jetFx.reset(); widthFx.reset();
    delay.reset();
    sequencer.reset();
    wasPlaying = false;
    internalPpq = 0.0;
    reverb.reset();
    toneShaper.prepare (sampleRate);

    outputGain.reset (sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p.volume->load()));

    setLatencySamples ((int) std::round (oversamplers[currentQuality]->getLatencyInSamples()));
}

bool OctoCidAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

//==============================================================================
void OctoCidAudioProcessor::updateParams() noexcept
{
    blockBpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                blockBpm = *b;
    updateVoiceParams();
    updateFxParams();
}

// Réglages de la voix et du Drive (relus toutes les 64 échantillons)
void OctoCidAudioProcessor::updateVoiceParams() noexcept
{
    octocid::VoiceParams v;
    v.tune      = modValue (p.tune);
    v.cutoff    = p.cutoff->load()    * 0.01f;
    // Cutoff modulé en octaves (comme le circuit : linéaire en octaves, sans butée sur la course du potard)
    if (anyModulation) v.cutoffModOct = modOffset[(size_t) cutoffDest] * 2.931652f;
    v.resonance = modValue (p.resonance) * 0.01f;
    v.envMod    = modValue (p.envMod)    * 0.01f;
    v.decay     = modValue (p.decay)     * 0.01f;
    v.accent    = modValue (p.accent)    * 0.01f;
    v.squareMix = p.waveform->load() > 0.5f ? 1.0f : 0.0f;   // fondu lissé dans la voix

    v.slideTimeMs   = modValue (p.slideTime);
    v.sweepSpeed    = juce::jlimit (0, 2, (int) p.sweepSpeed->load());
    v.slidePunch    = modValue (p.slidePunch) * 0.01f;
    v.accentDecayMs = modValue (p.accentDecay);
    v.softAttackMs  = modValue (p.softAttack);
    v.filterTrack   = modValue (p.filterTrack) * 0.01f;
    v.filterFm      = modValue (p.filterFm) * 0.01f;
    v.filterDrive   = modValue (p.filterDrive) * 0.01f;
    v.subMode       = juce::jlimit (0, 2, (int) p.subOsc->load());
    v.subLevel      = modValue (p.subLevel) * 0.01f;
    v.muffler       = juce::jlimit (0, 2, (int) p.muffler->load());
    v.analog        = modValue (p.analog) * 0.01f;

    v.lfoShape  = juce::jlimit (0, 3, (int) p.lfoShape->load());
    v.lfoDest   = juce::jlimit (0, 2, (int) p.lfoDest->load());
    v.lfoDepth  = p.lfoDepth->load() * 0.01f;
    v.lfoRateHz = p.lfoRate->load();

    // LFO synchronisé : fréquence tirée du tempo du DAW
    const int sync = juce::jlimit (0, LfoSync::names.size() - 1, (int) p.lfoSync->load());
    if (sync > 0)
        v.lfoRateHz = (float) (blockBpm / 60.0 / LfoSync::beats[sync]);

    voice.setParams (v);
    voice.setAccentThreshold ((int) p.accentThr->load());

    drive.setParams ((int) p.driveType->load(), modValue (p.driveAmt) * 0.01f,
                     modValue (p.driveTone) * 0.01f, modValue (p.driveMix) * 0.01f);

    const bool pre = p.drivePos->load() > 0.5f;
    if (pre != drivePreVca)
    {
        drivePreVca = pre;
        drive.reset();               // les états du Drive ne valent rien d'un emplacement à l'autre
    }
    voice.setPreVcaDrive (drivePreVca ? &drive : nullptr);
}

// Réglages des effets (une fois par bloc : ils sont lissés à l'échantillon)
void OctoCidAudioProcessor::updateFxParams() noexcept
{
    // Effets
    impactFx.setAmount (modValue (p.impact) * 0.01f);
    smashFx.setAmount (modValue (p.smash) * 0.01f);
    jetFx.setParams ((int) p.jetMode->load(), modValue (p.jetMix) * 0.01f, modValue (p.jetFb) * 0.0095f);
    pumpFx.setDepth (modValue (p.pump) * 0.01f);
    widthFx.setAmount (modValue (p.width) * 0.01f);

    // Punch + FX (fréquence de l'hôte)
    punch.setAmount (modValue (p.punchAmt) * 0.01f);
    punch.setAttackMs (modValue (p.punchAttack));
    reverb.setParams (modValue (p.reverbMix) * 0.01f, modValue (p.reverbDecay) * 0.01f, modValue (p.reverbTone) * 0.01f);
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm())
                    bpm = *b;
        const int t = juce::jlimit (0, DelaySync::names.size() - 1, (int) p.delayTime->load());
        delay.setParams ((float) (DelaySync::beats[t] * 60.0 / juce::jlimit (20.0, 400.0, bpm)),
                         modValue (p.delayFb) * 0.01f, modValue (p.delayMix) * 0.01f);
    }
}

// En lecture, la phase du LFO synchronisé est recalée sur la position du DAW
void OctoCidAudioProcessor::updateLfoSync() noexcept
{
    const int sync = (int) p.lfoSync->load();
    if (sync <= 0 || sync >= LfoSync::names.size())
        return;

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (pos->getIsPlaying())
                if (auto ppq = pos->getPpqPosition())
                    voice.syncLfo ((float) std::fmod (*ppq / LfoSync::beats[sync], 1.0));
}

void OctoCidAudioProcessor::handleEvent (const octocid::NoteEvent& ev) noexcept
{
    using octocid::NoteEvent;
    switch (ev.type)
    {
        case NoteEvent::On:
            lastSlide.store (voice.isGateOn(), std::memory_order_relaxed);
            voice.noteOnAccent (ev.note, ev.accent);
            noteCount.fetch_add (1, std::memory_order_relaxed);
            lastNote.store (ev.note, std::memory_order_relaxed);
            lastAccent.store (ev.accent, std::memory_order_relaxed);
            break;
        case NoteEvent::Off:
            voice.noteOff (ev.note);
            if (! voice.isGateOn())
                lastNote.store (-1, std::memory_order_relaxed);
            break;
        case NoteEvent::AllOff:
        default:
            voice.allNotesOff();
            lastNote.store (-1, std::memory_order_relaxed);
            break;
    }
}

//==============================================================================
// Patterns : copie de référence côté interface, file sans verrou vers l'audio
void OctoCidAudioProcessor::pushPattern (int index, const octocid::Pattern& pattern)
{
    const auto scope = patternFifo.write (1);
    if (scope.blockSize1 > 0)
        fifoData[(size_t) scope.startIndex1] = { index, pattern };
    else if (scope.blockSize2 > 0)
        fifoData[(size_t) scope.startIndex2] = { index, pattern };
}

void OctoCidAudioProcessor::drainPatternFifo() noexcept
{
    const int ready = patternFifo.getNumReady();
    if (ready <= 0)
        return;
    const auto scope = patternFifo.read (ready);
    for (int i = 0; i < scope.blockSize1; ++i)
        sequencer.setPattern (fifoData[(size_t) (scope.startIndex1 + i)].index, fifoData[(size_t) (scope.startIndex1 + i)].pattern);
    for (int i = 0; i < scope.blockSize2; ++i)
        sequencer.setPattern (fifoData[(size_t) (scope.startIndex2 + i)].index, fifoData[(size_t) (scope.startIndex2 + i)].pattern);
}

octocid::Pattern OctoCidAudioProcessor::getPattern (int index) const
{
    const juce::SpinLock::ScopedLockType sl (bankLock);
    return masterBank.patterns[juce::jlimit (0, octocid::PatternBank::kNumPatterns - 1, index)];
}

void OctoCidAudioProcessor::setPattern (int index, const octocid::Pattern& pattern)
{
    if (index < 0 || index >= octocid::PatternBank::kNumPatterns)
        return;
    {
        const juce::SpinLock::ScopedLockType sl (bankLock);
        masterBank.patterns[index] = pattern;
    }
    pushPattern (index, pattern);
}

octocid::PatternBank OctoCidAudioProcessor::getBank() const
{
    const juce::SpinLock::ScopedLockType sl (bankLock);
    return masterBank;
}

void OctoCidAudioProcessor::setBank (const octocid::PatternBank& bank)
{
    {
        const juce::SpinLock::ScopedLockType sl (bankLock);
        masterBank = bank;
    }
    for (int i = 0; i < octocid::PatternBank::kNumPatterns; ++i)
        pushPattern (i, bank.patterns[i]);
}

double OctoCidAudioProcessor::getGate() const
{
    return p.seqGate->load() * 0.01;
}

void OctoCidAudioProcessor::pushRecorded (int note, int velocity, bool legato) noexcept
{
    const auto scope = recFifo.write (1);
    if (scope.blockSize1 > 0)      recData[(size_t) scope.startIndex1] = { note, velocity, legato };
    else if (scope.blockSize2 > 0) recData[(size_t) scope.startIndex2] = { note, velocity, legato };
}

bool OctoCidAudioProcessor::popRecordedNote (RecNote& out)
{
    if (recFifo.getNumReady() <= 0)
        return false;
    const auto scope = recFifo.read (1);
    if (scope.blockSize1 > 0)      out = recData[(size_t) scope.startIndex1];
    else if (scope.blockSize2 > 0) out = recData[(size_t) scope.startIndex2];
    else return false;
    return true;
}

double OctoCidAudioProcessor::getStepBeats() const
{
    const int r = juce::jlimit (0, SeqRes::names.size() - 1, (int) p.seqRes->load());
    return SeqRes::beats[r];
}

//==============================================================================
// Construit la liste des notes du bloc : séquenceur (SEQ ON) ou clavier (SEQ OFF)
void OctoCidAudioProcessor::buildEvents (const juce::MidiBuffer& midi, int numSamples) noexcept
{
    using octocid::NoteEvent;
    events.clear();
    eventCursor = 0;

    drainPatternFifo();

    const bool seqOn = p.seqOn->load() > 0.5f;
    if (seqOn != wasSeqOn)
    {
        events.add ({ 0, NoteEvent::AllOff, 0, false });
        sequencer.reset();
        wasSeqOn = seqOn;
        wasPlaying = false;
    }

    if (! seqOn)
    {
        // Jeu au clavier / piano roll : vélocité >= seuil = accent, chevauchement = slide
        const int threshold = (int) p.accentThr->load();
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();
            const int s = juce::jlimit (0, numSamples - 1, meta.samplePosition);
            if (msg.isNoteOn())
            {
                events.add ({ s, NoteEvent::On, msg.getNoteNumber(), msg.getVelocity() >= threshold });
                if (stepRecord.load (std::memory_order_relaxed))
                    pushRecorded (msg.getNoteNumber(), msg.getVelocity(), heldKeys.any());
                heldKeys.set ((size_t) msg.getNoteNumber());
            }
            else if (msg.isNoteOff())
            {
                events.add ({ s, NoteEvent::Off, msg.getNoteNumber(), false });
                heldKeys.reset ((size_t) msg.getNoteNumber());
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                events.add ({ s, NoteEvent::AllOff, 0, false });
                heldKeys.reset();
            }
        }
        seqStep.store (-1, std::memory_order_relaxed);
        return;
    }

    // --- SEQ ON : le MIDI entrant choisit le pattern (Do0..Sol0) ou transpose (Do3 = 0)
    const int patParam = juce::jlimit (0, 7, (int) p.seqPattern->load());
    if (patParam != lastPatternParam)
    {
        sequencer.requestPattern (patParam);
        lastPatternParam = patParam;
    }
    const bool recording = stepRecord.load (std::memory_order_relaxed);
    const int threshold = (int) p.accentThr->load();
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int s = juce::jlimit (0, numSamples - 1, meta.samplePosition);
        if (msg.isNoteOff())
        {
            if (recording)
                events.add ({ s, NoteEvent::Off, msg.getNoteNumber(), false });
            heldKeys.reset ((size_t) msg.getNoteNumber());
            continue;
        }
        if (! msg.isNoteOn())
            continue;
        const int n = msg.getNoteNumber();
        if (n >= 24 && n <= 31)
            sequencer.requestPattern (n - 24);
        else if (recording)
        {
            // Enregistrement pas à pas : la note est écrite dans le pattern et on l'entend
            pushRecorded (n, msg.getVelocity(), heldKeys.any());
            events.add ({ s, NoteEvent::On, n, msg.getVelocity() >= threshold });
            heldKeys.set ((size_t) n);
            continue;
        }
        else
            sequencer.setTranspose (n - 60);
        heldKeys.set ((size_t) n);
    }
    sequencer.setGate (modValue (p.seqGate) * 0.01);

    // --- Position du DAW
    double bpm = 120.0, ppq = 0.0;
    bool playing = false, hostClock = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition())
            {
                ppq = *q;
                playing = pos->getIsPlaying();
                hostClock = true;
            }
        }

    if (! hostClock)
    {
        // Pas de transport (application autonome) : horloge interne à 120 BPM
        playing = true;
        ppq = internalPpq;
        internalPpq += numSamples / (hostSampleRate * 60.0 / bpm);
    }

    if (playing)
    {
        const int r = juce::jlimit (0, SeqRes::names.size() - 1, (int) p.seqRes->load());
        sequencer.process (ppq, juce::jlimit (20.0, 999.0, bpm), numSamples, hostSampleRate,
                           SeqRes::beats[r], p.seqSwing->load() * 0.01f, 36, events);
    }
    else if (wasPlaying)
    {
        sequencer.stop (events, 0);
    }
    wasPlaying = playing;

    seqStep.store (playing ? sequencer.getCurrentStep() : -1, std::memory_order_relaxed);
    seqActivePattern.store (sequencer.getActivePattern(), std::memory_order_relaxed);
    seqQueuedPattern.store (sequencer.getRequestedPattern(), std::memory_order_relaxed);
    seqTranspose.store (sequencer.getTranspose(), std::memory_order_relaxed);
}

// Rend numSamples échantillons (≤ maxBlockSize) en traitant les notes au sample près
void OctoCidAudioProcessor::renderChunk (float* out, int numSamples, int startSample) noexcept
{
    auto& os = *oversamplers[currentQuality];
    const int factor = (int) os.getOversamplingFactor();

    juce::dsp::AudioBlock<float> mono (monoBuffer.getArrayOfWritePointers(), 1, (size_t) numSamples);
    mono.clear();

    auto up = os.processSamplesUp (mono);
    float* osData = up.getChannelPointer (0);

    for (int i = 0; i < numSamples; ++i)
    {
        while (eventCursor < events.size() && events[eventCursor].sample <= startSample + i)
            handleEvent (events[eventCursor++]);

        for (int j = 0; j < factor; ++j)
            osData[i * factor + j] = drivePreVca ? voice.process() : drive.process (voice.process());
    }

    os.processSamplesDown (mono);

    // Façonnage du grave à la fréquence de l'hôte
    const float* down = monoBuffer.getReadPointer (0);
    for (int i = 0; i < numSamples; ++i)
        out[i] = toneShaper.process (down[i]);
}

void OctoCidAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    // Changement de qualité : voix et Drive passent à la nouvelle fréquence sans couper le son
    const int q = juce::jlimit (0, 1, (int) p.quality->load());
    if (q != currentQuality)
    {
        currentQuality = q;
        oversamplers[q]->reset();
        const double osRate = hostSampleRate * (double) oversamplers[q]->getOversamplingFactor();
        voice.setSampleRate (osRate);
        drive.prepare (osRate);
    }

    // Position musicale (Jet, Pompe, LFO) : celle du DAW en lecture, sinon roue libre à son tempo
    double bpm = 120.0;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = juce::jlimit (20.0, 400.0, *b);
            if (pos->getIsPlaying())
                if (auto ppq = pos->getPpqPosition()) { fxPpq = *ppq; playing = true; }
        }
    blockBpm = bpm;
    const double dPpq = bpm / 60.0 / hostSampleRate;

    updateLfoSync();
    buildEvents (midi, numSamples);

    const bool stereo = buffer.getNumChannels() > 1;
    float* left = buffer.getWritePointer (0);
    float* right = stereo ? buffer.getWritePointer (1) : nullptr;

    // Rendu par tranches de 64 échantillons : modulation, réglages de la voix et des effets
    // relus à chaque tranche (un potard ou un LFO agit en ~1 ms quel que soit le buffer du DAW).
    constexpr int kParamSlice = 64;
    for (int pos = 0; pos < numSamples; pos += kParamSlice)
    {
        const int n = std::min ({ kParamSlice, maxBlockSize, numSamples - pos });
        const double slicePpq = fxPpq + dPpq * (double) pos;
        computeModulation (n, slicePpq, playing);
        updateVoiceParams();
        updateFxParams();
        renderChunk (left + pos, n, pos);
        processFx (left, right, pos, n, slicePpq, dPpq);
    }
    fxPpq += dPpq * (double) numSamples;
    publishModulationView();

    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

// Punch (mono), puis Jet, delay, reverb, pompe, largeur (stéréo), volume et limiteur de sécurité
void OctoCidAudioProcessor::processFx (float* left, float* right, int start, int n, double ppqStart, double dPpq) noexcept
{
    const bool usePunch = punch.isActive();
    const bool useDelay = delay.isActive();
    const bool useReverb = reverb.isActive();
    const bool useImpact = impactFx.isActive(), useSmash = smashFx.isActive(), useJet = jetFx.isActive();
    const bool usePump = pumpFx.isActive(), useWidth = widthFx.isActive();
    const double jetBeats = JetSync::beats[juce::jlimit (0, JetSync::names.size() - 1, (int) p.jetRate->load())];
    const double pumpBeats = PumpSync::beats[juce::jlimit (0, PumpSync::names.size() - 1, (int) p.pumpRate->load())];

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (modValue (p.volume), -60.0f) + 1.0e-6f);
    for (int k = 0; k < n; ++k)
    {
        const int i = start + k;
        const double ppqNow = ppqStart + dPpq * (double) k;
        float l = left[i];
        if (useImpact) l = impactFx.process (l);
        if (usePunch)  l = punch.process (l);
        if (useSmash)  l = smashFx.process (l);
        float r = l;
        if (useJet)    jetFx.process (l, (float) std::fmod (ppqNow / jetBeats, 1.0), l, r);
        if (useDelay)  delay.process (l, r);
        if (useReverb) reverb.process (l, r);
        if (usePump)
        {
            const float pg = pumpFx.gain ((float) std::fmod (ppqNow / pumpBeats, 1.0));
            l *= pg; r *= pg;
        }
        if (useWidth)  widthFx.process (l, r);

        const float g = outputGain.getNextValue();
        l = safetyLimit (l * g);
        r = safetyLimit (r * g);

        if (right != nullptr)
        {
            left[i] = l;
            right[i] = r;
        }
        else
            left[i] = 0.5f * (l + r);   // sortie mono : on replie la stéréo des FX
    }
}

//==============================================================================
// Modulation : 3 LFO -> 8 emplacements -> décalage (en valeur normalisée) par destination
// L'ancien LFO (une seule destination) devient un LFO assignable : presets et anciens projets sonnent pareil
void OctoCidAudioProcessor::migrateOldLfo()
{
    const float depth = p.lfoDepth->load();
    if (depth < 0.01f) return;

    auto setReal = [this] (const juce::String& id, float v)
    {
        if (auto* prm = apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    };
    int slot = -1;
    bool lfoTaken[ModParams::numLfos] {};
    for (int k = 0; k < ModParams::numSlots; ++k)
    {
        const int src = (int) slotRaw[(size_t) k].src->load();
        if (src > 0) lfoTaken[src - 1] = true;
        else if (slot < 0) slot = k;
    }
    if (slot < 0) return;
    int lfo = 0;
    while (lfo < ModParams::numLfos - 1 && lfoTaken[lfo]) ++lfo;

    static const int shapeMap[4] = { octocid::ModLfo::Sine, octocid::ModLfo::Triangle, octocid::ModLfo::Square, octocid::ModLfo::SampleHold };
    const int oldSync = juce::jlimit (0, LfoSync::names.size() - 1, (int) p.lfoSync->load());
    int newSync = 0;
    for (int i = 0; i < octocid::ModSync::count; ++i)
        if (LfoSync::names[oldSync] == octocid::ModSync::names[i]) newSync = i;

    setReal (ModParams::lfoId (lfo, "shape"), (float) shapeMap[juce::jlimit (0, 3, (int) p.lfoShape->load())]);
    setReal (ModParams::lfoId (lfo, "sync"), (float) newSync);
    setReal (ModParams::lfoId (lfo, "rate"), p.lfoRate->load());
    setReal (ModParams::lfoId (lfo, "mode"), (float) (newSync > 0 ? octocid::ModLfo::TransportSync : octocid::ModLfo::Free));
    setReal (ModParams::lfoId (lfo, "pol"), 0.0f);

    // mêmes amplitudes que l'ancien LFO : Cutoff ±2,5 octaves, Résonance ±0,5, Pitch ±2 demi-tons (à 100 %)
    const int dest = juce::jlimit (0, 2, (int) p.lfoDest->load());
    const char* destId = dest == 0 ? "cutoff" : (dest == 1 ? "resonance" : "tune");
    const float scale = dest == 0 ? 2.5f / 2.931652f : (dest == 1 ? 0.5f : 2.0f / 24.0f);
    setReal (ModParams::slotId (slot, "src"), (float) (lfo + 1));
    setReal (ModParams::slotId (slot, "dst"), (float) ModParams::destIndex (destId));
    setReal (ModParams::slotId (slot, "amt"), depth * scale);
    setReal ("lfodepth", 0.0f);
}

void OctoCidAudioProcessor::setupModulation()
{
    for (int d = 0; d < ModParams::numDests; ++d)
    {
        modDest[(size_t) d].param = nullptr; modDest[(size_t) d].raw = nullptr;
        if (d == 0) continue;
        modDest[(size_t) d].param = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (ModParams::dests[d].id));
        modDest[(size_t) d].raw = apvts.getRawParameterValue (ModParams::dests[d].id);
        jassert (modDest[(size_t) d].param != nullptr);
    }
    for (int i = 0; i < ModParams::numLfos; ++i)
    {
        auto& r = lfoRaw[(size_t) i];
        r.shape = apvts.getRawParameterValue (ModParams::lfoId (i, "shape"));
        r.rate = apvts.getRawParameterValue (ModParams::lfoId (i, "rate"));
        r.sync = apvts.getRawParameterValue (ModParams::lfoId (i, "sync"));
        r.mode = apvts.getRawParameterValue (ModParams::lfoId (i, "mode"));
        r.phase = apvts.getRawParameterValue (ModParams::lfoId (i, "phase"));
        r.delay = apvts.getRawParameterValue (ModParams::lfoId (i, "delay"));
        r.fade = apvts.getRawParameterValue (ModParams::lfoId (i, "fade"));
        r.smooth = apvts.getRawParameterValue (ModParams::lfoId (i, "smooth"));
        r.pol = apvts.getRawParameterValue (ModParams::lfoId (i, "pol"));
        lfoRateDest[(size_t) i] = ModParams::destIndex (ModParams::lfoId (i, "rate"));
    }
    cutoffDest = ModParams::destIndex ("cutoff");
    for (int k = 0; k < ModParams::numSlots; ++k)
    {
        slotRaw[(size_t) k].src = apvts.getRawParameterValue (ModParams::slotId (k, "src"));
        slotRaw[(size_t) k].dst = apvts.getRawParameterValue (ModParams::slotId (k, "dst"));
        slotRaw[(size_t) k].amt = apvts.getRawParameterValue (ModParams::slotId (k, "amt"));
    }
}

void OctoCidAudioProcessor::resetModulation()
{
    for (int i = 0; i < ModParams::numLfos; ++i) modLfo[(size_t) i].reset ((uint32_t) (i + 1) * 7919u);
    modOffset.fill (0.0f);
    anyModulation = false;
    lastModNoteCount = noteCount.load (std::memory_order_relaxed);
}

void OctoCidAudioProcessor::computeModulation (int numSamples, double ppq, bool playing) noexcept
{
    // Une note détachée (pas un slide) relance les LFO en mode Note / Un coup / Aléatoire et leur délai / fondu
    const int nc = noteCount.load (std::memory_order_relaxed);
    const bool newNote = nc != lastModNoteCount && ! lastSlide.load (std::memory_order_relaxed);
    lastModNoteCount = nc;

    bool used[ModParams::numLfos] {};
    for (auto& sr : slotRaw)
    {
        const int src = (int) sr.src->load();
        if (src > 0 && src <= ModParams::numLfos && (int) sr.dst->load() > 0 && std::abs (sr.amt->load()) > 1.0e-4f)
            used[src - 1] = true;
    }

    for (int i = 0; i < ModParams::numLfos; ++i)
    {
        auto& r = lfoRaw[(size_t) i];
        octocid::ModLfo::Settings st;
        st.shape = (int) r.shape->load();
        st.mode = (int) r.mode->load();
        st.sync = (int) r.sync->load();
        st.rateHz = modValueIndex (lfoRateDest[(size_t) i], r.rate);
        st.phase01 = r.phase->load() / 360.0f;
        st.delaySec = r.delay->load() * 0.001f;
        st.fadeSec = r.fade->load() * 0.001f;
        st.smooth01 = r.smooth->load() * 0.01f;
        st.unipolar = r.pol->load() > 0.5f;
        if (newNote) modLfo[(size_t) i].noteOn (st);
        lfoValue[(size_t) i] = modLfo[(size_t) i].advance (st, numSamples, hostSampleRate, ppq, blockBpm, playing);
        lfoUsed[(size_t) i] = used[i];
    }

    modOffset.fill (0.0f);
    anyModulation = false;
    for (auto& sr : slotRaw)
    {
        const int src = (int) sr.src->load(), dst = (int) sr.dst->load();
        const float amt = sr.amt->load() * 0.01f;
        if (src <= 0 || src > ModParams::numLfos || dst <= 0 || dst >= ModParams::numDests || std::abs (amt) < 1.0e-6f) continue;
        modOffset[(size_t) dst] += amt * lfoValue[(size_t) (src - 1)];
        anyModulation = true;
    }
}

float OctoCidAudioProcessor::modValueIndex (int d, std::atomic<float>* raw) const noexcept
{
    const float v = raw->load();
    if (! anyModulation || d <= 0) return v;
    const float off = modOffset[(size_t) d];
    if (off == 0.0f) return v;
    auto* prm = modDest[(size_t) d].param;
    return prm->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, prm->convertTo0to1 (v) + off));
}

float OctoCidAudioProcessor::modValue (std::atomic<float>* raw) const noexcept
{
    if (! anyModulation) return raw->load();
    for (int d = 1; d < ModParams::numDests; ++d)
        if (modDest[(size_t) d].raw == raw)
            return modValueIndex (d, raw);
    return raw->load();
}

void OctoCidAudioProcessor::publishModulationView() noexcept
{
    float snail = 0.0f; int n = 0;
    for (int i = 0; i < ModParams::numLfos; ++i)
    {
        lfoValueView[(size_t) i].store (lfoValue[(size_t) i], std::memory_order_relaxed);
        lfoPhaseView[(size_t) i].store (modLfo[(size_t) i].cyclePosition(), std::memory_order_relaxed);
        if (lfoUsed[(size_t) i]) { snail += lfoValue[(size_t) i]; ++n; }
    }
    for (int d = 0; d < ModParams::numDests; ++d)
        modOffsetView[(size_t) d].store (modOffset[(size_t) d], std::memory_order_relaxed);
    // l'escargot fait onduler ses pédoncules avec les LFO utilisés (ou l'ancien LFO)
    lfoView.store (n > 0 ? snail / (float) n : voice.getLfoValue(), std::memory_order_relaxed);
}

//==============================================================================
int OctoCidAudioProcessor::getNumPrograms()
{
    const juce::ScopedLock sl (userPresetLock);
    return FactoryPresets::count + (int) userPresets.size();
}

const juce::String OctoCidAudioProcessor::getProgramName (int index)
{
    if (index >= FactoryPresets::count)
    {
        const juce::ScopedLock sl (userPresetLock);
        const int u = index - FactoryPresets::count;
        return u < (int) userPresets.size() ? userPresets[(size_t) u].name : juce::String();
    }
    if (index < 0)
        return {};
    return juce::String::fromUTF8 (FactoryPresets::list[index].name);
}

juce::String OctoCidAudioProcessor::getProgramDescription (int index) const
{
    if (index >= FactoryPresets::count)
        return juce::String::fromUTF8 ("Mon preset");
    if (index < 0)
        return {};
    return juce::String::fromUTF8 (FactoryPresets::list[index].tag);
}

void OctoCidAudioProcessor::setCurrentProgram (int index)
{
    // Le DAW renvoie parfois le programme courant après un rechargement d'état :
    // on ne l'applique pas une deuxième fois pour ne pas écraser les retouches.
    if (index < 0 || index >= getNumPrograms() || index == currentProgram)
        return;

    applyProgram (index);
}

juce::File OctoCidAudioProcessor::userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("OctoBio").getChildFile ("OctoCid").getChildFile ("Presets");
}

void OctoCidAudioProcessor::rescanUserPresets()
{
    std::vector<UserPreset> found;
    auto files = userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.ocpreset");
    files.sort();
    for (auto& f : files)
        found.push_back ({ f.getFileNameWithoutExtension(), f });
    const juce::ScopedLock sl (userPresetLock);
    userPresets = std::move (found);
}

namespace
{
    // Réglages qui restent ceux de l'utilisateur quand on change de preset (séquenceur, qualité…)
    const juce::StringArray& keptParams()
    {
        static const juce::StringArray keep { "quality", "accentthr", "seqon", "seqpattern", "seqres", "seqswing", "seqgate", "snailanim" };
        return keep;
    }
}

bool OctoCidAudioProcessor::saveUserPreset (const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty()) return false;
    juce::XmlElement xml ("OctoCidPreset");
    xml.setAttribute ("name", rawName.trim());
    xml.setAttribute ("version", JucePlugin_VersionString);
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (! keptParams().contains (rp->getParameterID()))
            {
                auto* e = xml.createNewChildElement ("Param");
                e->setAttribute ("id", rp->getParameterID());
                e->setAttribute ("value", (double) rp->convertFrom0to1 (rp->getValue()));
            }
    const auto folder = userPresetFolder();
    if (! folder.createDirectory()) return false;
    const auto file = folder.getChildFile (name + ".ocpreset");
    if (! xml.writeTo (file)) return false;
    rescanUserPresets();
    const juce::ScopedLock sl (userPresetLock);
    for (size_t i = 0; i < userPresets.size(); ++i)
        if (userPresets[i].file == file)
            currentProgram = FactoryPresets::count + (int) i;
    return true;
}

bool OctoCidAudioProcessor::deleteUserPreset (int programIndex)
{
    juce::File file;
    {
        const juce::ScopedLock sl (userPresetLock);
        const int u = programIndex - FactoryPresets::count;
        if (u < 0 || u >= (int) userPresets.size()) return false;
        file = userPresets[(size_t) u].file;
    }
    const bool ok = file.deleteFile();
    rescanUserPresets();
    if (currentProgram >= getNumPrograms()) currentProgram = 0;
    return ok;
}

void OctoCidAudioProcessor::applyProgram (int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;

    if (index >= FactoryPresets::count)
    {
        juce::File file;
        {
            const juce::ScopedLock sl (userPresetLock);
            file = userPresets[(size_t) (index - FactoryPresets::count)].file;
        }
        auto xml = juce::XmlDocument::parse (file);
        if (xml == nullptr || ! xml->hasTagName ("OctoCidPreset"))
            return;
        currentProgram = index;
        for (auto* param : getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
                if (! keptParams().contains (rp->getParameterID()))
                    rp->setValueNotifyingHost (rp->getDefaultValue());
        for (auto* e : xml->getChildWithTagNameIterator ("Param"))
            if (auto* rp = apvts.getParameter (e->getStringAttribute ("id")))
                if (! keptParams().contains (rp->getParameterID()))
                    rp->setValueNotifyingHost (rp->convertTo0to1 ((float) e->getDoubleAttribute ("value")));
        migrateOldLfo();
        return;
    }

    currentProgram = index;

    // Tout le son repart des valeurs par défaut ; la qualité et le seuil d'accent
    // restent ceux de l'utilisateur.
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (! keptParams().contains (rp->getParameterID()))
                rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& v : FactoryPresets::list[index].values)
    {
        if (v.id == nullptr)
            break;
        if (auto* rp = apvts.getParameter (v.id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (v.v));
    }
    migrateOldLfo();
}

//==============================================================================
juce::AudioProcessorEditor* OctoCidAudioProcessor::createEditor()
{
    // La palette est appliquée avant de construire les composants
    return new OctoCidAudioProcessorEditor (*this);
}

//==============================================================================
void OctoCidAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("editorWidth", editorWidth, nullptr);
    if (isUserPreset (currentProgram))
    {
        const juce::ScopedLock sl (userPresetLock);
        const int u = currentProgram - FactoryPresets::count;
        if (u < (int) userPresets.size())
            state.setProperty ("userPreset", userPresets[(size_t) u].name, nullptr);
    }

    // Les 8 patterns voyagent avec le projet
    state.removeChild (state.getChildWithName ("Patterns"), nullptr);
    state.appendChild (octocid::PatternIO::toTree (getBank()), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OctoCidAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = juce::jlimit (0, FactoryPresets::count - 1, (int) tree.getProperty ("program", 0));
            editorWidth = (int) tree.getProperty ("editorWidth", editorWidth);
            const auto userName = tree.getProperty ("userPreset").toString();
            if (userName.isNotEmpty())
            {
                const juce::ScopedLock sl (userPresetLock);
                for (size_t i = 0; i < userPresets.size(); ++i)
                    if (userPresets[i].name == userName)
                        currentProgram = FactoryPresets::count + (int) i;
            }

            auto bank = getBank();
            if (octocid::PatternIO::fromTree (tree.getChildWithName ("Patterns"), bank))
                setBank (bank);
            tree.removeChild (tree.getChildWithName ("Patterns"), nullptr);

            apvts.replaceState (tree);
            migrateOldLfo();     // projet d'avant la v1.4 : l'ancien LFO devient un LFO assignable
        }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OctoCidAudioProcessor();
}

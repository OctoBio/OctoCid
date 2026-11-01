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
    auto percent = [&] (const ParameterID& id, const char* name, float def)
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
    octocid::VoiceParams v;
    v.tune      = p.tune->load();
    v.cutoff    = p.cutoff->load()    * 0.01f;
    v.resonance = p.resonance->load() * 0.01f;
    v.envMod    = p.envMod->load()    * 0.01f;
    v.decay     = p.decay->load()     * 0.01f;
    v.accent    = p.accent->load()    * 0.01f;
    v.squareMix = p.waveform->load() > 0.5f ? 1.0f : 0.0f;   // fondu lissé dans la voix

    v.slideTimeMs   = p.slideTime->load();
    v.sweepSpeed    = juce::jlimit (0, 2, (int) p.sweepSpeed->load());
    v.slidePunch    = p.slidePunch->load() * 0.01f;
    v.accentDecayMs = p.accentDecay->load();
    v.softAttackMs  = p.softAttack->load();
    v.filterTrack   = p.filterTrack->load() * 0.01f;
    v.filterFm      = p.filterFm->load() * 0.01f;
    v.filterDrive   = p.filterDrive->load() * 0.01f;
    v.subMode       = juce::jlimit (0, 2, (int) p.subOsc->load());
    v.subLevel      = p.subLevel->load() * 0.01f;
    v.muffler       = juce::jlimit (0, 2, (int) p.muffler->load());
    v.analog        = p.analog->load() * 0.01f;

    v.lfoShape  = juce::jlimit (0, 3, (int) p.lfoShape->load());
    v.lfoDest   = juce::jlimit (0, 2, (int) p.lfoDest->load());
    v.lfoDepth  = p.lfoDepth->load() * 0.01f;
    v.lfoRateHz = p.lfoRate->load();

    // LFO synchronisé : fréquence tirée du tempo du DAW
    const int sync = juce::jlimit (0, LfoSync::names.size() - 1, (int) p.lfoSync->load());
    if (sync > 0)
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm())
                    bpm = *b;
        v.lfoRateHz = (float) (bpm / 60.0 / LfoSync::beats[sync]);
    }

    voice.setParams (v);
    voice.setAccentThreshold ((int) p.accentThr->load());

    drive.setParams ((int) p.driveType->load(), p.driveAmt->load() * 0.01f,
                     p.driveTone->load() * 0.01f, p.driveMix->load() * 0.01f);

    const bool pre = p.drivePos->load() > 0.5f;
    if (pre != drivePreVca)
    {
        drivePreVca = pre;
        drive.reset();               // les états du Drive ne valent rien d'un emplacement à l'autre
    }
    voice.setPreVcaDrive (drivePreVca ? &drive : nullptr);

    // Punch + FX (fréquence de l'hôte)
    punch.setAmount (p.punchAmt->load() * 0.01f);
    punch.setAttackMs (p.punchAttack->load());
    reverb.setParams (p.reverbMix->load() * 0.01f, p.reverbDecay->load() * 0.01f, p.reverbTone->load() * 0.01f);
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm())
                    bpm = *b;
        const int t = juce::jlimit (0, DelaySync::names.size() - 1, (int) p.delayTime->load());
        delay.setParams ((float) (DelaySync::beats[t] * 60.0 / juce::jlimit (20.0, 400.0, bpm)),
                         p.delayFb->load() * 0.01f, p.delayMix->load() * 0.01f);
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
    sequencer.setGate (p.seqGate->load() * 0.01);

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

    updateParams();
    updateLfoSync();
    buildEvents (midi, numSamples);

    float* left = buffer.getWritePointer (0);
    for (int pos = 0; pos < numSamples; pos += maxBlockSize)
        renderChunk (left + pos, juce::jmin (maxBlockSize, numSamples - pos), pos);
        lfoView.store (voice.getLfoValue(), std::memory_order_relaxed);

    // Punch (mono), puis delay et reverb (stéréo), puis volume et limiteur de sécurité
    const bool stereo = buffer.getNumChannels() > 1;
    float* right = stereo ? buffer.getWritePointer (1) : nullptr;

    const bool usePunch = punch.isActive();
    const bool useDelay = delay.isActive();
    const bool useReverb = reverb.isActive();

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (p.volume->load(), -60.0f) + 1.0e-6f);
    for (int i = 0; i < numSamples; ++i)
    {
        float l = left[i];
        if (usePunch)
            l = punch.process (l);
        float r = l;
        if (useDelay)  delay.process (l, r);
        if (useReverb) reverb.process (l, r);

        const float g = outputGain.getNextValue();
        l = safetyLimit (l * g);
        r = safetyLimit (r * g);

        if (stereo)
        {
            left[i] = l;
            right[i] = r;
        }
        else
            left[i] = 0.5f * (l + r);   // sortie mono : on replie la stéréo des FX
    }

    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

//==============================================================================
int OctoCidAudioProcessor::getNumPrograms()
{
    return FactoryPresets::count;
}

const juce::String OctoCidAudioProcessor::getProgramName (int index)
{
    if (index < 0 || index >= FactoryPresets::count)
        return {};
    return juce::String::fromUTF8 (FactoryPresets::list[index].name);
}

juce::String OctoCidAudioProcessor::getProgramDescription (int index) const
{
    if (index < 0 || index >= FactoryPresets::count)
        return {};
    return juce::String::fromUTF8 (FactoryPresets::list[index].tag);
}

void OctoCidAudioProcessor::setCurrentProgram (int index)
{
    // Le DAW renvoie parfois le programme courant après un rechargement d'état :
    // on ne l'applique pas une deuxième fois pour ne pas écraser les retouches.
    if (index < 0 || index >= FactoryPresets::count || index == currentProgram)
        return;

    applyProgram (index);
}

void OctoCidAudioProcessor::applyProgram (int index)
{
    if (index < 0 || index >= FactoryPresets::count)
        return;

    currentProgram = index;

    // Tout le son repart des valeurs par défaut ; la qualité et le seuil d'accent
    // restent ceux de l'utilisateur.
    static const juce::StringArray keep { "quality", "accentthr", "seqon", "seqpattern", "seqres", "seqswing", "seqgate", "snailanim" };
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (! keep.contains (rp->getParameterID()))
                rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& v : FactoryPresets::list[index].values)
    {
        if (v.id == nullptr)
            break;
        if (auto* rp = apvts.getParameter (v.id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (v.v));
    }
}

//==============================================================================
juce::AudioProcessorEditor* OctoCidAudioProcessor::createEditor()
{
    // La palette est appliquée avant de construire les composants
    OctoColours::applyTheme (OctoPrefs::getInt ("theme", 2));
    OctoLook::apply (OctoPrefs::getInt ("look", OctoLook::defaultLook));
    OctoDecor::apply (OctoPrefs::getInt ("decor", OctoDecor::defaultDecor));
    return new OctoCidAudioProcessorEditor (*this);
}

//==============================================================================
void OctoCidAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);

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

            auto bank = getBank();
            if (octocid::PatternIO::fromTree (tree.getChildWithName ("Patterns"), bank))
                setBank (bank);
            tree.removeChild (tree.getChildWithName ("Patterns"), nullptr);

            apvts.replaceState (tree);
        }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OctoCidAudioProcessor();
}

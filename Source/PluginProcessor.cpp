#include "PluginProcessor.h"
#include "PluginEditor.h"
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

void OctoCidAudioProcessor::handleMidi (const juce::MidiMessage& msg) noexcept
{
    if (msg.isNoteOn())
    {
        voice.noteOn (msg.getNoteNumber(), msg.getVelocity());
        lastNote.store (msg.getNoteNumber(), std::memory_order_relaxed);
        lastAccent.store (voice.isAccent(), std::memory_order_relaxed);
    }
    else if (msg.isNoteOff())
    {
        voice.noteOff (msg.getNoteNumber());
        if (! voice.isGateOn())
            lastNote.store (-1, std::memory_order_relaxed);
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        voice.allNotesOff();
        lastNote.store (-1, std::memory_order_relaxed);
    }
}

// Rend numSamples échantillons (≤ maxBlockSize) en traitant le MIDI au sample près
void OctoCidAudioProcessor::renderChunk (float* out, int numSamples, const juce::MidiBuffer& midi, int startSample) noexcept
{
    auto& os = *oversamplers[currentQuality];
    const int factor = (int) os.getOversamplingFactor();

    juce::dsp::AudioBlock<float> mono (monoBuffer.getArrayOfWritePointers(), 1, (size_t) numSamples);
    mono.clear();

    auto up = os.processSamplesUp (mono);
    float* osData = up.getChannelPointer (0);

    auto it = midi.findNextSamplePosition (startSample);
    const int endSample = startSample + numSamples;

    for (int i = 0; i < numSamples; ++i)
    {
        while (it != midi.cend() && (*it).samplePosition <= startSample + i && (*it).samplePosition < endSample)
        {
            handleMidi ((*it).getMessage());
            ++it;
        }

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

    float* left = buffer.getWritePointer (0);
    for (int pos = 0; pos < numSamples; pos += maxBlockSize)
        renderChunk (left + pos, juce::jmin (maxBlockSize, numSamples - pos), midi, pos);

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
    static const juce::StringArray keep { "quality", "accentthr" };
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
    return new OctoCidAudioProcessorEditor (*this);
}

//==============================================================================
void OctoCidAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
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
            apvts.replaceState (tree);
        }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OctoCidAudioProcessor();
}

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OctoCidAudioProcessor::OctoCidAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "OctoCidState", createParameterLayout())
{
    pTune      = apvts.getRawParameterValue (ParamIDs::tune.getParamID());
    pCutoff    = apvts.getRawParameterValue (ParamIDs::cutoff.getParamID());
    pResonance = apvts.getRawParameterValue (ParamIDs::resonance.getParamID());
    pEnvMod    = apvts.getRawParameterValue (ParamIDs::envMod.getParamID());
    pDecay     = apvts.getRawParameterValue (ParamIDs::decay.getParamID());
    pAccent    = apvts.getRawParameterValue (ParamIDs::accent.getParamID());
    pWaveform  = apvts.getRawParameterValue (ParamIDs::waveform.getParamID());
    pVolume    = apvts.getRawParameterValue (ParamIDs::volume.getParamID());
    pAccentThr = apvts.getRawParameterValue (ParamIDs::accentThreshold.getParamID());
    pQuality   = apvts.getRawParameterValue (ParamIDs::quality.getParamID());
    pSlideTime  = apvts.getRawParameterValue (ParamIDs::slideTime.getParamID());
    pSweepSpeed = apvts.getRawParameterValue (ParamIDs::sweepSpeed.getParamID());
    pSlidePunch = apvts.getRawParameterValue (ParamIDs::slidePunch.getParamID());
}

juce::AudioProcessorValueTreeState::ParameterLayout OctoCidAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = AudioParameterFloatAttributes().withLabel ("%");

    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::tune, "Tune",
        NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("st")));

    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::cutoff,    "Cutoff",    NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::resonance, "Resonance", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::envMod,    "Env Mod",   NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::decay,     "Decay",     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::accent,    "Accent",    NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, pct));

    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::waveform, "Waveform",
        StringArray { "Saw", "Square" }, 0));

    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::volume, "Volume",
        NormalisableRange<float> (-48.0f, 6.0f, 0.1f), -3.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<AudioParameterInt> (ParamIDs::accentThreshold, "Accent Velocity", 1, 127, 100));

    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::quality, "Quality",
        StringArray { "Normal (2x)", "High (4x)" }, 0));

    // Slime Mods (Devil Fish) — à leur réglage par défaut, le son est celui de l'original
    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::slideTime, "Slide Time",
        NormalisableRange<float> (60.0f, 360.0f, 1.0f), 60.0f,
        AudioParameterFloatAttributes().withLabel ("ms")));

    layout.add (std::make_unique<AudioParameterChoice> (ParamIDs::sweepSpeed, "Sweep Speed",
        StringArray { "Slow", "Normal", "Fast" }, 1));

    layout.add (std::make_unique<AudioParameterFloat> (ParamIDs::slidePunch, "Slide Punch",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, pct));

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

    currentQuality = (int) pQuality->load();
    voice.prepare (sampleRate * (double) oversamplers[currentQuality]->getOversamplingFactor());
    updateVoiceParams();
    voice.reset();
    toneShaper.prepare (sampleRate);

    outputGain.reset (sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pVolume->load()));

    setLatencySamples ((int) std::round (oversamplers[currentQuality]->getLatencyInSamples()));
}

bool OctoCidAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

//==============================================================================
void OctoCidAudioProcessor::updateVoiceParams() noexcept
{
    octocid::VoiceParams p;
    p.tune      = pTune->load();
    p.cutoff    = pCutoff->load()    * 0.01f;
    p.resonance = pResonance->load() * 0.01f;
    p.envMod    = pEnvMod->load()    * 0.01f;
    p.decay     = pDecay->load()     * 0.01f;
    p.accent    = pAccent->load()    * 0.01f;
    p.squareMix = pWaveform->load() > 0.5f ? 1.0f : 0.0f;   // fondu lissé dans la voix
    p.slideTimeMs = pSlideTime->load();
    p.sweepSpeed  = juce::jlimit (0, 2, (int) pSweepSpeed->load());
    p.slidePunch  = pSlidePunch->load() * 0.01f;
    voice.setParams (p);
    voice.setAccentThreshold ((int) pAccentThr->load());
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

    // On récupère le tampon suréchantillonné et on y écrit directement la voix
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
            osData[i * factor + j] = voice.process();
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

    // Changement de qualité : la voix passe à la nouvelle fréquence sans couper le son
    const int q = (int) pQuality->load();
    if (q != currentQuality)
    {
        currentQuality = q;
        oversamplers[q]->reset();
        voice.setSampleRate (hostSampleRate * (double) oversamplers[q]->getOversamplingFactor());
    }

    updateVoiceParams();

    float* left = buffer.getWritePointer (0);
    for (int pos = 0; pos < numSamples; pos += maxBlockSize)
        renderChunk (left + pos, juce::jmin (maxBlockSize, numSamples - pos), midi, pos);

    // Volume lissé
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (pVolume->load(), -60.0f) + 1.0e-6f);
    for (int i = 0; i < numSamples; ++i)
        left[i] *= outputGain.getNextValue();

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
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
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OctoCidAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OctoCidAudioProcessor();
}

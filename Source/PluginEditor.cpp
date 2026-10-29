#include "PluginEditor.h"

namespace OctoColours
{
    // Palette provisoire (variantes à valider avant l'étape 5)
    static const juce::Colour background { 0xff2b2826 };   // anthracite chaud
    static const juce::Colour cream      { 0xffefe6d2 };   // crème
    static const juce::Colour slime      { 0xff9be22d };   // vert bave
    static const juce::Colour dim        { 0xff5a554f };
}

//==============================================================================
OctoCidAudioProcessorEditor::OctoCidAudioProcessorEditor (OctoCidAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (1000, 620);
    setResizable (false, false);
    ledArea = { 470, 380, 60, 60 };
    startTimerHz (30);
}

void OctoCidAudioProcessorEditor::timerCallback()
{
    const int note = processor.lastNote.load (std::memory_order_relaxed);
    if (note != displayedNote)
    {
        displayedNote = note;
        repaint (ledArea.expanded (120, 40));   // on ne redessine que cette zone
    }
}

void OctoCidAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (OctoColours::background);

    // Titre
    g.setColour (OctoColours::cream);
    g.setFont (juce::FontOptions (64.0f, juce::Font::bold));
    g.drawText ("OctoCid", getLocalBounds().withHeight (300), juce::Justification::centred);

    g.setColour (OctoColours::dim);
    g.setFont (juce::FontOptions (16.0f));
    g.drawText ("v" JucePlugin_VersionString " - squelette (etape 1)",
                getLocalBounds().withTrimmedTop (200).withHeight (100),
                juce::Justification::centred);

    // LED MIDI : s'allume en vert bave quand une note est tenue
    const bool on = displayedNote >= 0;
    g.setColour (on ? OctoColours::slime : OctoColours::dim);
    g.fillEllipse (ledArea.toFloat().reduced (14.0f));

    g.setColour (on ? OctoColours::cream : OctoColours::dim);
    g.setFont (juce::FontOptions (15.0f));
    const juce::String label = on ? "Note " + juce::MidiMessage::getMidiNoteName (displayedNote, true, true, 3)
                                  : juce::String ("En attente de MIDI");
    g.drawText (label, ledArea.withY (ledArea.getBottom()).withHeight (30).expanded (120, 0),
                juce::Justification::centred);
}

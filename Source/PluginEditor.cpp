#include "PluginEditor.h"

namespace OctoColours
{
    // Palette provisoire (variantes à valider avant l'étape 5)
    static const juce::Colour background { 0xff2b2826 };   // anthracite chaud
    static const juce::Colour panel      { 0xff35312e };
    static const juce::Colour cream      { 0xffefe6d2 };   // crème
    static const juce::Colour slime      { 0xff9be22d };   // vert bave
    static const juce::Colour dim        { 0xff5a554f };
}

//==============================================================================
OctoCidAudioProcessorEditor::OctoCidAudioProcessorEditor (OctoCidAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    using namespace OctoColours;
    lookAndFeel.setColour (juce::Slider::rotarySliderFillColourId, slime);
    lookAndFeel.setColour (juce::Slider::rotarySliderOutlineColourId, dim);
    lookAndFeel.setColour (juce::Slider::thumbColourId, cream);
    lookAndFeel.setColour (juce::Slider::textBoxTextColourId, cream);
    lookAndFeel.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    lookAndFeel.setColour (juce::Label::textColourId, cream);
    lookAndFeel.setColour (juce::ComboBox::backgroundColourId, panel);
    lookAndFeel.setColour (juce::ComboBox::textColourId, cream);
    lookAndFeel.setColour (juce::ComboBox::outlineColourId, dim);
    lookAndFeel.setColour (juce::ComboBox::arrowColourId, slime);
    lookAndFeel.setColour (juce::PopupMenu::backgroundColourId, panel);
    lookAndFeel.setColour (juce::PopupMenu::textColourId, cream);
    lookAndFeel.setColour (juce::PopupMenu::highlightedBackgroundColourId, slime.withAlpha (0.35f));
    setLookAndFeel (&lookAndFeel);

    setupKnob (tune,      ParamIDs::tune,      "Tune");
    setupKnob (cutoff,    ParamIDs::cutoff,    "Cutoff");
    setupKnob (resonance, ParamIDs::resonance, "Resonance");
    setupKnob (envMod,    ParamIDs::envMod,    "Env Mod");
    setupKnob (decay,     ParamIDs::decay,     "Decay");
    setupKnob (accent,    ParamIDs::accent,    "Accent");
    setupKnob (volume,    ParamIDs::volume,    "Volume");
    setupKnob (accentThr, ParamIDs::accentThreshold, "Seuil accent");

    setupChoice (waveform, ParamIDs::waveform, "Waveform");
    setupChoice (quality,  ParamIDs::quality,  "Qualite");

    setSize (1000, 620);
    setResizable (false, false);
    startTimerHz (30);
}

OctoCidAudioProcessorEditor::~OctoCidAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void OctoCidAudioProcessorEditor::setupKnob (Knob& k, const juce::ParameterID& id, const juce::String& name)
{
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    k.slider.setDoubleClickReturnValue (true, 0.0);   // la vraie valeur par défaut est fixée par l'attachment
    addAndMakeVisible (k.slider);

    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, id.getParamID(), k.slider);

    // Double-clic = valeur par défaut du paramètre
    if (auto* param = processor.apvts.getParameter (id.getParamID()))
        k.slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
}

void OctoCidAudioProcessorEditor::setupChoice (Choice& c, const juce::ParameterID& id, const juce::String& name)
{
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (id.getParamID())))
        c.box.addItemList (param->choices, 1);
    addAndMakeVisible (c.box);

    c.label.setText (name, juce::dontSendNotification);
    c.label.setJustificationType (juce::Justification::centred);
    c.label.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    addAndMakeVisible (c.label);

    c.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, id.getParamID(), c.box);
}

//==============================================================================
void OctoCidAudioProcessorEditor::resized()
{
    // Rangée principale : 8 potards façon panneau acid
    auto row = juce::Rectangle<int> (30, 230, 940, 150);
    const int w = row.getWidth() / 8;

    Knob* main[] = { &tune, &cutoff, &resonance, &envMod, &decay, &accent };
    int i = 0;
    for (auto* k : main)
    {
        auto cell = row.withX (row.getX() + w * i++).withWidth (w).reduced (8, 0);
        k->label.setBounds (cell.removeFromTop (24));
        k->slider.setBounds (cell);
    }

    // Waveform (Saw/Square) en 7e position, Volume en 8e
    {
        auto cell = row.withX (row.getX() + w * 6).withWidth (w).reduced (8, 0);
        waveform.label.setBounds (cell.removeFromTop (24));
        waveform.box.setBounds (cell.withSizeKeepingCentre (w - 20, 28));
    }
    {
        auto cell = row.withX (row.getX() + w * 7).withWidth (w).reduced (8, 0);
        volume.label.setBounds (cell.removeFromTop (24));
        volume.slider.setBounds (cell);
    }

    // Rangée réglages : seuil d'accent (vélocité) et qualité
    auto row2 = juce::Rectangle<int> (30, 420, 940, 130);
    {
        auto cell = row2.withWidth (w).reduced (8, 0);
        accentThr.label.setBounds (cell.removeFromTop (24));
        accentThr.slider.setBounds (cell);
    }
    {
        auto cell = row2.withX (row2.getX() + w).withWidth (w + 40).reduced (8, 0);
        quality.label.setBounds (cell.removeFromTop (24));
        quality.box.setBounds (cell.withSizeKeepingCentre (w + 20, 28));
    }

    ledArea = { 870, 60, 60, 60 };
}

void OctoCidAudioProcessorEditor::timerCallback()
{
    const int note = processor.lastNote.load (std::memory_order_relaxed);
    const bool acc = processor.lastAccent.load (std::memory_order_relaxed);
    if (note != displayedNote || acc != displayedAccent)
    {
        displayedNote = note;
        displayedAccent = acc;
        repaint (ledArea.expanded (90, 30));   // on ne redessine que cette zone
    }
}

void OctoCidAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace OctoColours;
    g.fillAll (background);

    // Titre
    g.setColour (cream);
    g.setFont (juce::FontOptions (54.0f, juce::Font::bold));
    g.drawText ("OctoCid", juce::Rectangle<int> (40, 40, 400, 70), juce::Justification::centredLeft);

    g.setColour (dim);
    g.setFont (juce::FontOptions (15.0f));
    g.drawText ("v" JucePlugin_VersionString " - moteur sonore (etape 2) - potards provisoires",
                juce::Rectangle<int> (42, 105, 600, 24), juce::Justification::centredLeft);

    // Panneaux
    g.setColour (panel);
    g.fillRoundedRectangle (juce::Rectangle<float> (20, 215, 960, 175), 14.0f);
    g.fillRoundedRectangle (juce::Rectangle<float> (20, 405, 290, 155), 14.0f);

    g.setColour (dim);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("Clavier : notes qui se chevauchent = slide, velocite >= seuil = accent",
                juce::Rectangle<int> (330, 470, 640, 24), juce::Justification::centredLeft);

    // LED MIDI : verte sur une note, crème plus vive sur un accent
    const bool on = displayedNote >= 0;
    g.setColour (on ? (displayedAccent ? cream : slime) : dim);
    g.fillEllipse (ledArea.toFloat().reduced (14.0f));
    if (on && displayedAccent)
    {
        g.setColour (slime);
        g.drawEllipse (ledArea.toFloat().reduced (10.0f), 3.0f);
    }

    g.setColour (on ? cream : dim);
    g.setFont (juce::FontOptions (14.0f));
    juce::String label ("En attente de MIDI");
    if (on)
        label = juce::MidiMessage::getMidiNoteName (displayedNote, true, true, 3) + (displayedAccent ? "  ACCENT" : "");
    g.drawText (label, ledArea.withY (ledArea.getBottom()).withHeight (24).expanded (90, 0),
                juce::Justification::centred);
}

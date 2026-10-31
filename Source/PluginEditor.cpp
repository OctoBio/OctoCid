#include "PluginEditor.h"

namespace OctoColours
{
    // Palette provisoire (variantes à valider avant l'étape 5)
    static const juce::Colour background { 0xff2b2826 };   // anthracite chaud
    static const juce::Colour panel      { 0xff35312e };
    static const juce::Colour cream      { 0xffefe6d2 };   // crème
    static const juce::Colour slime      { 0xff9be22d };   // vert bave
    static const juce::Colour dim        { 0xff6b655e };
}

//==============================================================================
OctoCidAudioProcessorEditor::OctoCidAudioProcessorEditor (OctoCidAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    using namespace OctoColours;
    lookAndFeel.setColour (juce::Slider::rotarySliderFillColourId, slime);
    lookAndFeel.setColour (juce::Slider::rotarySliderOutlineColourId, dim.darker (0.3f));
    lookAndFeel.setColour (juce::Slider::thumbColourId, cream);
    lookAndFeel.setColour (juce::Slider::textBoxTextColourId, cream);
    lookAndFeel.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    lookAndFeel.setColour (juce::Label::textColourId, cream);
    lookAndFeel.setColour (juce::ComboBox::backgroundColourId, background);
    lookAndFeel.setColour (juce::ComboBox::textColourId, cream);
    lookAndFeel.setColour (juce::ComboBox::outlineColourId, dim);
    lookAndFeel.setColour (juce::ComboBox::arrowColourId, slime);
    lookAndFeel.setColour (juce::PopupMenu::backgroundColourId, panel);
    lookAndFeel.setColour (juce::PopupMenu::textColourId, cream);
    lookAndFeel.setColour (juce::PopupMenu::highlightedBackgroundColourId, slime.withAlpha (0.35f));
    lookAndFeel.setColour (juce::TextButton::buttonColourId, background);
    lookAndFeel.setColour (juce::TextButton::textColourOffId, slime);
    setLookAndFeel (&lookAndFeel);

    using namespace ParamIDs;

    // --- Sections
    sections.push_back ({ "303", { 12, 84, 976, 132 }, false,
        { { knob (tune, "Tune"), knob (cutoff, "Cutoff"), knob (resonance, "Resonance"), knob (envMod, "Env Mod"),
            knob (decay, "Decay"), knob (accent, "Accent"), choice (waveform, "Waveform"), knob (volume, "Volume") } } });

    sections.push_back ({ "DRIVE", { 12, 224, 480, 132 }, false,
        { { choice (driveType, "Type"), knob (driveAmt, "Drive"), knob (driveTone, "Tone"), knob (driveMix, "Mix"),
            choice (drivePos, "Position") } } });

    sections.push_back ({ "LFO", { 500, 224, 488, 132 }, false,
        { { choice (lfoSync, "Sync"), knob (lfoRate, "Rate"), knob (lfoDepth, "Depth"),
            choice (lfoShape, "Forme"), choice (lfoDest, "Destination") } } });

    sections.push_back ({ "PUNCH + FX", { 12, 364, 976, 132 }, false,
        { { knob (punch, "Punch"), knob (punchAttack, "Attaque"),
            knob (delayMix, "Delay"), choice (delayTime, "Temps"), knob (delayFb, "Feedback"),
            knob (reverbMix, "Reverb"), knob (reverbDecay, "Decay"), knob (reverbTone, "Tone") } } });

    sections.push_back ({ "SLIME MODS", { 12, 504, 976, 244 }, true,
        { { knob (slideTime, "Slide Time"), knob (slidePunch, "Slide Punch"), choice (sweepSpeed, "Sweep Speed"),
            knob (accentDecay, "Accent Decay"), knob (softAttack, "Soft Attack"), knob (accentThreshold, "Seuil accent") },
          { knob (filterTrack, "Filter Track"), knob (filterFm, "Filter FM"), knob (filterDrive, "Filter Drive"),
            choice (subOsc, "Sub Osc"), knob (subLevel, "Sub Level"), choice (muffler, "Muffler") } } });

    // --- Barre du haut : presets et qualité
    for (int i = 0; i < processor.getNumPrograms(); ++i)
        presetBox.addItem (juce::String (i + 1).paddedLeft ('0', 2) + "  " + processor.getProgramName (i), i + 1);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0 && idx != processor.getCurrentProgram())
        {
            processor.applyProgram (idx);
            processor.updateHostDisplay (juce::AudioProcessor::ChangeDetails().withProgramChanged (true));
        }
        refreshPresetBox();
    };
    auto step = [this] (int delta)
    {
        const int n = processor.getNumPrograms();
        const int idx = (processor.getCurrentProgram() + delta + n) % n;
        processor.applyProgram (idx);
        processor.updateHostDisplay (juce::AudioProcessor::ChangeDetails().withProgramChanged (true));
        refreshPresetBox();
    };
    prevButton.onClick = [step] { step (-1); };
    nextButton.onClick = [step] { step (+1); };
    addAndMakeVisible (presetBox);
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);

    presetInfo.setFont (juce::FontOptions (13.0f));
    presetInfo.setColour (juce::Label::textColourId, dim.brighter (0.4f));
    addAndMakeVisible (presetInfo);

    auto* q = choice (quality, "Qualite");
    q->label.setJustificationType (juce::Justification::centredRight);

    refreshPresetBox();

    setSize (1000, 760);   // provisoire : la vraie interface (étape 5) revient à ~1000 x 620
    setResizable (false, false);
    startTimerHz (30);
}

OctoCidAudioProcessorEditor::~OctoCidAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
OctoCidAudioProcessorEditor::Control* OctoCidAudioProcessorEditor::knob (const juce::ParameterID& id, const juce::String& name)
{
    auto c = std::make_unique<Control>();
    c->slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
    c->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 18);
    addAndMakeVisible (*c->slider);

    c->label.setText (name, juce::dontSendNotification);
    c->label.setJustificationType (juce::Justification::centred);
    c->label.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (c->label);

    c->sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, id.getParamID(), *c->slider);

    // Double-clic = valeur par défaut, Ctrl + glisser = réglage fin (géré par JUCE)
    if (auto* param = processor.apvts.getParameter (id.getParamID()))
        c->slider->setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    controls.push_back (std::move (c));
    return controls.back().get();
}

OctoCidAudioProcessorEditor::Control* OctoCidAudioProcessorEditor::choice (const juce::ParameterID& id, const juce::String& name)
{
    auto c = std::make_unique<Control>();
    c->box = std::make_unique<juce::ComboBox>();
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (id.getParamID())))
        c->box->addItemList (param->choices, 1);
    addAndMakeVisible (*c->box);

    c->label.setText (name, juce::dontSendNotification);
    c->label.setJustificationType (juce::Justification::centred);
    c->label.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (c->label);

    c->boxAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, id.getParamID(), *c->box);

    controls.push_back (std::move (c));
    return controls.back().get();
}

void OctoCidAudioProcessorEditor::refreshPresetBox()
{
    const int idx = processor.getCurrentProgram();
    displayedProgram = idx;
    presetBox.setSelectedItemIndex (idx, juce::dontSendNotification);
    presetInfo.setText (processor.getProgramDescription (idx), juce::dontSendNotification);
}

//==============================================================================
void OctoCidAudioProcessorEditor::layoutRow (juce::Rectangle<int> area, const std::vector<Control*>& row)
{
    if (row.empty())
        return;

    const int w = area.getWidth() / (int) row.size();
    for (size_t i = 0; i < row.size(); ++i)
    {
        auto cell = area.withX (area.getX() + w * (int) i).withWidth (w).reduced (4, 0);
        auto* c = row[i];
        c->label.setBounds (cell.removeFromTop (20));
        if (c->slider != nullptr)
            c->slider->setBounds (cell);
        else
            c->box->setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth() - 6, 120), 26));
    }
}

void OctoCidAudioProcessorEditor::resized()
{
    for (auto& s : sections)
    {
        auto inner = s.bounds.reduced (8, 0).withTrimmedTop (22).withTrimmedBottom (6);
        const int rowH = inner.getHeight() / (int) s.rows.size();
        for (size_t r = 0; r < s.rows.size(); ++r)
            layoutRow (inner.withY (inner.getY() + rowH * (int) r).withHeight (rowH), s.rows[r]);
    }

    // Barre du haut
    prevButton.setBounds (230, 18, 28, 28);
    presetBox.setBounds (262, 18, 260, 28);
    nextButton.setBounds (526, 18, 28, 28);
    presetInfo.setBounds (230, 50, 420, 22);

    auto& q = *controls.back();   // Qualité (dernier contrôle créé)
    q.label.setBounds (600, 18, 70, 28);
    q.box->setBounds (674, 18, 120, 28);

    ledArea = { 880, 12, 40, 40 };
}

void OctoCidAudioProcessorEditor::timerCallback()
{
    const int note = processor.lastNote.load (std::memory_order_relaxed);
    const bool acc = processor.lastAccent.load (std::memory_order_relaxed);
    if (note != displayedNote || acc != displayedAccent)
    {
        displayedNote = note;
        displayedAccent = acc;
        repaint (ledArea.expanded (70, 30));   // on ne redessine que cette zone
    }

    if (processor.getCurrentProgram() != displayedProgram)
        refreshPresetBox();
}

void OctoCidAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace OctoColours;
    g.fillAll (background);

    // Titre
    g.setColour (cream);
    g.setFont (juce::FontOptions (40.0f, juce::Font::bold));
    g.drawText ("OctoCid", juce::Rectangle<int> (20, 12, 200, 46), juce::Justification::centredLeft);
    g.setColour (dim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("v" JucePlugin_VersionString " - potards provisoires", juce::Rectangle<int> (22, 54, 200, 18),
                juce::Justification::centredLeft);

    // Sections
    for (auto& s : sections)
    {
        auto r = s.bounds.toFloat();
        g.setColour (s.slime ? slime.withAlpha (0.10f) : panel);
        g.fillRoundedRectangle (r, 12.0f);
        g.setColour (s.slime ? slime : dim.brighter (0.3f));
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (s.title, s.bounds.reduced (12, 4).withHeight (16), juce::Justification::centredLeft);
    }

    // LED MIDI : verte sur une note, crème cerclée sur un accent
    const bool on = displayedNote >= 0;
    g.setColour (on ? (displayedAccent ? cream : slime) : dim.darker (0.3f));
    g.fillEllipse (ledArea.toFloat().reduced (8.0f));
    if (on && displayedAccent)
    {
        g.setColour (slime);
        g.drawEllipse (ledArea.toFloat().reduced (5.0f), 3.0f);
    }

    g.setColour (on ? cream : dim);
    g.setFont (juce::FontOptions (12.0f));
    juce::String label ("MIDI");
    if (on)
        label = juce::MidiMessage::getMidiNoteName (displayedNote, true, true, 3) + (displayedAccent ? " ACC" : "");
    g.drawText (label, ledArea.withY (ledArea.getBottom()).withHeight (18).expanded (60, 0),
                juce::Justification::centred);
}

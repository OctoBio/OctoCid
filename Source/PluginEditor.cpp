#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
    // Charge le fond illustré du décor courant (intégré au plugin), une seule fois par image
    void loadDecorImages()
    {
        if (! octoui::logoImage.isValid())
            octoui::logoImage = juce::ImageCache::getFromMemory (BinaryData::logo_glitch_png, BinaryData::logo_glitch_pngSize);
        const char* name = OctoDecor::get().image;
        if (name == nullptr) { OctoDecor::bgImage = {}; OctoDecor::bgBlur = {}; OctoDecor::bgSoft = {}; return; }
        int size = 0;
        if (auto* data = BinaryData::getNamedResource ((juce::String (name) + "_jpg").toRawUTF8(), size))
            OctoDecor::bgImage = juce::ImageCache::getFromMemory (data, size);
        if (auto* data = BinaryData::getNamedResource ((juce::String (name) + "_blur_jpg").toRawUTF8(), size))
            OctoDecor::bgBlur = juce::ImageCache::getFromMemory (data, size);
        if (auto* data = BinaryData::getNamedResource ((juce::String (name) + "_soft_jpg").toRawUTF8(), size))
            OctoDecor::bgSoft = juce::ImageCache::getFromMemory (data, size);
    }
}

namespace
{
    // Menu déroulant étiqueté (petits choix du tiroir)
    class LabeledCombo : public juce::Component
    {
    public:
        explicit LabeledCombo (const juce::String& n) : name (n) { addAndMakeVisible (box); }
        void resized() override
        {
            auto r = getLocalBounds();
            r.removeFromBottom (16);
            box.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 4, 92), 24));
        }
        void paint (juce::Graphics& g) override
        {
            g.setColour (OctoColours::cream.withAlpha (0.85f));
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawFittedText (name, getLocalBounds().removeFromBottom (16), juce::Justification::centred, 1, 0.8f);
        }
        juce::ComboBox box;
        juce::String name;
    };

    void layoutGrid (juce::Rectangle<int> area, std::vector<std::unique_ptr<juce::Component>>& items, int perRow)
    {
        const int rows = ((int) items.size() + perRow - 1) / perRow;
        const int rowH = area.getHeight() / juce::jmax (1, rows);
        const int cellW = area.getWidth() / perRow;
        for (size_t i = 0; i < items.size(); ++i)
        {
            const int r = (int) i / perRow, c = (int) i % perRow;
            items[i]->setBounds (area.getX() + c * cellW, area.getY() + r * rowH, cellW, rowH);
        }
    }
}

//==============================================================================
OctoCidAudioProcessorEditor::Drawer::Drawer (OctoCidAudioProcessor& p)
{
    using namespace ParamIDs;
    auto knob = [&] (const juce::ParameterID& id, const juce::String& name, auto& into)
    {
        into.push_back (std::make_unique<octoui::Knob> (p.apvts, id, name));
        addAndMakeVisible (*into.back());
    };

    knob (slideTime, "Slide Time", slimeItems);
    knob (slidePunch, "Slide Punch", slimeItems);
    addCombo (p, sweepSpeed, "Sweep", slimeItems);
    knob (accentDecay, "Acc. Decay", slimeItems);
    knob (softAttack, "Soft Attack", slimeItems);
    knob (accentThreshold, "Seuil acc.", slimeItems);
    knob (analog, "Analog", slimeItems);
    knob (filterTrack, "F. Track", slimeItems);
    knob (filterFm, "Filter FM", slimeItems);
    knob (filterDrive, "F. Drive", slimeItems);
    addCombo (p, subOsc, "Sub Osc", slimeItems);
    knob (subLevel, "Sub Level", slimeItems);
    addCombo (p, muffler, "Muffler", slimeItems);

    knob (punch, "Punch", fxItems);
    knob (punchAttack, "Attaque", fxItems);
    knob (reverbMix, "Reverb", fxItems);
    knob (reverbDecay, "Rev. Decay", fxItems);
    knob (delayMix, "Delay", fxItems);
    addCombo (p, delayTime, "Temps", fxItems);
    knob (delayFb, "Feedback", fxItems);
    knob (reverbTone, "Rev. Tone", fxItems);
}

juce::Component* OctoCidAudioProcessorEditor::Drawer::addCombo (OctoCidAudioProcessor& p, const juce::ParameterID& id,
                                                                const juce::String& name,
                                                                std::vector<std::unique_ptr<juce::Component>>& into)
{
    auto c = std::make_unique<LabeledCombo> (name);
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter (id.getParamID())))
        c->box.addItemList (param->choices, 1);
    comboAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, id.getParamID(), c->box));
    addAndMakeVisible (*c);
    into.push_back (std::move (c));
    return into.back().get();
}

void OctoCidAudioProcessorEditor::Drawer::paint (juce::Graphics& g)
{
    g.fillAll (OctoColours::background);
    OctoDecor::paint (g, { 0.0f, 0.0f, 1000.0f, 620.0f }, getPosition().toFloat());
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const float split = r.getWidth() * 0.63f;
    const auto org = getPosition().toFloat();
    octoui::drawPanel (g, r.withWidth (split - 4.0f), "SLIME MODS", true, org);
    octoui::drawPanel (g, r.withTrimmedLeft (split + 4.0f), "PUNCH + FX", false, org);
}

void OctoCidAudioProcessorEditor::Drawer::resized()
{
    auto r = getLocalBounds().reduced (1);
    const int split = (int) ((float) r.getWidth() * 0.63f);
    auto left = r.withWidth (split - 4).reduced (8, 0).withTrimmedTop (20).withTrimmedBottom (6);
    auto right = r.withTrimmedLeft (split + 4).reduced (8, 0).withTrimmedTop (20).withTrimmedBottom (6);
    layoutGrid (left, slimeItems, 7);
    layoutGrid (right, fxItems, 4);
}

//==============================================================================
void OctoCidAudioProcessorEditor::Content::paint (juce::Graphics& g)
{
    using namespace OctoColours;
    auto& e = editor;
    g.fillAll (background);
    OctoDecor::paint (g, getLocalBounds().toFloat());

    // Barre du haut (le décor transparaît)
    { juce::Path tb; tb.addRectangle (e.topBar.toFloat()); OctoDecor::frostStrong (g, tb, {}); }
    g.setColour (panel.withAlpha (OctoDecor::get().bolts == 6 ? 0.6f : 0.82f));
    g.fillRect (e.topBar);
    g.setColour (ink);
    g.fillRect (e.topBar.withTop (e.topBar.getBottom() - 2));
    octoui::drawLogo (g, juce::Rectangle<float> (8.0f, 5.0f, 190.0f, 42.0f));

    // LED MIDI (verte sur une note, crème cerclée sur un accent)
    const bool on = e.displayedNote >= 0;
    auto led = e.ledArea.toFloat();
    g.setColour (ink);
    g.fillEllipse (led.expanded (2.0f));
    g.setColour (on ? (e.displayedAccent ? cream : slime) : dim.darker (0.3f));
    g.fillEllipse (led);

    // Panneaux
    octoui::drawPanel (g, e.drivePanel.toFloat(), "DRIVE");
    octoui::drawPanel (g, e.lfoPanel.toFloat(), "LFO");
    octoui::drawPanel (g, e.mainPanel.toFloat(), "303");
}

//==============================================================================
OctoCidAudioProcessorEditor::OctoCidAudioProcessorEditor (OctoCidAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), drawer (p), sequencer (p)
{
    using namespace ParamIDs;

    // Apparence (préférences globales ; par défaut : portail violet, glitch fort, potards néon, escargot punk)
    OctoLook::apply (OctoPrefs::getInt ("look", OctoLook::defaultLook));
    OctoLook::knobOverride = OctoPrefs::getInt ("knob", -1);
    OctoDecor::apply (OctoPrefs::getInt ("decor", OctoDecor::defaultDecor));
    OctoSnail::mascotStyle = OctoPrefs::getInt ("snail", OctoSnail::defaultMascotStyle);
    OctoColours::applyTheme (OctoDecor::get().theme);
    loadDecorImages();
    lnf.refreshColours();
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    tooltips = std::make_unique<juce::TooltipWindow> (&content, 700);

    auto& st = processor.apvts;
    auto add = [this] (juce::Component& c) { content.addAndMakeVisible (c); };

    // --- Barre du haut
    rebuildPresetBox();
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
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
        processor.applyProgram ((processor.getCurrentProgram() + delta + n) % n);
        processor.updateHostDisplay (juce::AudioProcessor::ChangeDetails().withProgramChanged (true));
        refreshPresetBox();
    };
    prevButton.onClick = [step] { step (-1); };
    nextButton.onClick = [step] { step (+1); };
    menuButton.onClick = [this] { showMenu(); };
    prevButton.setTooltip ("Preset precedent");
    nextButton.setTooltip ("Preset suivant");
    menuButton.setTooltip ("Presets, apparence, taille, patterns d'usine");
    seqToggle.setTooltip ("ON : le sequenceur joue avec le DAW. OFF : jeu au clavier / piano roll");
    snailToggle.setTooltip ("Animation de l'escargot On / Off");
    seqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, seqOn.getParamID(), seqToggle);
    snailAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, snailAnim.getParamID(), snailToggle);
    quality = std::make_unique<octoui::Segmented> (st, ParamIDs::quality, juce::StringArray { "Normal", "High" });
    quality->setTooltip ("Qualite : suréchantillonnage 2x (Normal) ou 4x (High)");
    for (auto* c : std::initializer_list<juce::Component*> { &presetBox, &prevButton, &nextButton, &menuButton,
                                                             &seqToggle, &snailToggle, quality.get() })
        add (*c);

    // --- Drive
    driveType = std::make_unique<octoui::Segmented> (st, ParamIDs::driveType, juce::StringArray { "Off", "Over", "Dist" });
    driveType->setTooltip ("Off / Overdrive chaud / Distortion facon pedale rat");
    drivePos = std::make_unique<octoui::Segmented> (st, ParamIDs::drivePos, juce::StringArray { "Apres", "Avant" });
    drivePos->setTooltip ("Apres le VCA (comme une pedale) / Avant le VCA (reste percutant)");
    drive     = std::make_unique<octoui::Knob> (st, driveAmt, "Drive");
    driveTone = std::make_unique<octoui::Knob> (st, ParamIDs::driveTone, "Tone");
    driveMix  = std::make_unique<octoui::Knob> (st, ParamIDs::driveMix, "Mix");
    for (auto* c : std::initializer_list<juce::Component*> { driveType.get(), drivePos.get(), drive.get(), driveTone.get(), driveMix.get() })
        add (*c);

    // --- LFO
    lfoSync.addItemList (LfoSync::names, 1);
    lfoSync.setTooltip ("Libre (Rate en Hz) ou cale sur le tempo");
    lfoSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (st, ParamIDs::lfoSync.getParamID(), lfoSync);
    lfoShape = std::make_unique<octoui::Segmented> (st, ParamIDs::lfoShape, juce::StringArray { "Sin", "Tri", "Sqr", "S&H" });
    lfoDest  = std::make_unique<octoui::Segmented> (st, ParamIDs::lfoDest, juce::StringArray { "Cutoff", "Reso", "Pitch" });
    lfoRate  = std::make_unique<octoui::Knob> (st, ParamIDs::lfoRate, "Rate");
    lfoDepth = std::make_unique<octoui::Knob> (st, ParamIDs::lfoDepth, "Depth");
    for (auto* c : std::initializer_list<juce::Component*> { &lfoSync, lfoShape.get(), lfoDest.get(), lfoRate.get(), lfoDepth.get() })
        add (*c);

    add (mascot);
    {
        MascotView::Sources src;
        auto P = [&st] (const juce::ParameterID& id) { return dynamic_cast<juce::RangedAudioParameter*> (st.getParameter (id.getParamID())); };
        src.cutoff = P (ParamIDs::cutoff); src.resonance = P (ParamIDs::resonance); src.envmod = P (ParamIDs::envMod);
        src.decay = P (ParamIDs::decay); src.drive = P (ParamIDs::driveAmt); src.driveType = P (ParamIDs::driveType);
        src.tune = P (ParamIDs::tune); src.lfoDepth = P (ParamIDs::lfoDepth); src.anim = P (ParamIDs::snailAnim);
        src.noteCount = &processor.noteCount; src.lastAccent = &processor.lastAccent; src.lastSlide = &processor.lastSlide;
        src.lastNote = &processor.lastNote; src.lfo = &processor.lfoView;
        mascot.setSources (src);
    }

    // --- Panneau 303
    tune      = std::make_unique<octoui::Knob> (st, ParamIDs::tune, "Tune", true);
    cutoff    = std::make_unique<octoui::Knob> (st, ParamIDs::cutoff, "Cutoff");
    resonance = std::make_unique<octoui::Knob> (st, ParamIDs::resonance, "Resonance");
    envMod    = std::make_unique<octoui::Knob> (st, ParamIDs::envMod, "Env Mod");
    decay     = std::make_unique<octoui::Knob> (st, ParamIDs::decay, "Decay");
    accent    = std::make_unique<octoui::Knob> (st, ParamIDs::accent, "Accent");
    volume    = std::make_unique<octoui::Knob> (st, ParamIDs::volume, "Volume");
    waveform  = std::make_unique<octoui::Segmented> (st, ParamIDs::waveform, juce::StringArray { "Saw", "Square" }, "Waveform");
    drawerButton.setClickingTogglesState (false);
    drawerButton.setComponentID ("drawerButton");
    drawerButton.onClick = [this] { toggleDrawer(); };
    drawerButton.setTooltip ("Ouvre / ferme le tiroir des reglages avances et des effets");
    for (auto* c : std::initializer_list<juce::Component*> { tune.get(), cutoff.get(), resonance.get(), envMod.get(), decay.get(),
                                                             accent.get(), waveform.get(), volume.get(), &drawerButton })
        add (*c);

    // --- Séquenceur, puis tiroir par-dessus la zone haute
    add (sequencer);
    content.addChildComponent (drawer);

    refreshPresetBox();
    content.sendLookAndFeelChange();      // toutes les zones de texte relisent la palette
    // Fenêtre redimensionnable (coin en bas à droite), proportions gardées
    const int w = processor.editorWidth > 0 ? processor.editorWidth : OctoPrefs::getInt ("width", kWidth);  // lu avant : resized() l'écrase
    setResizable (true, true);
    setResizeLimits (kWidth * 6 / 10, kHeight * 6 / 10, kWidth * 2, kHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) kWidth / (double) kHeight);
    const int wc = juce::jlimit (kWidth * 6 / 10, kWidth * 2, w);
    setSize (wc, juce::roundToInt ((double) wc * kHeight / kWidth));
    startTimerHz (30);
}

OctoCidAudioProcessorEditor::~OctoCidAudioProcessorEditor()
{
    OctoPrefs::setInt ("width", getWidth());   // taille par défaut des prochaines fenêtres
    tooltips.reset();
    setLookAndFeel (nullptr);
}

//==============================================================================
void OctoCidAudioProcessorEditor::resized()
{
    processor.editorWidth = getWidth();
    content.setBounds (0, 0, kWidth, kHeight);
    const float z = (float) getWidth() / (float) kWidth;
    content.setTransform (juce::AffineTransform::scale (z));

    topBar = { 0, 0, kWidth, 52 };
    ledArea = { 204, 21, 10, 10 };
    prevButton.setBounds (222, 13, 26, 26);
    presetBox.setBounds (252, 13, 208, 26);
    nextButton.setBounds (462, 13, 26, 26);
    seqToggle.setBounds (510, 13, 90, 26);
    snailToggle.setBounds (608, 13, 118, 26);
    quality->setBounds (736, 13, 130, 26);
    menuButton.setBounds (874, 13, 34, 26);

    // Rangée haute : Drive | mascotte | LFO
    upperArea = { 10, 60, 980, 146 };
    drivePanel = { 10, 60, 384, 146 };
    mascotArea = { 402, 60, 196, 146 };
    lfoPanel = { 606, 60, 384, 146 };

    driveType->setBounds (drivePanel.getX() + 12, drivePanel.getY() + 22, 170, 24);
    drivePos->setBounds (drivePanel.getRight() - 134, drivePanel.getY() + 22, 122, 24);
    const int dk = 96, dy = drivePanel.getY() + 52, dh = 88;
    drive->setBounds (drivePanel.getX() + 30, dy, dk, dh);
    driveTone->setBounds (drivePanel.getX() + 30 + 112, dy, dk, dh);
    driveMix->setBounds (drivePanel.getX() + 30 + 224, dy, dk, dh);

    // la mascotte déborde un peu (sauts, glissades) ; l'escargot reste dessiné dans sa zone
    mascot.setBounds (mascotArea.getX() - 10, topBar.getBottom(), mascotArea.getWidth() + 20, mascotArea.getBottom() + 4 - topBar.getBottom());
    mascot.setSnailArea (mascotArea.toFloat().translated ((float) -mascot.getX(), (float) -mascot.getY()));

    lfoSync.setBounds (lfoPanel.getX() + 12, lfoPanel.getY() + 22, 92, 24);
    lfoShape->setBounds (lfoPanel.getX() + 110, lfoPanel.getY() + 22, 150, 24);
    lfoDest->setBounds (lfoPanel.getRight() - 12 - 108, lfoPanel.getY() + 52, 108, 24);
    lfoRate->setBounds (lfoPanel.getX() + 40, dy, dk, dh);
    lfoDepth->setBounds (lfoPanel.getX() + 40 + 112, dy, dk, dh);

    // Panneau 303
    mainPanel = { 10, 214, 980, 124 };
    const int ky = mainPanel.getY() + 18, kh = 102, kw = 96;
    int kx = mainPanel.getX() + 8;
    for (auto* k : { tune.get(), cutoff.get(), resonance.get(), envMod.get(), decay.get(), accent.get() })
    {
        k->setBounds (kx, ky, kw, kh);
        kx += kw + 2;
    }
    waveform->setBounds (kx + 4, ky + 34, 116, 44);
    kx += 126;
    volume->setBounds (kx, ky, kw, kh);
    drawerButton.setBounds (mainPanel.getRight() - 124, mainPanel.getY() + 46, 112, 34);

    // Séquenceur
    seqArea = { 10, 346, 980, 266 };
    sequencer.setBounds (seqArea);

    // Tiroir : se déplie vers le haut depuis le panneau 303
    const int h = (int) std::round ((float) upperArea.getHeight() * drawerFrac);
    drawer.setBounds (upperArea.getX(), upperArea.getBottom() - h, upperArea.getWidth(), upperArea.getHeight());
    drawer.setVisible (drawerFrac > 0.001f);
    if (drawerFrac > 0.001f)
        drawer.setBounds (upperArea.withTop (upperArea.getBottom() - h));
}

//==============================================================================
void OctoCidAudioProcessorEditor::toggleDrawer()
{
    drawerTarget = drawerTarget > 0.5f ? 0.0f : 1.0f;
    drawerButton.setButtonText (drawerTarget > 0.5f ? "FERMER" : "SLIME MODS");
}

void OctoCidAudioProcessorEditor::timerCallback()
{
    // Animation du tiroir (douce, ~200 ms)
    if (std::abs (drawerFrac - drawerTarget) > 0.001f)
    {
        drawerFrac += (drawerTarget - drawerFrac) * 0.35f;
        if (std::abs (drawerFrac - drawerTarget) < 0.01f)
            drawerFrac = drawerTarget;
        const int h = (int) std::round ((float) upperArea.getHeight() * drawerFrac);
        drawer.setVisible (h > 0);
        drawer.setBounds (upperArea.withTop (upperArea.getBottom() - juce::jmax (1, h)));
    }

    const int note = processor.lastNote.load (std::memory_order_relaxed);
    const bool acc = processor.lastAccent.load (std::memory_order_relaxed);
    if (note != displayedNote || acc != displayedAccent)
    {
        displayedNote = note;
        displayedAccent = acc;
        content.repaint (ledArea.expanded (4));
    }

    if (processor.getCurrentProgram() != displayedProgram)
        refreshPresetBox();
}

void OctoCidAudioProcessorEditor::rebuildPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    const int n = processor.getNumPrograms();
    for (int i = 0; i < juce::jmin (n, FactoryPresets::count); ++i)
        presetBox.addItem (juce::String (i + 1).paddedLeft ('0', 2) + "  " + processor.getProgramName (i), i + 1);
    if (n > FactoryPresets::count)
    {
        presetBox.addSectionHeading ("MES PRESETS");
        for (int i = FactoryPresets::count; i < n; ++i)
            presetBox.addItem (processor.getProgramName (i), i + 1);
    }
    displayedProgram = -2;
}

void OctoCidAudioProcessorEditor::refreshPresetBox()
{
    const int idx = processor.getCurrentProgram();
    displayedProgram = idx;
    presetBox.setSelectedId (idx + 1, juce::dontSendNotification);
    presetBox.setTooltip (processor.getProgramDescription (idx));
}

//==============================================================================
void OctoCidAudioProcessorEditor::showMenu()
{
    juce::PopupMenu zoom, menu, look, knobs, decor, snail, banks;
    const int zooms[] = { 75, 100, 125, 150, 175 };
    for (int i = 0; i < 5; ++i)
        zoom.addItem (100 + i, juce::String (zooms[i]) + " %", true, std::abs (getWidth() - kWidth * zooms[i] / 100) < 4);
    zoom.addItem (110, juce::String::fromUTF8 ("(ou tirer le coin en bas à droite)"), false);

    for (int i = 0; i < OctoLook::numLooks; ++i)
        look.addItem (300 + i, juce::String::fromUTF8 (OctoLook::looks[i].name), true, i == OctoLook::current);
    knobs.addItem (600, juce::String::fromUTF8 ("Celui du style d'interface"), true, OctoLook::knobOverride < 0);
    knobs.addSeparator();
    for (int i = 0; i < KnobStyles::chickenCount; ++i)
        knobs.addItem (601 + i, juce::String::fromUTF8 (KnobStyles::chickenNames[i]), true, i == OctoLook::knobOverride);
    for (int i = 0; i < OctoDecor::numDecors; ++i)
        decor.addItem (700 + i, juce::String::fromUTF8 (OctoDecor::decors[i].name), true, i == OctoDecor::current);
    for (int i = 0; i < OctoSnail::numStyles; ++i)
        snail.addItem (800 + i, juce::String::fromUTF8 (OctoSnail::styles[i].name), true, i == OctoSnail::mascotStyle);
    juce::PopupMenu appearance;
    appearance.addSubMenu ("Fond", decor);
    appearance.addSubMenu ("Interface", look);
    appearance.addSubMenu ("Potards", knobs);
    appearance.addSubMenu ("Escargot", snail);
    appearance.addSeparator();
    appearance.addItem (899, juce::String::fromUTF8 ("Revenir à l'apparence d'origine"));

    for (int i = 0; i < octocid::FactoryBanks::count; ++i)
        banks.addItem (400 + i, octocid::FactoryBanks::names[i]);
    banks.addSeparator();
    banks.addItem (499, juce::String::fromUTF8 ("Revenir à mes patterns d'avant"), sequencer.canRestoreBank());

    const int cur = processor.getCurrentProgram();
    menu.addSectionHeader ("OctoCid v" JucePlugin_VersionString);
    menu.addItem (501, juce::String::fromUTF8 ("Enregistrer le preset…"));
    menu.addItem (502, juce::String::fromUTF8 ("Supprimer « ") + processor.getProgramName (cur) + juce::String::fromUTF8 (" »"),
                  processor.isUserPreset (cur));
    menu.addItem (503, "Ouvrir le dossier des presets");
    menu.addSeparator();
    menu.addSubMenu ("Apparence", appearance);
    menu.addSubMenu ("Taille", zoom);
    menu.addSubMenu (juce::String::fromUTF8 ("Patterns d'usine (remplace A à H)"), banks);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (menuButton), [this, zooms] (int r)
    {
        if (r >= 100 && r < 105) setSize (kWidth * zooms[r - 100] / 100, kHeight * zooms[r - 100] / 100);
        else if (r >= 300 && r < 300 + OctoLook::numLooks) { OctoPrefs::setInt ("look", r - 300); applyAppearance(); }
        else if (r >= 600 && r <= 600 + KnobStyles::chickenCount) { OctoPrefs::setInt ("knob", r - 601); applyAppearance(); }
        else if (r >= 700 && r < 700 + OctoDecor::numDecors) { OctoPrefs::setInt ("decor", r - 700); applyAppearance(); }
        else if (r >= 800 && r < 800 + OctoSnail::numStyles) { OctoPrefs::setInt ("snail", r - 800); applyAppearance(); }
        else if (r == 899)
        {
            OctoPrefs::setInt ("look", OctoLook::defaultLook); OctoPrefs::setInt ("knob", -1);
            OctoPrefs::setInt ("decor", OctoDecor::defaultDecor); OctoPrefs::setInt ("snail", OctoSnail::defaultMascotStyle);
            applyAppearance();
        }
        else if (r >= 400 && r < 400 + octocid::FactoryBanks::count) sequencer.loadFactoryBank (r - 400);
        else if (r == 499) sequencer.restorePreviousBank();
        else if (r == 501) savePresetDialog();
        else if (r == 502)
        {
            processor.deleteUserPreset (processor.getCurrentProgram());
            rebuildPresetBox(); refreshPresetBox();
        }
        else if (r == 503)
        {
            OctoCidAudioProcessor::userPresetFolder().createDirectory();
            OctoCidAudioProcessor::userPresetFolder().startAsProcess();
        }
    });
}

void OctoCidAudioProcessorEditor::savePresetDialog()
{
    auto* w = new juce::AlertWindow ("Enregistrer le preset", "Nom du preset :", juce::MessageBoxIconType::NoIcon, this);
    const int cur = processor.getCurrentProgram();
    w->addTextEditor ("name", processor.isUserPreset (cur) ? processor.getProgramName (cur) : juce::String());
    w->addButton ("Enregistrer", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Annuler", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<OctoCidAudioProcessorEditor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int r)
    {
        if (r != 1 || safe == nullptr) return;
        const auto name = w->getTextEditorContents ("name");
        if (safe->processor.saveUserPreset (name))
        {
            safe->rebuildPresetBox();
            safe->refreshPresetBox();
            safe->processor.updateHostDisplay (juce::AudioProcessor::ChangeDetails().withProgramChanged (true));
        }
    }), true);
}

void OctoCidAudioProcessorEditor::applyAppearance()
{
    OctoLook::apply (OctoPrefs::getInt ("look", OctoLook::defaultLook));
    OctoLook::knobOverride = OctoPrefs::getInt ("knob", -1);
    OctoDecor::apply (OctoPrefs::getInt ("decor", OctoDecor::defaultDecor));
    OctoSnail::mascotStyle = OctoPrefs::getInt ("snail", OctoSnail::defaultMascotStyle);
    loadDecorImages();
    setTheme (OctoDecor::get().theme);
    mascot.styleChanged();
    drawer.repaint();
    sequencer.repaint();
}

void OctoCidAudioProcessorEditor::setTheme (int index)
{
    OctoColours::applyTheme (index);
    lnf.refreshColours();
    sendLookAndFeelChange();
    content.sendLookAndFeelChange();
    repaint();
    content.repaint();
}

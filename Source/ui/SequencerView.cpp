#include "SequencerView.h"
#include "Palette.h"
#include "Widgets.h"

octocid::Pattern SequencerView::clipboard {};
bool SequencerView::hasClipboard = false;

namespace
{
    const char* const kNoteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    //==========================================================================
    // Bulle de réglages du générateur
    //==========================================================================
    class GeneratorSettings : public juce::Component
    {
    public:
        explicit GeneratorSettings (SequencerView& v) : view (v)
        {
            for (int i = 0; i < 12; ++i) root.addItem (kNoteNames[i], i + 1);
            root.setSelectedItemIndex (view.genRoot, juce::dontSendNotification);
            root.onChange = [this] { view.genRoot = root.getSelectedItemIndex(); };

            for (int i = 0; i < octocid::kNumScales; ++i) scale.addItem (juce::String::fromUTF8 (octocid::kScales[i].name), i + 1);
            scale.setSelectedItemIndex (view.genScale, juce::dontSendNotification);
            scale.onChange = [this] { view.genScale = scale.getSelectedItemIndex(); };

            density.setRange (-7, 7, 1);
            density.setValue (view.genDensity, juce::dontSendNotification);
            density.getProperties().set ("bipolar", true);
            density.onValueChange = [this] { view.genDensity = (int) density.getValue(); };
            density.setTooltip ("-7 : tonique martelee, 0 : ligne aeree, +7 : toute la gamme, ligne pleine");

            accents.setRange (0.0, 1.0, 0.01);
            accents.setValue (view.genAccents, juce::dontSendNotification);
            accents.onValueChange = [this] { view.genAccents = (float) accents.getValue(); };
            slides.setRange (0.0, 1.0, 0.01);
            slides.setValue (view.genSlides, juce::dontSendNotification);
            slides.onValueChange = [this] { view.genSlides = (float) slides.getValue(); };

            for (auto* s : { &density, &accents, &slides })
                s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 40, 20);

            for (auto* c : std::initializer_list<juce::Component*> { &root, &scale, &density, &accents, &slides })
                addAndMakeVisible (c);
            setSize (300, 170);
        }

        void paint (juce::Graphics& g) override
        {
            using namespace OctoColours;
            g.setColour (cream);
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            const char* names[] = { "Tonique", "Gamme", "Densite", "Accents", "Slides" };
            for (int i = 0; i < 5; ++i)
                g.drawText (names[i], 8, 8 + i * 31, 70, 24, juce::Justification::centredLeft);
        }

        void resized() override
        {
            root.setBounds (80, 8, 70, 24);
            scale.setBounds (80, 39, 210, 24);
            density.setBounds (80, 70, 210, 24);
            accents.setBounds (80, 101, 210, 24);
            slides.setBounds (80, 132, 210, 24);
        }

    private:
        SequencerView& view;
        juce::ComboBox root, scale;
        juce::Slider density { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight },
                     accents { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight },
                     slides  { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    };
}

//==============================================================================
SequencerView::SequencerView (OctoCidAudioProcessor& p) : processor (p)
{
    getProperties().set ("plainStyle", true);   // le séquenceur garde le style simple
    setWantsKeyboardFocus (true);

    for (int i = 0; i < 8; ++i)
    {
        auto& b = patternButtons[i];
        b.setButtonText (juce::String::charToString ((juce::juce_wchar) ('A' + i)));
        b.onClick = [this, i] { selectPattern (i); };
        b.setTooltip ("Pattern " + b.getButtonText() + " (vert : joue, crème : en édition)");
        addAndMakeVisible (b);
    }

    auto setupLabel = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        l.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (l);
    };
    setupLabel (lengthLabel, "Pas");
    setupLabel (swingLabel, "Swing");
    setupLabel (gateLabel, "Gate");
    transposeLabel.setFont (juce::FontOptions (11.5f));
    transposeLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (transposeLabel);

    lengthSlider.setRange (1, octocid::Pattern::kMaxSteps, 1);
    lengthSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 28, 22);
    lengthSlider.setTooltip ("Longueur du pattern (1 a 32 pas)");
    lengthSlider.onValueChange = [this]
    {
        const int len = (int) lengthSlider.getValue();
        if (len != pattern.length)
            edit ([len] (octocid::Pattern& pt) { pt.length = len; });
    };
    addAndMakeVisible (lengthSlider);

    resBox.addItemList (SeqRes::names, 1);
    resBox.setTooltip ("Resolution des pas");
    resAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, ParamIDs::seqRes.getParamID(), resBox);
    addAndMakeVisible (resBox);

    for (auto* s : { &swingSlider, &gateSlider })
    {
        s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 42, 20);
        addAndMakeVisible (*s);
    }
    swingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamIDs::seqSwing.getParamID(), swingSlider);
    gateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamIDs::seqGate.getParamID(), gateSlider);
    gateSlider.setTooltip ("Longueur des notes (50 % = l'original ; plus court = plus percutant)");

    auto seed = [] { return (uint32_t) juce::Random::getSystemRandom().nextInt(); };
    generateButton.onClick = [this, seed]
    {
        const int root = genRoot, scale = genScale, d = genDensity;
        const float a = genAccents, s = genSlides;
        const uint32_t sd = seed();
        edit ([=] (octocid::Pattern& pt) { octocid::PatternTools::generate (pt, root, scale, d, a, s, sd); });
    };
    genSettingsButton.onClick = [this] { showGeneratorSettings(); };
    mutateButton.onClick = [this, seed]
    {
        const int root = genRoot, scale = genScale;
        const uint32_t sd = seed();
        edit ([=] (octocid::Pattern& pt) { octocid::PatternTools::mutate (pt, root, scale, 0.45f, sd); });
    };
    quantizeButton.onClick = [this]
    {
        const int root = genRoot, scale = genScale;
        edit ([=] (octocid::Pattern& pt) { octocid::PatternTools::quantize (pt, root, scale); });
    };
    shiftLeft.onClick  = [this] { edit ([] (octocid::Pattern& pt) { octocid::PatternTools::shift (pt, -1); }); };
    shiftRight.onClick = [this] { edit ([] (octocid::Pattern& pt) { octocid::PatternTools::shift (pt, +1); }); };
    transDown.onClick  = [this] { edit ([] (octocid::Pattern& pt) { octocid::PatternTools::transpose (pt, -1); }); };
    transUp.onClick    = [this] { edit ([] (octocid::Pattern& pt) { octocid::PatternTools::transpose (pt, +1); }); };
    copyButton.onClick = [this] { clipboard = pattern; hasClipboard = true; };
    pasteButton.onClick = [this] { if (hasClipboard) edit ([] (octocid::Pattern& pt) { pt = clipboard; }); };
    clearButton.onClick = [this] { edit ([] (octocid::Pattern& pt) { octocid::PatternTools::clear (pt); }); };
    undoButton.onClick = [this] { undo(); };
    redoButton.onClick = [this] { redo(); };

    recButton.setClickingTogglesState (true);
    recButton.onClick = [this]
    {
        processor.stepRecord.store (recButton.getToggleState());
        grabKeyboardFocus();
    };
    recButton.setToggleState (processor.stepRecord.load(), juce::dontSendNotification);

    generateButton.setTooltip ("Nouvelle ligne acid dans la gamme (reglages : bouton ...)");
    genSettingsButton.setTooltip ("Reglages du generateur : tonique, gamme, densite, accents, slides");
    mutateButton.setTooltip ("Fait evoluer la ligne : decale accents ou slides, inverse, change quelques notes");
    quantizeButton.setTooltip ("Ramene toutes les notes dans la gamme du generateur");
    shiftLeft.setTooltip ("Decaler tout le pattern d'un pas vers la gauche");
    shiftRight.setTooltip ("Decaler tout le pattern d'un pas vers la droite");
    transDown.setTooltip ("Transposer le pattern d'un demi-ton vers le bas");
    transUp.setTooltip ("Transposer le pattern d'un demi-ton vers le haut");
    undoButton.setTooltip ("Annuler (Ctrl+Z)");
    redoButton.setTooltip ("Retablir (Ctrl+Y)");
    recButton.setTooltip ("Enregistrement pas a pas : chaque note jouee au clavier MIDI s'ecrit au curseur. "
                          "Velocite haute = accent, notes liees = slide.");

    for (auto* b : { &generateButton, &genSettingsButton, &mutateButton, &quantizeButton, &shiftLeft, &shiftRight,
                     &transDown, &transUp, &copyButton, &pasteButton, &clearButton, &undoButton, &redoButton, &recButton })
        addAndMakeVisible (b);

    reloadFromProcessor();
    timerCallback();
    startTimerHz (30);
}

SequencerView::~SequencerView()
{
    processor.stepRecord.store (false);
}

void SequencerView::showGeneratorSettings()
{
    juce::CallOutBox::launchAsynchronously (std::make_unique<GeneratorSettings> (*this),
                                            genSettingsButton.getScreenBounds(), nullptr);
}

//==============================================================================
int SequencerView::editIndex() const
{
    return juce::jlimit (0, 7, (int) processor.apvts.getRawParameterValue (ParamIDs::seqPattern.getParamID())->load());
}

void SequencerView::selectPattern (int index)
{
    if (auto* param = processor.apvts.getParameter (ParamIDs::seqPattern.getParamID()))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) index));
        param->endChangeGesture();
    }
    reloadFromProcessor();
}

void SequencerView::reloadFromProcessor()
{
    if (editIndex() != shownIndex)
    {
        undoStack.clear();
        redoStack.clear();
    }
    shownIndex = editIndex();
    pattern = processor.getPattern (shownIndex);
    lengthSlider.setValue (pattern.length, juce::dontSendNotification);
    cursor = juce::jlimit (0, pattern.length - 1, cursor);
    repaint();
}

void SequencerView::commit()
{
    processor.setPattern (shownIndex, pattern);
}

void SequencerView::pushUndo()
{
    undoStack.push_back (pattern);
    if (undoStack.size() > kMaxUndo)
        undoStack.erase (undoStack.begin());
    redoStack.clear();
}

void SequencerView::edit (const std::function<void (octocid::Pattern&)>& change)
{
    pushUndo();
    change (pattern);
    lengthSlider.setValue (pattern.length, juce::dontSendNotification);
    cursor = juce::jlimit (0, pattern.length - 1, cursor);
    commit();
    repaint();
}

void SequencerView::undo()
{
    if (undoStack.empty()) return;
    redoStack.push_back (pattern);
    pattern = undoStack.back();
    undoStack.pop_back();
    lengthSlider.setValue (pattern.length, juce::dontSendNotification);
    commit();
    repaint();
}

void SequencerView::redo()
{
    if (redoStack.empty()) return;
    undoStack.push_back (pattern);
    pattern = redoStack.back();
    redoStack.pop_back();
    lengthSlider.setValue (pattern.length, juce::dontSendNotification);
    commit();
    repaint();
}

void SequencerView::moveCursor (int delta)
{
    const int n = std::max (1, pattern.length);
    cursor = ((cursor + delta) % n + n) % n;
    repaint (gridArea);
}

void SequencerView::handleRecordedNotes()
{
    OctoCidAudioProcessor::RecNote rn;
    bool any = false;
    const int threshold = (int) processor.apvts.getRawParameterValue (ParamIDs::accentThreshold.getParamID())->load();

    while (processor.popRecordedNote (rn))
    {
        if (! any) pushUndo();
        any = true;

        if (rn.legato)
        {
            const int prev = (cursor - 1 + pattern.length) % pattern.length;
            pattern.steps[prev].slide = true;
        }

        auto& st = pattern.steps[cursor];
        st.note = (uint8_t) (rn.note % 12);
        st.octave = (int8_t) juce::jlimit (-1, 1, (rn.note - 36) / 12 - (rn.note < 36 ? 1 : 0));
        st.accent = rn.velocity >= threshold;
        st.slide = false;
        st.gate = octocid::Gate::Note;
        cursor = (cursor + 1) % std::max (1, pattern.length);
    }

    if (any)
    {
        commit();
        repaint();
    }
}

void SequencerView::timerCallback()
{
    using namespace OctoColours;
    handleRecordedNotes();

    if (editIndex() != shownIndex)
        reloadFromProcessor();
    else if (! (processor.getPattern (shownIndex) == pattern))
        reloadFromProcessor();

    const int step = processor.seqStep.load (std::memory_order_relaxed);
    playingPattern = processor.seqActivePattern.load (std::memory_order_relaxed);
    queuedPattern = processor.seqQueuedPattern.load (std::memory_order_relaxed);
    const int tr = processor.seqTranspose.load (std::memory_order_relaxed);

    for (int i = 0; i < 8; ++i)
    {
        auto& b = patternButtons[i];
        const bool isPlaying = i == playingPattern;
        const bool isQueued = i == queuedPattern && queuedPattern != playingPattern;
        const bool isEdited = i == shownIndex;
        // Vert : joue ; vert pâle : attend son tour ; crème : en édition
        const auto fill = isPlaying ? slime : (isQueued ? slime.withAlpha (0.35f) : (isEdited ? cream.withAlpha (0.85f) : panelHi));
        const auto text = isPlaying ? onSlime() : (isEdited ? background : cream);
        b.setToggleState (false, juce::dontSendNotification);
        b.setColour (juce::TextButton::buttonColourId, fill);
        b.setColour (juce::TextButton::buttonOnColourId, fill);
        b.setColour (juce::TextButton::textColourOffId, text);
        b.setColour (juce::TextButton::textColourOnId, text);
        if (isPlaying && isEdited)
            b.setColour (juce::TextButton::textColourOffId, onSlime());
    }

    if (tr != transpose || transposeLabel.getText().isEmpty())
    {
        transpose = tr;
        transposeLabel.setText ("Transpo " + juce::String (tr > 0 ? "+" : "") + juce::String (tr), juce::dontSendNotification);
    }
    transposeLabel.setColour (juce::Label::textColourId, dim.brighter (0.5f));

    if (recButton.getToggleState() != processor.stepRecord.load())
        recButton.setToggleState (processor.stepRecord.load(), juce::dontSendNotification);

    if (step != playingStep)
    {
        playingStep = step;
        repaint (gridArea);
    }
}

//==============================================================================
void SequencerView::resized()
{
    const int W = getWidth(), H = getHeight();

    // Rangée du haut
    int x = 12;
    for (int i = 0; i < 8; ++i)
    {
        patternButtons[i].setBounds (x, 6, 28, 24);
        x += 31;
    }
    lengthLabel.setBounds (x + 4, 6, 28, 24);
    lengthSlider.setBounds (x + 34, 6, 96, 24);
    resBox.setBounds (x + 136, 6, 70, 24);
    swingLabel.setBounds (x + 210, 6, 42, 24);
    swingSlider.setBounds (x + 254, 6, 140, 24);
    gateLabel.setBounds (x + 398, 6, 34, 24);
    gateSlider.setBounds (x + 434, 6, 140, 24);
    transposeLabel.setBounds (W - 100, 6, 88, 24);

    // Outils (en bas)
    const int toolsY = H - 32, th = 26;
    int bx = 12;
    auto place = [&] (juce::Component& c, int w, int gap = 4) { c.setBounds (bx, toolsY, w, th); bx += w + gap; };
    place (generateButton, 70, 2);
    place (genSettingsButton, 26);
    place (mutateButton, 58);
    place (quantizeButton, 58, 12);
    place (shiftLeft, 32); place (shiftRight, 32); place (transDown, 32); place (transUp, 32, 12);
    place (copyButton, 58); place (pasteButton, 58); place (clearButton, 60, 12);
    place (undoButton, 62); place (redoButton, 62, 12);
    place (recButton, 48, 10);
    midiDragArea = { bx, toolsY, W - 12 - bx, th };

    // Grille : hauteur répartie entre les notes et les 4 lignes
    const int gridTop = 50, gridBottom = toolsY - 8;
    const int laneH = 15, laneGap = 3;
    const int lanesH = 4 * laneH + 4 * laneGap;
    noteRowH = juce::jmax (8, (gridBottom - gridTop - lanesH - 4) / 12);
    const int x0 = 12, w = W - 24;
    noteArea  = { x0, gridTop, w, 12 * noteRowH };
    octaveRow = { x0, noteArea.getBottom() + 5, w, laneH };
    accentRow = { x0, octaveRow.getBottom() + laneGap, w, laneH };
    slideRow  = { x0, accentRow.getBottom() + laneGap, w, laneH };
    gateRow   = { x0, slideRow.getBottom() + laneGap, w, laneH };
    gridArea  = noteArea.getUnion (gateRow).withTop (noteArea.getY() - 14).expanded (2);
}

juce::Rectangle<int> SequencerView::stepColumn (int step) const
{
    const int x0 = noteArea.getX() + kLabelW;
    const float w = (float) (noteArea.getRight() - x0) / (float) visibleSteps();
    const int xa = x0 + (int) std::round (step * w);
    const int xb = x0 + (int) std::round ((step + 1) * w);
    return { xa, noteArea.getY(), xb - xa, gateRow.getBottom() - noteArea.getY() };
}

int SequencerView::stepAt (int x) const
{
    const int x0 = noteArea.getX() + kLabelW;
    if (x < x0 || x >= noteArea.getRight()) return -1;
    const float w = (float) (noteArea.getRight() - x0) / (float) visibleSteps();
    return juce::jlimit (0, visibleSteps() - 1, (int) ((float) (x - x0) / w));
}

SequencerView::Row SequencerView::rowAt (int y, int& noteOut) const
{
    noteOut = -1;
    if (y >= noteArea.getY() && y < noteArea.getBottom())
    {
        const int r = (y - noteArea.getY()) / noteRowH;
        noteOut = 11 - juce::jlimit (0, 11, r);
        return Row::Note;
    }
    if (y >= octaveRow.getY() && y < octaveRow.getBottom()) return Row::Octave;
    if (y >= accentRow.getY() && y < accentRow.getBottom()) return Row::Accent;
    if (y >= slideRow.getY() && y < slideRow.getBottom())   return Row::Slide;
    if (y >= gateRow.getY() && y < gateRow.getBottom())     return Row::Gate;
    return Row::None;
}

juce::Rectangle<int> SequencerView::laneButton (Row row, int dir) const
{
    juce::Rectangle<int> r;
    switch (row)
    {
        case Row::Note:   r = { noteArea.getX(), noteArea.getY() - 14, kLabelW, 13 }; break;
        case Row::Accent: r = accentRow.withWidth (kLabelW); break;
        case Row::Slide:  r = slideRow.withWidth (kLabelW); break;
        case Row::Gate:   r = gateRow.withWidth (kLabelW); break;
        case Row::Octave:
        case Row::None:
        default:          return {};
    }
    const int bw = 14;
    const int x = r.getRight() - 6 - (dir < 0 ? 2 * bw + 2 : bw);
    return { x, r.getCentreY() - 6, bw, 13 };
}

//==============================================================================
void SequencerView::editCell (juce::Point<int> pos, bool isDrag, bool rightClick)
{
    const int step = stepAt (pos.x);
    if (step < 0 || step >= pattern.length)
        return;
    if (isDrag && step == lastDragStep)
        return;
    lastDragStep = step;

    int note = -1;
    const Row row = isDrag ? dragRow : rowAt (pos.y, note);
    if (isDrag && row == Row::Note)
        rowAt (juce::jlimit (noteArea.getY(), noteArea.getBottom() - 1, pos.y), note);
    if (row == Row::None)
        return;

    if (! isDrag)
    {
        pushUndo();
        cursor = step;
    }

    auto& st = pattern.steps[step];
    using octocid::Gate;

    switch (row)
    {
        case Row::Note:
            if (rightClick)
                st.gate = Gate::Rest;
            else if (! isDrag && st.gate == Gate::Note && st.note == note)
                st.gate = Gate::Rest;
            else
            {
                st.note = (uint8_t) note;
                st.gate = Gate::Note;
            }
            break;
        case Row::Octave:
            if (! isDrag)
                dragOctave = st.octave >= 1 ? -1 : st.octave + 1;
            st.octave = (int8_t) dragOctave;
            break;
        case Row::Accent:
            if (! isDrag) dragValue = ! st.accent;
            st.accent = dragValue;
            break;
        case Row::Slide:
            if (! isDrag) dragValue = ! st.slide;
            st.slide = dragValue;
            break;
        case Row::Gate:
            if (! isDrag)
                st.gate = st.gate == Gate::Note ? Gate::Rest : (st.gate == Gate::Rest ? Gate::Tie : Gate::Note);
            break;
        case Row::None:
        default:
            return;
    }

    if (! isDrag)
        dragRow = row;

    commit();
    repaint (gridArea);
}

void SequencerView::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    midiDragStarted = false;
    if (midiDragArea.contains (e.getPosition()))
        return;

    using Lane = octocid::PatternTools::Lane;
    const std::pair<Row, Lane> lanes[] = { { Row::Note, Lane::Notes }, { Row::Accent, Lane::Accents },
                                           { Row::Slide, Lane::Slides }, { Row::Gate, Lane::Gates } };
    for (const auto& [row, lane] : lanes)
        for (int dir : { -1, 1 })
            if (laneButton (row, dir).expanded (2).contains (e.getPosition()))
            {
                const Lane l = lane;
                edit ([l, dir] (octocid::Pattern& pt) { octocid::PatternTools::shiftLane (pt, l, dir); });
                return;
            }

    lastDragStep = -1;
    editCell (e.getPosition(), false, e.mods.isPopupMenu());
}

void SequencerView::mouseDrag (const juce::MouseEvent& e)
{
    if (midiDragArea.contains (e.getMouseDownPosition()))
    {
        if (! midiDragStarted && e.getDistanceFromDragStart() > 6)
        {
            midiDragStarted = true;
            const auto name = "OctoCid_" + juce::String::charToString ((juce::juce_wchar) ('A' + shownIndex));
            const auto file = octocid::PatternIO::writeMidiFile (pattern, processor.getStepBeats(), 36 + transpose, name,
                                                                 processor.getGate());
            if (file.existsAsFile())
                juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
        }
        return;
    }

    if (dragRow == Row::Note || dragRow == Row::Accent || dragRow == Row::Slide || dragRow == Row::Octave)
        editCell (e.getPosition(), true, e.mods.isPopupMenu());
}

void SequencerView::mouseUp (const juce::MouseEvent&)
{
    dragRow = Row::None;
    lastDragStep = -1;
    midiDragStarted = false;
}

//==============================================================================
bool SequencerView::keyPressed (const juce::KeyPress& key)
{
    using octocid::Gate;
    const auto mods = key.getModifiers();
    const int code = key.getKeyCode();

    if (mods.isCommandDown())
    {
        if (code == 'Z' && mods.isShiftDown()) { redo(); return true; }
        if (code == 'Z') { undo(); return true; }
        if (code == 'Y') { redo(); return true; }
        if (code == 'C') { clipboard = pattern; hasClipboard = true; return true; }
        if (code == 'V') { if (hasClipboard) edit ([] (octocid::Pattern& pt) { pt = clipboard; }); return true; }
        return false;
    }

    auto change = [this] (const std::function<void (octocid::Step&)>& f)
    {
        const int c = cursor;
        edit ([&f, c] (octocid::Pattern& pt) { f (pt.steps[c]); });
    };

    if (code == juce::KeyPress::leftKey)  { moveCursor (-1); return true; }
    if (code == juce::KeyPress::rightKey) { moveCursor (+1); return true; }
    if (code == juce::KeyPress::upKey || code == juce::KeyPress::downKey)
    {
        const int dir = code == juce::KeyPress::upKey ? 1 : -1;
        if (mods.isShiftDown())
            change ([dir] (octocid::Step& s) { s.octave = (int8_t) juce::jlimit (-1, 1, s.octave + dir); });
        else
            change ([dir] (octocid::Step& s)
            {
                int abs = s.octave * 12 + s.note + dir;
                abs = juce::jlimit (-12, 23, abs);
                s.octave = (int8_t) ((abs + 12) / 12 - 1);
                s.note = (uint8_t) ((abs + 12) % 12);
                s.gate = Gate::Note;
            });
        return true;
    }
    if (code == juce::KeyPress::deleteKey || code == juce::KeyPress::backspaceKey)
    {
        change ([] (octocid::Step& s) { s.gate = Gate::Rest; });
        return true;
    }

    switch (juce::CharacterFunctions::toUpperCase (key.getTextCharacter()))
    {
        case 'A': change ([] (octocid::Step& s) { s.accent = ! s.accent; }); return true;
        case 'S': change ([] (octocid::Step& s) { s.slide = ! s.slide; }); return true;
        case 'G': change ([] (octocid::Step& s)
                  { s.gate = s.gate == Gate::Note ? Gate::Rest : (s.gate == Gate::Rest ? Gate::Tie : Gate::Note); });
                  return true;
        case 'N': change ([] (octocid::Step& s) { s.gate = Gate::Note; }); return true;
        case 'R': change ([] (octocid::Step& s) { s.gate = Gate::Rest; }); return true;
        case 'T': change ([] (octocid::Step& s) { s.gate = Gate::Tie; }); return true;
        default: break;
    }
    return false;
}

//==============================================================================
void SequencerView::paint (juce::Graphics& g)
{
    using namespace OctoColours;
    using octocid::Gate;

    octoui::drawPanel (g, getLocalBounds().toFloat().reduced (1.0f), {}, false, getPosition().toFloat(), OctoLook::get().seqAlpha, true);

    const int nVis = visibleSteps();
    const bool showPlayhead = playingStep >= 0 && playingPattern == shownIndex;

    // Fond de la grille
    g.setColour (background.withAlpha (0.55f));
    g.fillRoundedRectangle (gridArea.toFloat().expanded (4.0f), juce::jmin (8.0f, OctoLook::get().ctrlR));

    // Libellés des notes
    g.setFont (juce::FontOptions ((float) juce::jmin (11, noteRowH), juce::Font::bold));
    for (int r = 0; r < 12; ++r)
    {
        const int note = 11 - r;
        const bool black = juce::String (kNoteNames[note]).containsChar ('#');
        auto rowRect = juce::Rectangle<int> (noteArea.getX(), noteArea.getY() + r * noteRowH, noteArea.getWidth(), noteRowH);
        g.setColour (black ? ink.withAlpha (0.35f) : cream.withAlpha (0.03f));
        g.fillRect (rowRect.withTrimmedLeft (kLabelW));
        g.setColour (black ? dim : cream.withAlpha (0.75f));
        g.drawText (kNoteNames[note], rowRect.withWidth (kLabelW - 8), juce::Justification::centredRight);
    }
    auto drawRowLabel = [&] (juce::Rectangle<int> r, const char* text)
    {
        g.setColour (cream.withAlpha (0.8f));
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (text, r.withWidth (kLabelW - 38).withTrimmedLeft (4), juce::Justification::centredLeft);
    };
    drawRowLabel ({ noteArea.getX(), noteArea.getY() - 14, kLabelW, 13 }, "Notes");
    drawRowLabel (octaveRow, "Octave");
    drawRowLabel (accentRow, "Accent");
    drawRowLabel (slideRow, "Slide");
    drawRowLabel (gateRow, "Gate");

    for (auto row : { Row::Note, Row::Accent, Row::Slide, Row::Gate })
        for (int dir : { -1, 1 })
        {
            auto b = laneButton (row, dir).toFloat();
            g.setColour (panelHi);
            g.fillRoundedRectangle (b, 3.0f);
            g.setColour (ink);
            g.drawRoundedRectangle (b, 3.0f, 1.0f);
            g.setColour (slime);
            g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
            g.drawText (dir < 0 ? "<" : ">", b, juce::Justification::centred);
        }

    for (int s = 0; s < nVis; ++s)
    {
        auto col = stepColumn (s);
        const bool inPattern = s < pattern.length;

        if ((s / 4) % 2 == 0)
        {
            g.setColour (cream.withAlpha (0.04f));
            g.fillRect (col);
        }
        g.setColour (ink.withAlpha (0.5f));
        g.drawVerticalLine (col.getX(), (float) col.getY(), (float) col.getBottom());

        g.setColour (s % 4 == 0 ? cream.withAlpha (0.7f) : dim);
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.drawText (juce::String (s + 1), col.getX(), noteArea.getY() - 14, col.getWidth(), 12, juce::Justification::centred);

        if (! inPattern)
        {
            g.setColour (background.withAlpha (0.75f));
            g.fillRect (col);
            continue;
        }

        const auto& st = pattern.steps[s];

        // Note
        const int r = 11 - st.note;
        auto cell = juce::Rectangle<int> (col.getX(), noteArea.getY() + r * noteRowH, col.getWidth(), noteRowH).reduced (2, 1);
        if (st.gate == Gate::Note)
        {
            g.setColour (st.accent ? cream : slime);
            g.fillRoundedRectangle (cell.toFloat(), 2.5f);
            g.setColour (ink);
            g.drawRoundedRectangle (cell.toFloat(), 2.5f, 1.0f);
        }
        else if (st.gate == Gate::Tie)
        {
            g.setColour (slime.withAlpha (0.35f));
            g.fillRoundedRectangle (cell.toFloat(), 2.5f);
        }
        else
        {
            g.setColour (dim.withAlpha (0.45f));
            g.drawRoundedRectangle (cell.toFloat(), 2.5f, 1.0f);
        }

        // Octave
        auto oc = juce::Rectangle<int> (col.getX(), octaveRow.getY(), col.getWidth(), octaveRow.getHeight()).reduced (2, 1);
        g.setColour (st.octave == 0 ? panelHi : slime.withAlpha (0.8f));
        g.fillRoundedRectangle (oc.toFloat(), 3.0f);
        g.setColour (st.octave == 0 ? cream.withAlpha (0.45f) : onSlime());
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText (st.octave < 0 ? "DN" : (st.octave > 0 ? "UP" : "0"), oc, juce::Justification::centred);

        // Accent / Slide
        auto ac = juce::Rectangle<int> (col.getX(), accentRow.getY(), col.getWidth(), accentRow.getHeight()).reduced (2, 1);
        auto sl = juce::Rectangle<int> (col.getX(), slideRow.getY(), col.getWidth(), slideRow.getHeight()).reduced (2, 1);
        const float d = (float) juce::jmin (ac.getWidth(), ac.getHeight()) - 3.0f;
        g.setColour (st.accent ? slime : dim.withAlpha (0.4f));
        g.fillEllipse (ac.toFloat().withSizeKeepingCentre (d, d));
        if (st.accent) { g.setColour (ink); g.drawEllipse (ac.toFloat().withSizeKeepingCentre (d, d), 1.0f); }
        g.setColour (st.slide ? slime : dim.withAlpha (0.4f));
        g.fillRoundedRectangle (sl.toFloat().reduced (3.0f, 4.0f), 2.5f);

        // Gate
        auto gt = juce::Rectangle<int> (col.getX(), gateRow.getY(), col.getWidth(), gateRow.getHeight()).reduced (2, 1);
        g.setColour (st.gate == Gate::Note ? slime.withAlpha (0.25f) : ink.withAlpha (0.25f));
        g.fillRoundedRectangle (gt.toFloat(), 3.0f);
        g.setColour (st.gate == Gate::Rest ? dim : cream);
        g.drawText (st.gate == Gate::Note ? "N" : (st.gate == Gate::Tie ? "T" : "-"), gt, juce::Justification::centred);

        // Pas en cours
        if (showPlayhead && s == playingStep)
        {
            g.setColour (cream.withAlpha (0.14f));
            g.fillRect (col);
            g.setColour (slime);
            g.fillRect (col.getX() + 1, gateRow.getBottom() + 2, col.getWidth() - 2, 3);
        }

        // Curseur d'édition
        if (s == cursor && hasKeyboardFocus (true))
        {
            g.setColour (processor.stepRecord.load() ? slime : cream.withAlpha (0.7f));
            g.drawRect (col.reduced (1), 2);
        }
    }

    // Glisser-déposer MIDI
    g.setColour (slime.withAlpha (0.15f));
    g.fillRoundedRectangle (midiDragArea.toFloat(), OctoLook::get().ctrlR);
    g.setColour (slime);
    g.drawRoundedRectangle (midiDragArea.toFloat().reduced (1.0f), OctoLook::get().ctrlR, OctoLook::get().outline * 0.75f);
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawFittedText ("MIDI >> glisser vers le DAW", midiDragArea, juce::Justification::centred, 1, 0.8f);
}

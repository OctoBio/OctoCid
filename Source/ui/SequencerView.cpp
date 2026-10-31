#include "SequencerView.h"
#include "Palette.h"

octocid::Pattern SequencerView::clipboard {};
bool SequencerView::hasClipboard = false;

namespace
{
    const char* const kNoteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
}

//==============================================================================
SequencerView::SequencerView (OctoCidAudioProcessor& p) : processor (p)
{
    using namespace OctoColours;
    setWantsKeyboardFocus (true);

    // --- SEQ ON / OFF
    seqOnButton.setColour (juce::ToggleButton::textColourId, cream);
    seqOnButton.setColour (juce::ToggleButton::tickColourId, slime);
    seqOnAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, ParamIDs::seqOn.getParamID(), seqOnButton);
    addAndMakeVisible (seqOnButton);

    // --- Patterns A..H
    for (int i = 0; i < 8; ++i)
    {
        auto& b = patternButtons[i];
        b.setButtonText (juce::String::charToString ((juce::juce_wchar) ('A' + i)));
        b.onClick = [this, i] { selectPattern (i); };
        addAndMakeVisible (b);
    }

    // --- Libellés
    auto setupLabel = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        l.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (l);
    };
    setupLabel (lengthLabel, "Pas");
    setupLabel (resLabel, "Resol.");
    setupLabel (swingLabel, "Swing");
    setupLabel (gateLabel, "Gate");
    setupLabel (rootLabel, "Tonique");
    setupLabel (scaleLabel, "Gamme");
    setupLabel (densityLabel, "Densite");
    setupLabel (accDensLabel, "Accents");
    setupLabel (slideDensLabel, "Slides");
    transposeLabel.setFont (juce::FontOptions (13.0f));
    transposeLabel.setColour (juce::Label::textColourId, dim.brighter (0.5f));
    addAndMakeVisible (transposeLabel);

    // --- Longueur, résolution, swing, gate
    lengthSlider.setRange (1, octocid::Pattern::kMaxSteps, 1);
    lengthSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 36, 26);
    lengthSlider.onValueChange = [this]
    {
        const int len = (int) lengthSlider.getValue();
        if (len != pattern.length)
            edit ([len] (octocid::Pattern& pt) { pt.length = len; });
    };
    addAndMakeVisible (lengthSlider);

    resBox.addItemList (SeqRes::names, 1);
    resAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, ParamIDs::seqRes.getParamID(), resBox);
    addAndMakeVisible (resBox);

    for (auto* s : { &swingSlider, &gateSlider })
    {
        s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 46, 22);
        addAndMakeVisible (*s);
    }
    swingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamIDs::seqSwing.getParamID(), swingSlider);
    gateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamIDs::seqGate.getParamID(), gateSlider);

    // --- Générateur
    for (int i = 0; i < 12; ++i)
        rootBox.addItem (kNoteNames[i], i + 1);
    rootBox.setSelectedItemIndex (0, juce::dontSendNotification);
    for (int i = 0; i < octocid::kNumScales; ++i)
        scaleBox.addItem (juce::String::fromUTF8 (octocid::kScales[i].name), i + 1);
    scaleBox.setSelectedItemIndex (0, juce::dontSendNotification);
    density.setRange (-7, 7, 1);
    density.setValue (2, juce::dontSendNotification);
    density.setTextBoxStyle (juce::Slider::TextBoxRight, false, 30, 22);
    density.setTooltip ("-7 : tonique martelée, 0 : ligne aérée, +7 : toute la gamme, ligne pleine");
    accDensity.setRange (0.0, 1.0, 0.01);   accDensity.setValue (0.5, juce::dontSendNotification);
    slideDensity.setRange (0.0, 1.0, 0.01); slideDensity.setValue (0.5, juce::dontSendNotification);
    for (auto* c : std::initializer_list<juce::Component*> { &rootBox, &scaleBox, &density, &accDensity, &slideDensity })
        addAndMakeVisible (c);

    auto seed = [] { return (uint32_t) juce::Random::getSystemRandom().nextInt(); };
    generateButton.onClick = [this, seed]
    {
        const int root = rootBox.getSelectedItemIndex(), scale = scaleBox.getSelectedItemIndex(), d = (int) density.getValue();
        const float a = (float) accDensity.getValue(), s = (float) slideDensity.getValue();
        const uint32_t sd = seed();
        edit ([=] (octocid::Pattern& pt) { octocid::PatternTools::generate (pt, root, scale, d, a, s, sd); });
    };
    mutateButton.onClick = [this, seed]
    {
        const int root = rootBox.getSelectedItemIndex(), scale = scaleBox.getSelectedItemIndex();
        const uint32_t sd = seed();
        edit ([=] (octocid::Pattern& pt) { octocid::PatternTools::mutate (pt, root, scale, 0.45f, sd); });
    };
    quantizeButton.onClick = [this]
    {
        const int root = rootBox.getSelectedItemIndex(), scale = scaleBox.getSelectedItemIndex();
        edit ([=] (octocid::Pattern& pt) { octocid::PatternTools::quantize (pt, root, scale); });
    };
    generateButton.setTooltip ("Nouvelle ligne acid dans la gamme (densité, accents, slides)");
    mutateButton.setTooltip ("Fait évoluer la ligne actuelle : décale accents ou slides, inverse, change quelques notes");
    quantizeButton.setTooltip ("Ramène toutes les notes dans la gamme choisie");

    // --- Édition
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
    recButton.setColour (juce::TextButton::buttonOnColourId, slime);
    recButton.setColour (juce::TextButton::textColourOnId, background);
    recButton.setTooltip ("Enregistrement pas à pas : chaque note jouée au clavier MIDI est écrite au curseur. "
                          "Vélocité haute = accent, notes liées = slide.");
    recButton.onClick = [this]
    {
        processor.stepRecord.store (recButton.getToggleState());
        grabKeyboardFocus();
    };
    recButton.setToggleState (processor.stepRecord.load(), juce::dontSendNotification);

    shiftLeft.setTooltip ("Décaler tout le pattern d'un pas vers la gauche");
    shiftRight.setTooltip ("Décaler tout le pattern d'un pas vers la droite");
    transDown.setTooltip ("Transposer le pattern d'un demi-ton vers le bas");
    transUp.setTooltip ("Transposer le pattern d'un demi-ton vers le haut");

    for (auto* b : { &generateButton, &mutateButton, &quantizeButton, &shiftLeft, &shiftRight, &transDown, &transUp,
                     &copyButton, &pasteButton, &clearButton, &undoButton, &redoButton, &recButton })
        addAndMakeVisible (b);

    reloadFromProcessor();
    timerCallback();      // état des boutons dès l'ouverture
    startTimerHz (30);
}

SequencerView::~SequencerView()
{
    processor.stepRecord.store (false);
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

// Notes jouées au clavier MIDI pendant l'enregistrement pas à pas
void SequencerView::handleRecordedNotes()
{
    OctoCidAudioProcessor::RecNote rn;
    bool any = false;
    const int threshold = (int) processor.apvts.getRawParameterValue (ParamIDs::accentThreshold.getParamID())->load();

    while (processor.popRecordedNote (rn))
    {
        if (! any) pushUndo();
        any = true;

        // Une note jouée avant de relâcher la précédente = slide vers elle
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
    handleRecordedNotes();

    if (editIndex() != shownIndex)
        reloadFromProcessor();
    else if (! (processor.getPattern (shownIndex) == pattern))   // état rechargé par le DAW
        reloadFromProcessor();

    const int step = processor.seqStep.load (std::memory_order_relaxed);
    const int playing = processor.seqActivePattern.load (std::memory_order_relaxed);
    const int queued = processor.seqQueuedPattern.load (std::memory_order_relaxed);
    const int tr = processor.seqTranspose.load (std::memory_order_relaxed);

    playingPattern = playing;
    queuedPattern = queued;
    for (int i = 0; i < 8; ++i)
    {
        auto& b = patternButtons[i];
        const bool isPlaying = i == playingPattern;
        const bool isQueued = i == queuedPattern && queuedPattern != playingPattern;
        b.setToggleState (i == shownIndex, juce::dontSendNotification);
        b.setColour (juce::TextButton::buttonColourId, isPlaying ? OctoColours::slime.withAlpha (0.8f)
                                                                 : (isQueued ? OctoColours::slime.withAlpha (0.25f) : OctoColours::background));
        b.setColour (juce::TextButton::textColourOffId, isPlaying ? OctoColours::background : OctoColours::cream);
        b.setColour (juce::TextButton::buttonOnColourId, isPlaying ? OctoColours::slime : OctoColours::cream.withAlpha (0.85f));
        b.setColour (juce::TextButton::textColourOnId, OctoColours::background);
    }

    if (tr != transpose || transposeLabel.getText().isEmpty())
    {
        transpose = tr;
        transposeLabel.setText ("Transpo (note MIDI) : " + juce::String (tr > 0 ? "+" : "") + juce::String (tr) + " demi-tons",
                                juce::dontSendNotification);
    }

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
    // Rangée du haut
    seqOnButton.setBounds (12, 10, 92, 32);
    for (int i = 0; i < 8; ++i)
        patternButtons[i].setBounds (108 + i * 38, 10, 34, 32);
    lengthLabel.setBounds (410, 10, 30, 32);
    lengthSlider.setBounds (444, 10, 108, 32);
    resLabel.setBounds (556, 10, 46, 32);
    resBox.setBounds (606, 13, 72, 26);
    swingLabel.setBounds (680, 10, 44, 32);
    swingSlider.setBounds (726, 10, 118, 32);
    gateLabel.setBounds (846, 10, 36, 32);
    gateSlider.setBounds (884, 10, 104, 32);
    transposeLabel.setBounds (12, 46, 400, 20);

    // Grille
    const int x0 = 12, w = getWidth() - 24;
    noteArea  = { x0, 86, w, 12 * 17 };
    octaveRow = { x0, noteArea.getBottom() + 8, w, 26 };
    accentRow = { x0, octaveRow.getBottom() + 4, w, 26 };
    slideRow  = { x0, accentRow.getBottom() + 4, w, 26 };
    gateRow   = { x0, slideRow.getBottom() + 4, w, 26 };
    gridArea  = noteArea.getUnion (gateRow).withTop (noteArea.getY() - 22).expanded (2);

    // Générateur
    const int ty = gridArea.getBottom() + 16;
    generateButton.setBounds (12, ty, 86, 30);
    rootLabel.setBounds (100, ty, 56, 30);
    rootBox.setBounds (160, ty + 2, 58, 26);
    scaleLabel.setBounds (220, ty, 50, 30);
    scaleBox.setBounds (274, ty + 2, 128, 26);
    densityLabel.setBounds (404, ty, 56, 30);
    density.setBounds (462, ty, 120, 30);
    accDensLabel.setBounds (584, ty, 58, 30);
    accDensity.setBounds (644, ty, 86, 30);
    slideDensLabel.setBounds (732, ty, 46, 30);
    slideDensity.setBounds (780, ty, 86, 30);
    mutateButton.setBounds (872, ty, 56, 30);
    quantizeButton.setBounds (932, ty, 56, 30);

    // Édition
    const int ty2 = ty + 42;
    int bx = 12;
    for (auto* b : { &shiftLeft, &shiftRight, &transDown, &transUp })
    {
        b->setBounds (bx, ty2, 40, 30);
        bx += 44;
    }
    bx += 8;
    for (auto* b : { &copyButton, &pasteButton, &clearButton, &undoButton, &redoButton })
    {
        b->setBounds (bx, ty2, 66, 30);
        bx += 70;
    }
    bx += 8;
    recButton.setBounds (bx, ty2, 60, 30);
    midiDragArea = { bx + 70, ty2 - 2, getWidth() - 12 - (bx + 70), 34 };
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
        const int r = (y - noteArea.getY()) / 17;
        noteOut = 11 - juce::jlimit (0, 11, r);
        return Row::Note;
    }
    if (y >= octaveRow.getY() && y < octaveRow.getBottom()) return Row::Octave;
    if (y >= accentRow.getY() && y < accentRow.getBottom()) return Row::Accent;
    if (y >= slideRow.getY() && y < slideRow.getBottom())   return Row::Slide;
    if (y >= gateRow.getY() && y < gateRow.getBottom())     return Row::Gate;
    return Row::None;
}

// Petits boutons < > de décalage par ligne, dans la colonne des libellés
juce::Rectangle<int> SequencerView::laneButton (Row row, int dir) const
{
    juce::Rectangle<int> r;
    switch (row)
    {
        case Row::Note:   r = { noteArea.getX(), noteArea.getY() - 22, kLabelW, 18 }; break;
        case Row::Accent: r = accentRow.withWidth (kLabelW); break;
        case Row::Slide:  r = slideRow.withWidth (kLabelW); break;
        case Row::Gate:   r = gateRow.withWidth (kLabelW); break;
        case Row::Octave:
        case Row::None:
        default:          return {};
    }
    const int bw = 16;
    const int x = r.getRight() - 6 - (dir < 0 ? 2 * bw + 2 : bw);
    return { x, r.getCentreY() - 8, bw, 16 };
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
                st.gate = Gate::Rest;                 // re-cliquer une note l'éteint
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

    // Décalage d'une seule ligne
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
    // Glisser-déposer du pattern en fichier MIDI vers le DAW
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
// Raccourcis (comme le Transistor Bass) : flèches = curseur / demi-ton,
// Maj + haut/bas = octave, A = accent, S = slide, G = gate, N / R / T = note /
// silence / liaison, Suppr = silence, Ctrl+Z / Ctrl+Y = annuler / rétablir.
bool SequencerView::keyPressed (const juce::KeyPress& key)
{
    using octocid::Gate;
    const auto mods = key.getModifiers();
    const int code = key.getKeyCode();
    auto& st = pattern.steps[juce::jlimit (0, octocid::Pattern::kMaxSteps - 1, cursor)];

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
    juce::ignoreUnused (st);
    return false;
}

//==============================================================================
void SequencerView::paint (juce::Graphics& g)
{
    using namespace OctoColours;
    using octocid::Gate;

    g.setColour (panel);
    g.fillRoundedRectangle (gridArea.toFloat().expanded (6.0f), 10.0f);

    const int nVis = visibleSteps();
    const bool showPlayhead = playingStep >= 0 && playingPattern == shownIndex;

    // Libellés des lignes
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    for (int r = 0; r < 12; ++r)
    {
        const int note = 11 - r;
        const bool black = juce::String (kNoteNames[note]).containsChar ('#');
        auto rowRect = juce::Rectangle<int> (noteArea.getX(), noteArea.getY() + r * 17, noteArea.getWidth(), 17);
        g.setColour (black ? background.withAlpha (0.55f) : background.withAlpha (0.25f));
        g.fillRect (rowRect.withTrimmedLeft (kLabelW));
        g.setColour (black ? dim : cream.withAlpha (0.8f));
        g.drawText (kNoteNames[note], rowRect.withWidth (kLabelW - 8), juce::Justification::centredRight);
    }
    auto drawRowLabel = [&] (juce::Rectangle<int> r, const char* text)
    {
        g.setColour (cream.withAlpha (0.8f));
        g.drawText (text, r.withWidth (kLabelW - 44).withTrimmedLeft (4), juce::Justification::centredLeft);
    };
    g.setColour (cream.withAlpha (0.8f));
    g.drawText ("Notes", juce::Rectangle<int> (noteArea.getX() + 4, noteArea.getY() - 22, kLabelW - 44, 18), juce::Justification::centredLeft);
    drawRowLabel (octaveRow, "Octave");
    drawRowLabel (accentRow, "Accent");
    drawRowLabel (slideRow, "Slide");
    drawRowLabel (gateRow, "Gate");

    // Boutons < > de décalage par ligne
    for (auto row : { Row::Note, Row::Accent, Row::Slide, Row::Gate })
        for (int dir : { -1, 1 })
        {
            auto b = laneButton (row, dir).toFloat();
            g.setColour (background);
            g.fillRoundedRectangle (b, 3.0f);
            g.setColour (slime.withAlpha (0.85f));
            g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawText (dir < 0 ? "<" : ">", b, juce::Justification::centred);
        }

    for (int s = 0; s < nVis; ++s)
    {
        auto col = stepColumn (s);
        const bool inPattern = s < pattern.length;

        if ((s / 4) % 2 == 0)
        {
            g.setColour (cream.withAlpha (0.035f));
            g.fillRect (col);
        }
        g.setColour (background.withAlpha (0.6f));
        g.drawVerticalLine (col.getX(), (float) col.getY(), (float) col.getBottom());

        // Numéro de pas au-dessus de la grille
        g.setColour (s % 4 == 0 ? cream.withAlpha (0.6f) : dim);
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (juce::String (s + 1), col.getX(), noteArea.getY() - 20, col.getWidth(), 14, juce::Justification::centred);

        if (! inPattern)
        {
            g.setColour (background.withAlpha (0.7f));
            g.fillRect (col);
            continue;
        }

        const auto& st = pattern.steps[s];

        // Grille de notes
        const int r = 11 - st.note;
        auto cell = juce::Rectangle<int> (col.getX(), noteArea.getY() + r * 17, col.getWidth(), 17).reduced (2, 2);
        if (st.gate == Gate::Note)
        {
            g.setColour (st.accent ? cream : slime);
            g.fillRoundedRectangle (cell.toFloat(), 3.0f);
        }
        else if (st.gate == Gate::Tie)
        {
            g.setColour (slime.withAlpha (0.35f));
            g.fillRoundedRectangle (cell.toFloat(), 3.0f);
        }
        else
        {
            g.setColour (dim.withAlpha (0.5f));
            g.drawRoundedRectangle (cell.toFloat(), 3.0f, 1.0f);
        }

        // Octave
        auto oc = juce::Rectangle<int> (col.getX(), octaveRow.getY(), col.getWidth(), octaveRow.getHeight()).reduced (2);
        g.setColour (st.octave == 0 ? dim.withAlpha (0.6f) : slime.withAlpha (0.75f));
        g.fillRoundedRectangle (oc.toFloat(), 3.0f);
        g.setColour (st.octave == 0 ? cream.withAlpha (0.6f) : background);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (st.octave < 0 ? "DN" : (st.octave > 0 ? "UP" : "0"), oc, juce::Justification::centred);

        // Accent / Slide
        auto ac = juce::Rectangle<int> (col.getX(), accentRow.getY(), col.getWidth(), accentRow.getHeight()).reduced (2);
        auto sl = juce::Rectangle<int> (col.getX(), slideRow.getY(), col.getWidth(), slideRow.getHeight()).reduced (2);
        const float d = (float) juce::jmin (ac.getWidth(), ac.getHeight()) - 6.0f;
        g.setColour (st.accent ? slime : dim.withAlpha (0.45f));
        g.fillEllipse (ac.toFloat().withSizeKeepingCentre (d, d));
        g.setColour (st.slide ? slime : dim.withAlpha (0.45f));
        g.fillRoundedRectangle (sl.toFloat().reduced (3.0f, 7.0f), 3.0f);

        // Gate
        auto gt = juce::Rectangle<int> (col.getX(), gateRow.getY(), col.getWidth(), gateRow.getHeight()).reduced (2);
        g.setColour (st.gate == Gate::Note ? slime.withAlpha (0.25f) : background.withAlpha (0.5f));
        g.fillRoundedRectangle (gt.toFloat(), 3.0f);
        g.setColour (st.gate == Gate::Rest ? dim : cream);
        g.drawText (st.gate == Gate::Note ? "N" : (st.gate == Gate::Tie ? "T" : "-"), gt, juce::Justification::centred);

        // Pas en cours
        if (showPlayhead && s == playingStep)
        {
            g.setColour (cream.withAlpha (0.16f));
            g.fillRect (col);
            g.setColour (slime);
            g.fillRect (col.getX() + 1, noteArea.getY() - 4, col.getWidth() - 2, 3);
        }

        // Curseur d'édition (clavier, enregistrement pas à pas)
        if (s == cursor && hasKeyboardFocus (true))
        {
            g.setColour (processor.stepRecord.load() ? slime : cream.withAlpha (0.7f));
            g.drawRect (col.reduced (1), 2);
        }
    }

    // Zone de glisser-déposer MIDI
    g.setColour (slime.withAlpha (0.15f));
    g.fillRoundedRectangle (midiDragArea.toFloat(), 8.0f);
    g.setColour (slime);
    g.drawRoundedRectangle (midiDragArea.toFloat().reduced (1.0f), 8.0f, 1.5f);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("MIDI >> glisser vers le DAW", midiDragArea, juce::Justification::centred);

    // Aide
    g.setColour (dim.brighter (0.4f));
    g.setFont (juce::FontOptions (12.5f));
    auto help = juce::Rectangle<int> (12, midiDragArea.getBottom() + 14, getWidth() - 24, 70);
    g.drawFittedText ("Souris : clic = note (re-clic ou clic droit = silence), glisser = peindre ; < > a cote d'une ligne = decaler "
                      "seulement cette ligne.  Clavier : fleches = curseur / demi-ton, Maj+haut/bas = octave, A accent, S slide, "
                      "G gate, N/R/T note/silence/liaison, Ctrl+Z / Ctrl+Y.  REC : jouez au clavier MIDI, chaque note s'ecrit au curseur.  "
                      "SEQ ON : une note MIDI transpose (Do3 = 0), Do0 a Sol0 choisissent A a H (au cycle suivant).",
                      help, juce::Justification::topLeft, 5, 1.0f);
}

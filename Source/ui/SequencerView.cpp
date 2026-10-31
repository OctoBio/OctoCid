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

    // --- SEQ ON / OFF
    seqOnButton.setButtonText ("SEQ ON");
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

    // --- Longueur, résolution, swing
    auto setupLabel = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        l.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (l);
    };
    setupLabel (lengthLabel, "Pas");
    setupLabel (resLabel, "Resolution");
    setupLabel (swingLabel, "Swing");
    setupLabel (rootLabel, "Tonique");
    setupLabel (scaleLabel, "Gamme");
    setupLabel (accDensLabel, "Accents");
    setupLabel (slideDensLabel, "Slides");
    transposeLabel.setFont (juce::FontOptions (13.0f));
    transposeLabel.setColour (juce::Label::textColourId, dim.brighter (0.5f));
    addAndMakeVisible (transposeLabel);

    lengthSlider.setRange (1, octocid::Pattern::kMaxSteps, 1);
    lengthSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 40, 26);
    lengthSlider.onValueChange = [this]
    {
        const int len = (int) lengthSlider.getValue();
        if (len != pattern.length)
        {
            pattern.length = len;
            commit();
            repaint();
        }
    };
    addAndMakeVisible (lengthSlider);

    resBox.addItemList (SeqRes::names, 1);
    resAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, ParamIDs::seqRes.getParamID(), resBox);
    addAndMakeVisible (resBox);

    swingSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 22);
    swingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, ParamIDs::seqSwing.getParamID(), swingSlider);
    addAndMakeVisible (swingSlider);

    // --- Outils
    for (int i = 0; i < 12; ++i)
        rootBox.addItem (kNoteNames[i], i + 1);
    rootBox.setSelectedItemIndex (0, juce::dontSendNotification);
    for (int i = 0; i < octocid::kNumScales; ++i)
        scaleBox.addItem (juce::String::fromUTF8 (octocid::kScales[i].name), i + 1);
    scaleBox.setSelectedItemIndex (0, juce::dontSendNotification);
    accDensity.setRange (0.0, 1.0, 0.01);   accDensity.setValue (0.3, juce::dontSendNotification);
    slideDensity.setRange (0.0, 1.0, 0.01); slideDensity.setValue (0.25, juce::dontSendNotification);
    for (auto* c : std::initializer_list<juce::Component*> { &rootBox, &scaleBox, &accDensity, &slideDensity })
        addAndMakeVisible (c);

    randomButton.onClick = [this]
    {
        octocid::PatternTools::randomize (pattern, rootBox.getSelectedItemIndex(), scaleBox.getSelectedItemIndex(),
                                          (float) accDensity.getValue(), (float) slideDensity.getValue(), 0.15f,
                                          (uint32_t) juce::Random::getSystemRandom().nextInt());
        commit();
        repaint();
    };
    shiftLeft.onClick  = [this] { octocid::PatternTools::shift (pattern, -1); commit(); repaint(); };
    shiftRight.onClick = [this] { octocid::PatternTools::shift (pattern, +1); commit(); repaint(); };
    transDown.onClick  = [this] { octocid::PatternTools::transpose (pattern, -1); commit(); repaint(); };
    transUp.onClick    = [this] { octocid::PatternTools::transpose (pattern, +1); commit(); repaint(); };
    copyButton.onClick = [this] { clipboard = pattern; hasClipboard = true; };
    pasteButton.onClick = [this]
    {
        if (! hasClipboard) return;
        pattern = clipboard;
        lengthSlider.setValue (pattern.length, juce::dontSendNotification);
        commit();
        repaint();
    };
    clearButton.onClick = [this] { octocid::PatternTools::clear (pattern); commit(); repaint(); };
    for (auto* b : { &randomButton, &shiftLeft, &shiftRight, &transDown, &transUp, &copyButton, &pasteButton, &clearButton })
        addAndMakeVisible (b);

    shiftLeft.setTooltip ("Décaler le pattern d'un pas vers la gauche");
    shiftRight.setTooltip ("Décaler le pattern d'un pas vers la droite");
    transDown.setTooltip ("Transposer le pattern d'un demi-ton vers le bas");
    transUp.setTooltip ("Transposer le pattern d'un demi-ton vers le haut");

    reloadFromProcessor();
    timerCallback();      // état des boutons dès l'ouverture
    startTimerHz (30);
}

SequencerView::~SequencerView() = default;

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
    shownIndex = editIndex();
    pattern = processor.getPattern (shownIndex);
    lengthSlider.setValue (pattern.length, juce::dontSendNotification);
    repaint();
}

void SequencerView::commit()
{
    processor.setPattern (shownIndex, pattern);
}

void SequencerView::timerCallback()
{
    if (editIndex() != shownIndex)
        reloadFromProcessor();
    else if (! (processor.getPattern (shownIndex) == pattern))   // état rechargé par le DAW
        reloadFromProcessor();

    const int step = processor.seqStep.load (std::memory_order_relaxed);
    const int playing = processor.seqActivePattern.load (std::memory_order_relaxed);
    const int queued = processor.seqQueuedPattern.load (std::memory_order_relaxed);
    const int tr = processor.seqTranspose.load (std::memory_order_relaxed);

    if (playing != playingPattern || queued != queuedPattern)
    {
        playingPattern = playing;
        queuedPattern = queued;
        for (int i = 0; i < 8; ++i)
        {
            auto& b = patternButtons[i];
            const bool isPlaying = i == playingPattern;
            b.setColour (juce::TextButton::buttonColourId, isPlaying ? OctoColours::slime.withAlpha (0.8f) : OctoColours::background);
            b.setColour (juce::TextButton::textColourOffId, isPlaying ? OctoColours::background : OctoColours::cream);
        }
    }

    if (tr != transpose || transposeLabel.getText().isEmpty())
    {
        transpose = tr;
        transposeLabel.setText ("Transpo (note MIDI) : " + juce::String (tr > 0 ? "+" : "") + juce::String (tr) + " demi-tons",
                                juce::dontSendNotification);
    }

    if (step != playingStep)
    {
        auto dirty = gridArea;
        playingStep = step;
        repaint (dirty);
    }

    // Pattern édité : bouton enfoncé (crème) ; pattern qui joue : vert
    for (int i = 0; i < 8; ++i)
    {
        auto& b = patternButtons[i];
        b.setToggleState (i == shownIndex, juce::dontSendNotification);
        b.setColour (juce::TextButton::buttonOnColourId, i == playingPattern ? OctoColours::slime : OctoColours::cream.withAlpha (0.85f));
        b.setColour (juce::TextButton::textColourOnId, OctoColours::background);
    }
}

//==============================================================================
void SequencerView::resized()
{
    // Rangée du haut
    seqOnButton.setBounds (12, 10, 96, 32);
    for (int i = 0; i < 8; ++i)
        patternButtons[i].setBounds (116 + i * 40, 10, 36, 32);
    lengthLabel.setBounds (440, 10, 40, 32);
    lengthSlider.setBounds (484, 10, 120, 32);
    resLabel.setBounds (610, 10, 80, 32);
    resBox.setBounds (694, 13, 80, 26);
    swingLabel.setBounds (780, 10, 50, 32);
    swingSlider.setBounds (834, 10, 154, 32);
    transposeLabel.setBounds (12, 44, 400, 20);

    // Grille
    const int x0 = 12, w = getWidth() - 24;
    noteArea  = { x0, 72, w, 12 * 17 };
    octaveRow = { x0, noteArea.getBottom() + 8, w, 26 };
    accentRow = { x0, octaveRow.getBottom() + 4, w, 26 };
    slideRow  = { x0, accentRow.getBottom() + 4, w, 26 };
    gateRow   = { x0, slideRow.getBottom() + 4, w, 26 };
    gridArea  = noteArea.getUnion (gateRow).expanded (2);

    // Outils
    const int ty = gridArea.getBottom() + 16;
    randomButton.setBounds (12, ty, 110, 30);
    rootLabel.setBounds (126, ty, 60, 30);
    rootBox.setBounds (190, ty + 2, 62, 26);
    scaleLabel.setBounds (256, ty, 56, 30);
    scaleBox.setBounds (316, ty + 2, 140, 26);
    accDensLabel.setBounds (460, ty, 64, 30);
    accDensity.setBounds (528, ty, 110, 30);
    slideDensLabel.setBounds (640, ty, 54, 30);
    slideDensity.setBounds (698, ty, 110, 30);

    const int ty2 = ty + 42;
    int bx = 12;
    for (auto* b : { &shiftLeft, &shiftRight, &transDown, &transUp })
    {
        b->setBounds (bx, ty2, 46, 30);
        bx += 50;
    }
    bx += 12;
    for (auto* b : { &copyButton, &pasteButton, &clearButton })
    {
        b->setBounds (bx, ty2, 76, 30);
        bx += 80;
    }
    midiDragArea = { 640, ty2 - 4, getWidth() - 652, 38 };
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
    if (octaveRow.contains (octaveRow.getX() + 1, y)) return Row::Octave;
    if (accentRow.contains (accentRow.getX() + 1, y)) return Row::Accent;
    if (slideRow.contains (slideRow.getX() + 1, y))   return Row::Slide;
    if (gateRow.contains (gateRow.getX() + 1, y))     return Row::Gate;
    return Row::None;
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
                dragValue = st.octave >= 1;           // mémorise pour le glissé
            st.octave = (int8_t) (isDrag ? pattern.steps[juce::jmax (0, step - 1)].octave
                                         : (st.octave >= 1 ? -1 : st.octave + 1));
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
    midiDragStarted = false;
    if (midiDragArea.contains (e.getPosition()))
        return;
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
            const auto file = octocid::PatternIO::writeMidiFile (pattern, processor.getStepBeats(), 36 + transpose, name);
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
        g.drawText (text, r.withWidth (kLabelW - 8), juce::Justification::centredRight);
    };
    drawRowLabel (octaveRow, "Octave");
    drawRowLabel (accentRow, "Accent");
    drawRowLabel (slideRow, "Slide");
    drawRowLabel (gateRow, "Gate");

    for (int s = 0; s < nVis; ++s)
    {
        auto col = stepColumn (s);
        const bool inPattern = s < pattern.length;

        // Repères de temps (un groupe de 4 pas sur 2 légèrement éclairé)
        if ((s / 4) % 2 == 0)
        {
            g.setColour (cream.withAlpha (0.035f));
            g.fillRect (col);
        }
        g.setColour (background.withAlpha (0.6f));
        g.drawVerticalLine (col.getX(), (float) col.getY(), (float) col.getBottom());

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
            g.fillRect (col.getX() + 1, noteArea.getY() - 6, col.getWidth() - 2, 3);
        }
    }

    // Zone de glisser-déposer MIDI
    g.setColour (slime.withAlpha (0.15f));
    g.fillRoundedRectangle (midiDragArea.toFloat(), 8.0f);
    g.setColour (slime);
    g.drawRoundedRectangle (midiDragArea.toFloat().reduced (1.0f), 8.0f, 1.5f);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("MIDI  >>  glisser vers Bitwig", midiDragArea, juce::Justification::centred);

    // Aide
    g.setColour (dim.brighter (0.4f));
    g.setFont (juce::FontOptions (12.5f));
    auto help = juce::Rectangle<int> (12, midiDragArea.getBottom() + 16, getWidth() - 24, 60);
    g.drawFittedText ("Clic : poser une note (re-clic = silence, clic droit = silence).  Glisser : peindre notes, accents, slides.  "
                      "Gate : N = note, - = silence, T = liaison.  SEQ ON : joue en rythme avec le DAW ; une note MIDI transpose "
                      "(Do3 = 0), Do0 a Sol0 choisissent les patterns A a H (au cycle suivant).",
                      help, juce::Justification::topLeft, 4, 1.0f);
}

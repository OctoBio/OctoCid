#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

//==============================================================================
// Page SÉQUENCEUR.
// Grille façon Transistor Bass : une grille de notes, puis les lignes
// Octave, Accent, Slide et Gate, chacune décalable seule.
// Outils : Générer (densité bipolaire), Varier, Gamme (quantifier), décalage,
// transposition, copier / coller / effacer, annuler / rétablir,
// enregistrement pas à pas au clavier MIDI, raccourcis clavier,
// glisser-déposer MIDI vers le DAW.
//==============================================================================
class SequencerView : public juce::Component,
                      private juce::Timer
{
public:
    explicit SequencerView (OctoCidAudioProcessor&);
    ~SequencerView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    void timerCallback() override;

    // Édition
    void commit();                                   // envoie le pattern édité au processeur
    void edit (const std::function<void (octocid::Pattern&)>& change);   // annulable
    void pushUndo();
    void undo();
    void redo();
    void reloadFromProcessor();
    int  editIndex() const;
    void selectPattern (int index);
    void editCell (juce::Point<int> pos, bool isDrag, bool rightClick);
    void handleRecordedNotes();
    void moveCursor (int delta);

    // Géométrie de la grille
    int visibleSteps() const noexcept { return pattern.length <= 16 ? 16 : 32; }
    juce::Rectangle<int> stepColumn (int step) const;
    int stepAt (int x) const;
    enum class Row { None, Note, Octave, Accent, Slide, Gate };
    Row rowAt (int y, int& noteOut) const;
    juce::Rectangle<int> laneButton (Row row, int dir) const;

    OctoCidAudioProcessor& processor;
    octocid::Pattern pattern;
    int shownIndex = -1;
    int playingStep = -1, playingPattern = -1, queuedPattern = -1, transpose = 0;
    int cursor = 0;

    std::vector<octocid::Pattern> undoStack, redoStack;
    static constexpr size_t kMaxUndo = 64;

    // Pinceau de la souris (glisser pour peindre plusieurs pas)
    Row dragRow = Row::None;
    bool dragValue = false;
    int dragOctave = 0;
    int lastDragStep = -1;

    // Zones
    juce::Rectangle<int> gridArea, noteArea, octaveRow, accentRow, slideRow, gateRow, midiDragArea;
    static constexpr int kLabelW = 98;

    // Contrôles
    juce::ToggleButton seqOnButton { "SEQ ON" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqOnAttachment;
    juce::TextButton patternButtons[8];
    juce::Slider lengthSlider { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    juce::Label lengthLabel, resLabel, swingLabel, gateLabel, transposeLabel, rootLabel, scaleLabel,
                densityLabel, accDensLabel, slideDensLabel;
    juce::ComboBox resBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> resAttachment;
    juce::Slider swingSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight },
                 gateSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> swingAttachment, gateAttachment;

    juce::TextButton generateButton { "Generer" }, mutateButton { "Varier" }, quantizeButton { "Gamme" },
                     shiftLeft { "<<" }, shiftRight { ">>" }, transDown { "-1" }, transUp { "+1" },
                     copyButton { "Copier" }, pasteButton { "Coller" }, clearButton { "Effacer" },
                     undoButton { "Annuler" }, redoButton { "Retablir" }, recButton { "REC" };
    juce::ComboBox rootBox, scaleBox;
    juce::Slider density { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight },
                 accDensity { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox },
                 slideDensity { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };

    bool midiDragStarted = false;
    static octocid::Pattern clipboard;
    static bool hasClipboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerView)
};

#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

//==============================================================================
// SÉQUENCEUR (bas de la fenêtre, toute la largeur).
// Grille façon Transistor Bass : une grille de notes, puis les lignes
// Octave, Accent, Slide et Gate, chacune décalable seule.
// Outils : Générer (+ réglages), Varier, Gamme, décalage, transposition,
// copier / coller / effacer, annuler / rétablir, REC pas à pas au clavier MIDI,
// raccourcis clavier, glisser-déposer MIDI vers le DAW.
//==============================================================================
class SequencerView : public juce::Component,
                      private juce::Timer
{
public:
    explicit SequencerView (OctoCidAudioProcessor&);
    ~SequencerView() override;

    void paint (juce::Graphics&) override;

    // Banques d'usine : chargement (avec sauvegarde de la banque actuelle) et retour en arrière
    void loadFactoryBank (int index);
    void restorePreviousBank();
    bool canRestoreBank() const noexcept { return hasBankBackup; }
    void resized() override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

    // Réglages du générateur (modifiés par la bulle « Réglages »)
    int genRoot = 0, genScale = 0, genDensity = 2;
    float genAccents = 0.5f, genSlides = 0.5f;

private:
    void timerCallback() override;

    // Édition
    void commit();
    void edit (const std::function<void (octocid::Pattern&)>& change);
    void pushUndo();
    void undo();
    void redo();
    void reloadFromProcessor();
    int  editIndex() const;
    void selectPattern (int index);
    void editCell (juce::Point<int> pos, bool isDrag, bool rightClick);
    void handleRecordedNotes();
    void moveCursor (int delta);
    void showGeneratorSettings();

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
    octocid::PatternBank bankBackup;
    bool hasBankBackup = false;
    static constexpr size_t kMaxUndo = 64;

    Row dragRow = Row::None;
    bool dragValue = false;
    int dragOctave = 0;
    int lastDragStep = -1;

    // Zones
    juce::Rectangle<int> gridArea, noteArea, octaveRow, accentRow, slideRow, gateRow, midiDragArea;
    int noteRowH = 10;
    static constexpr int kLabelW = 92;

    // Contrôles
    juce::TextButton patternButtons[8];
    juce::Slider lengthSlider { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    juce::Label lengthLabel, swingLabel, gateLabel, transposeLabel;
    juce::ComboBox resBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> resAttachment;
    juce::Slider swingSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight },
                 gateSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> swingAttachment, gateAttachment;

    juce::TextButton generateButton { "Generer" }, genSettingsButton { "..." }, mutateButton { "Varier" },
                     quantizeButton { "Gamme" }, shiftLeft { "<<" }, shiftRight { ">>" }, transDown { "-1" }, transUp { "+1" },
                     copyButton { "Copier" }, pasteButton { "Coller" }, clearButton { "Effacer" },
                     undoButton { "Annuler" }, redoButton { "Retablir" }, recButton { "REC" };

    bool midiDragStarted = false;
    static octocid::Pattern clipboard;
    static bool hasClipboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerView)
};

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/OctoLookAndFeel.h"
#include "ui/Widgets.h"
#include "ui/Mascot.h"
#include "ui/SequencerView.h"

//==============================================================================
// Interface OctoCid (étape 5).
//  1. Barre du haut : logo, presets, SEQ ON/OFF, escargot On/Off, qualité, menu
//     (zoom 100 / 125 / 150 %, palette).
//  2. Zone mascotte au centre, entourée du DRIVE et du LFO.
//  3. Panneau principal (potards 303) + bouton du tiroir SLIME MODS, qui se
//     déplie en douceur au-dessus, sans masquer le séquenceur.
//  4. Séquenceur sur toute la largeur.
// Taille de base 1000 x 620, tout est dessiné en vectoriel.
//==============================================================================
class OctoCidAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit OctoCidAudioProcessorEditor (OctoCidAudioProcessor&);
    ~OctoCidAudioProcessorEditor() override;

    void paint (juce::Graphics&) override {}
    void resized() override;

    static constexpr int kWidth = 1000, kHeight = 620;

private:
    void timerCallback() override;
    void refreshPresetBox();
    void showMenu();
    void setTheme (int index);
    void applyAppearance();
    void rebuildPresetBox();
    void savePresetDialog();
    void toggleDrawer (int page);

    //==========================================================================
    // Contenu à l'échelle 1 (le zoom est une transformation de ce composant)
    class Content : public juce::Component
    {
    public:
        explicit Content (OctoCidAudioProcessorEditor& e) : editor (e) {}
        void paint (juce::Graphics&) override;
        OctoCidAudioProcessorEditor& editor;
    };

    // Tiroir SLIME MODS + PUNCH / FX
    class Drawer : public juce::Component
    {
    public:
        explicit Drawer (OctoCidAudioProcessor&);
        void paint (juce::Graphics&) override;
        void resized() override;
        void setPage (int newPage);
        int getPage() const noexcept { return page; }
    private:
        int page = 0;   // 0 : SLIME MODS + PUNCH/FX, 1 : EFFETS
        std::vector<std::unique_ptr<juce::Component>> slimeItems, fxItems;
        std::vector<std::unique_ptr<juce::Component>> hitItems, jetItems, pumpItems, stereoItems;
        std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
        juce::Component* addCombo (OctoCidAudioProcessor&, const juce::ParameterID&, const juce::String& name,
                                   std::vector<std::unique_ptr<juce::Component>>& into);
    };

    OctoCidAudioProcessor& processor;
    OctoLookAndFeel lnf;
    Content content { *this };
    std::unique_ptr<juce::TooltipWindow> tooltips;

    // Barre du haut
    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, menuButton { "..." };
    juce::ToggleButton seqToggle { "SEQ" }, snailToggle { "Escargot" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqAttachment, snailAttachment;
    std::unique_ptr<octoui::Segmented> quality;

    // Drive / LFO
    std::unique_ptr<octoui::Segmented> driveType, drivePos, lfoShape, lfoDest;
    std::unique_ptr<octoui::Knob> drive, driveTone, driveMix, lfoRate, lfoDepth;
    juce::ComboBox lfoSync;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoSyncAttachment;

    // Mascotte
    MascotView mascot;

    // Panneau 303
    std::unique_ptr<octoui::Knob> tune, cutoff, resonance, envMod, decay, accent, volume;
    std::unique_ptr<octoui::Segmented> waveform;
    juce::TextButton drawerButton { "SLIME MODS" }, effectsButton { "EFFETS" };

    // Tiroir et séquenceur
    Drawer drawer;
    SequencerView sequencer;

    // Zones (coordonnées du contenu)
    juce::Rectangle<int> topBar, drivePanel, mascotArea, lfoPanel, mainPanel, seqArea, upperArea, ledArea;

    float drawerFrac = 0.0f, drawerTarget = 0.0f;
    int displayedNote = -1;
    bool displayedAccent = false;
    int displayedProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctoCidAudioProcessorEditor)
};

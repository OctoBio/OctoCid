#pragma once

#include <JuceHeader.h>
#include "Widgets.h"
#include "LfoEditor.h"
#include "../Params.h"
#include "../dsp/ModLfo.h"
#include <array>

//==============================================================================
// 3 LFO assignables.
//  - Panneau LFO (à droite de l'escargot) : onglets 1 / 2 / 3, éditeur de forme à la
//    souris, Forme / Synchro / Mode, Vitesse, Phase, et le bouton du GRAND ÉDITEUR.
//  - Grand éditeur (page du tiroir) : grande zone de dessin, formes prêtes, grille,
//    aimant, pinceau (marches / rampes), inverser, miroir, annuler, et tous les réglages.
//  - Pour moduler un potard : attraper un onglet (ou « ✥ LFO n ») et le lâcher dessus.
//==============================================================================
namespace lfoui
{
    // Onglets 1 / 2 / 3 : clic = choisir, glisser = assigner à un potard
    class Tabs : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        explicit Tabs (LfoEditShared& s, bool isVertical = false) : shared (s), vertical (isVertical)
        {
            setTooltip (juce::String::fromUTF8 ("Clic : choisir le LFO. Glisser un onglet sur un potard : le moduler."));
        }
        void paint (juce::Graphics& g) override
        {
            for (int i = 0; i < 3; ++i)
            {
                auto r = cell (i).reduced (1.5f);
                const auto col = octoui::ModHost::lfoColour (i);
                const bool on = i == shared.selected;
                g.setColour (on ? col : OctoColours::panelHi.withAlpha (0.9f));
                g.fillRect (r);
                g.setColour (on ? col.brighter (0.4f) : col.withAlpha (0.8f));
                g.drawRect (r, 1.5f);
                g.setColour (on ? OctoColours::ink : OctoColours::cream);
                g.setFont (OctoLook::monoFont (12.0f));
                g.drawText (juce::String (i + 1), r, juce::Justification::centred);
            }
        }
        void mouseDown (const juce::MouseEvent& e) override { pressed = indexAt (e.position); started = false; }
        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (started || pressed < 0 || e.getDistanceFromDragStart() < 5) return;
            if (auto* dc = juce::DragAndDropContainer::findParentDragContainerFor (this))
            {
                started = true;
                juce::Image img (juce::Image::ARGB, 64, 22, true);
                {
                    juce::Graphics g (img);
                    g.setColour (octoui::ModHost::lfoColour (pressed)); g.fillRoundedRectangle (0, 0, 64, 22, 4);
                    g.setColour (OctoColours::ink); g.setFont (OctoLook::monoFont (12.0f));
                    g.drawText ("LFO " + juce::String (pressed + 1), 0, 0, 64, 22, juce::Justification::centred);
                }
                dc->startDragging ("octolfo:" + juce::String (pressed), this, juce::ScaledImage (img), true);
            }
        }
        void mouseUp (const juce::MouseEvent&) override { if (! started && pressed >= 0) shared.select (pressed); }
    private:
        juce::Rectangle<float> cell (int i) const
        {
            auto b = getLocalBounds().toFloat();
            return vertical ? juce::Rectangle<float> (b.getX(), b.getHeight() / 3.0f * (float) i, b.getWidth(), b.getHeight() / 3.0f)
                            : juce::Rectangle<float> (b.getWidth() / 3.0f * (float) i, b.getY(), b.getWidth() / 3.0f, b.getHeight());
        }
        int indexAt (juce::Point<float> p) const { for (int i = 0; i < 3; ++i) if (cell (i).contains (p)) return i; return -1; }
        LfoEditShared& shared;
        bool vertical;
        int pressed = -1;
        bool started = false;
    };

    // Les réglages d'un LFO (on en crée un jeu par LFO, seul celui choisi est visible)
    struct Controls
    {
        juce::ComboBox shape, sync, mode, pol;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAtt, syncAtt, modeAtt, polAtt;
        std::unique_ptr<octoui::Knob> rate, phase, delay, fade, smooth;

        void build (juce::Component& owner, juce::AudioProcessorValueTreeState& st, int i, bool full)
        {
            auto combo = [&] (juce::ComboBox& box, const char* what, const juce::StringArray& items, const juce::String& tip,
                              std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& att)
            {
                box.addItemList (items, 1);
                box.setTooltip (tip);
                att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (st, ModParams::lfoId (i, what), box);
                owner.addChildComponent (box);
            };
            juce::StringArray syncNames;
            for (auto* n : octocid::ModSync::names) syncNames.add (juce::String::fromUTF8 (n));
            combo (shape, "shape", ModParams::shapeNames, juce::String::fromUTF8 ("Forme (Dessin : la forme que tu as dessinée)"), shapeAtt);
            combo (sync, "sync", syncNames, juce::String::fromUTF8 ("Hz : vitesse libre ; sinon calée sur le tempo"), syncAtt);
            combo (mode, "mode", ModParams::modeNames,
                   juce::String::fromUTF8 ("Libre : tourne en continu | Note : repart à chaque note détachée | Synchro : calé sur la "
                                           "lecture du DAW | Un coup : un seul cycle (enveloppe) | Aléatoire : phase au hasard à chaque note"),
                   modeAtt);
            if (full) combo (pol, "pol", juce::StringArray { "Bipolaire", "Unipolaire" }, juce::String::fromUTF8 ("Bipolaire : autour du réglage ; unipolaire : au-dessus"), polAtt);
            auto knob = [&] (std::unique_ptr<octoui::Knob>& k, const char* what, const juce::String& name)
            {
                k = std::make_unique<octoui::Knob> (st, juce::ParameterID { ModParams::lfoId (i, what), 1 }, name);
                owner.addChildComponent (*k);
            };
            knob (rate, "rate", "Vitesse");
            knob (phase, "phase", "Phase");
            if (full)
            {
                knob (delay, "delay", juce::String::fromUTF8 ("Délai"));
                knob (fade, "fade", juce::String::fromUTF8 ("Montée"));
                knob (smooth, "smooth", "Lissage");
            }
        }
        juce::Array<juce::Component*> all()
        {
            juce::Array<juce::Component*> a { &shape, &sync, &mode };
            if (polAtt != nullptr) a.add (&pol);
            for (auto* k : { rate.get(), phase.get(), delay.get(), fade.get(), smooth.get() }) if (k != nullptr) a.add (k);
            return a;
        }
        void show (bool v) { for (auto* c : all()) c->setVisible (v); }
        void dimRate (juce::AudioProcessorValueTreeState& st, int i)
        {
            const bool free = st.getRawParameterValue (ModParams::lfoId (i, "sync"))->load() < 0.5f;
            rate->setAlpha (free ? 1.0f : 0.35f);
        }
    };
}

//==============================================================================
class LfoPanel : public juce::Component
{
public:
    LfoPanel (OctoCidAudioProcessor& p, LfoEditShared& s) : proc (p), shared (s), tabs (s), editor (p, s, true)
    {
        addAndMakeVisible (tabs);
        addAndMakeVisible (editor);
        for (int i = 0; i < ModParams::numLfos; ++i) controls[(size_t) i].build (*this, p.apvts, i, false);
        bigButton.setButtonText (juce::String::fromUTF8 ("GRAND ÉDITEUR"));
        bigButton.setComponentID ("lfoBigButton");
        bigButton.setTooltip (juce::String::fromUTF8 ("Ouvre le grand éditeur de LFO (formes prêtes, grille, pinceau, tous les réglages)"));
        bigButton.onClick = [this] { if (onOpenBig) onOpenBig(); };
        addAndMakeVisible (bigButton);
        shared.selectListeners.push_back ([this] (int i) { select (i); });
        select (shared.selected);
    }

    std::function<void()> onOpenBig;

    void resized() override
    {
        auto r = getLocalBounds().reduced (12, 0);
        auto row1 = r.withTrimmedTop (22).withHeight (22);
        tabs.setBounds (row1.removeFromLeft (84));
        row1.removeFromLeft (6);
        auto row2 = r.withTrimmedTop (50).withTrimmedBottom (6);
        editor.setBounds (row2.removeFromLeft (220));
        row2.removeFromLeft (4);
        bigButton.setBounds (row2.removeFromBottom (20));
        for (auto& c : controls)
        {
            auto rr = row1;
            c.shape.setBounds (rr.removeFromLeft (88)); rr.removeFromLeft (4);
            c.sync.setBounds (rr.removeFromLeft (76));  rr.removeFromLeft (4);
            c.mode.setBounds (rr);
            auto k = row2;
            c.rate->setBounds (k.removeFromLeft (k.getWidth() / 2));
            c.phase->setBounds (k);
        }
    }

    void tick()
    {
        controls[(size_t) shared.selected].dimRate (proc.apvts, shared.selected);
        editor.repaint();
    }

    LfoEditor& getEditor() noexcept { return editor; }

private:
    void select (int i)
    {
        for (int k = 0; k < ModParams::numLfos; ++k) controls[(size_t) k].show (k == i);
        tabs.repaint();
        editor.repaint();
    }

    OctoCidAudioProcessor& proc;
    LfoEditShared& shared;
    lfoui::Tabs tabs;
    LfoEditor editor;
    juce::TextButton bigButton;
    std::array<lfoui::Controls, ModParams::numLfos> controls;
};

//==============================================================================
// Grand éditeur (page LFO du tiroir)
class LfoBigPage : public juce::Component
{
public:
    LfoBigPage (OctoCidAudioProcessor& p, LfoEditShared& s) : proc (p), shared (s), tabs (s, true), editor (p, s, false)
    {
        addAndMakeVisible (tabs);
        addAndMakeVisible (editor);
        for (int i = 0; i < ModParams::numLfos; ++i) controls[(size_t) i].build (*this, p.apvts, i, true);

        auto button = [this] (juce::TextButton& b, const juce::String& text, const juce::String& tip, std::function<void()> f)
        {
            b.setButtonText (text); b.setTooltip (tip); b.onClick = std::move (f);
            addAndMakeVisible (b);
        };
        button (presets, juce::String::fromUTF8 ("FORMES ▾"), juce::String::fromUTF8 ("Formes prêtes (pompe, rebond, escaliers, porte trance…)"),
                [this] { editor.showPresetMenu (&presets); });
        button (flip, "INVERSER", "Retourner la forme (haut / bas)", [this] { editor.flip(); });
        button (mirror, "MIROIR", "Retourner la forme (gauche / droite)", [this] { editor.mirror(); });
        button (undo, "ANNULER", juce::String::fromUTF8 ("Annuler la dernière modification de forme"), [this] { editor.undo(); });
        button (snap, "AIMANT", juce::String::fromUTF8 ("Aimanter les points à la grille (Ctrl en glissant : l'inverse)"),
                [this] { shared.snap = ! shared.snap; refreshButtons(); });
        const char* brushNames[3] = { "POINTS", "MARCHES", "RAMPES" };
        const char* brushTips[3] = { "Dessin libre avec des points et des courbes",
                                     "Pinceau : peindre des marches dans la grille, comme un step séquenceur",
                                     "Pinceau : peindre des rampes (pompe, porte trance)" };
        for (int b = 0; b < 3; ++b)
            button (brush[(size_t) b], brushNames[b], juce::String::fromUTF8 (brushTips[b]), [this, b] { shared.brush = b; refreshButtons(); shared.repaintAll(); });

        for (int v : LfoEditShared::gridXChoices) gridX.addItem (juce::String (v), v);
        for (int v : LfoEditShared::gridYChoices) gridY.addItem (juce::String (v), v);
        gridX.setTooltip ("Grille horizontale (cases par cycle)");
        gridY.setTooltip ("Grille verticale (niveaux)");
        gridX.onChange = [this] { shared.gridX = juce::jmax (1, gridX.getSelectedId()); shared.repaintAll(); };
        gridY.onChange = [this] { shared.gridY = juce::jmax (1, gridY.getSelectedId()); shared.repaintAll(); };
        addAndMakeVisible (gridX); addAndMakeVisible (gridY);

        button (close, juce::String::fromUTF8 ("FERMER ×"), "Refermer le grand éditeur", [this] { if (onClose) onClose(); });

        shared.selectListeners.push_back ([this] (int i) { select (i); });
        select (shared.selected);
        refreshButtons();
    }

    std::function<void()> onClose;

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.setFont (OctoLook::monoFont (12.0f));
        g.drawText ("X", gridXLabel, juce::Justification::centredRight);
        g.drawText ("Y", gridYLabel, juce::Justification::centredRight);
    }

    void resized() override
    {
        close.setBounds (getWidth() - 96, 3, 84, 18);
        auto r = getLocalBounds().reduced (10, 0).withTrimmedTop (22).withTrimmedBottom (6);
        tabs.setBounds (r.removeFromLeft (30).withTrimmedTop (2));
        r.removeFromLeft (8);
        auto right = r.removeFromRight (360);
        r.removeFromRight (8);
        auto bar = r.removeFromTop (22);
        r.removeFromTop (4);
        editor.setBounds (r);

        presets.setBounds (bar.removeFromLeft (78)); bar.removeFromLeft (6);
        for (auto& b : brush) { b.setBounds (bar.removeFromLeft (66)); bar.removeFromLeft (2); }
        bar.removeFromLeft (6);
        snap.setBounds (bar.removeFromLeft (58));

        auto tools = right.removeFromBottom (22);
        right.removeFromBottom (4);
        flip.setBounds (tools.removeFromLeft (70)); tools.removeFromLeft (3);
        mirror.setBounds (tools.removeFromLeft (60)); tools.removeFromLeft (3);
        undo.setBounds (tools.removeFromLeft (68)); tools.removeFromLeft (6);
        gridXLabel = tools.removeFromLeft (14); gridX.setBounds (tools.removeFromLeft (52)); tools.removeFromLeft (4);
        gridYLabel = tools.removeFromLeft (14); gridY.setBounds (tools.removeFromLeft (52));

        for (auto& c : controls)
        {
            auto row = right.withHeight (22);
            const int w = (row.getWidth() - 12) / 4;
            c.shape.setBounds (row.removeFromLeft (w)); row.removeFromLeft (4);
            c.sync.setBounds (row.removeFromLeft (w));  row.removeFromLeft (4);
            c.mode.setBounds (row.removeFromLeft (w));  row.removeFromLeft (4);
            c.pol.setBounds (row);
            auto k = right.withTrimmedTop (28);
            const int kw = k.getWidth() / 5;
            for (auto* kn : { c.rate.get(), c.phase.get(), c.delay.get(), c.fade.get(), c.smooth.get() })
                kn->setBounds (k.removeFromLeft (kw));
        }
    }

    void tick()
    {
        if (! isShowing()) return;
        controls[(size_t) shared.selected].dimRate (proc.apvts, shared.selected);
        editor.repaint();
    }

private:
    void select (int i)
    {
        for (int k = 0; k < ModParams::numLfos; ++k) controls[(size_t) k].show (k == i);
        tabs.repaint();
    }
    void refreshButtons()
    {
        snap.setToggleState (shared.snap, juce::dontSendNotification);
        for (int b = 0; b < 3; ++b) brush[(size_t) b].setToggleState (shared.brush == b, juce::dontSendNotification);
        gridX.setSelectedId (shared.gridX, juce::dontSendNotification);
        gridY.setSelectedId (shared.gridY, juce::dontSendNotification);
    }

    OctoCidAudioProcessor& proc;
    LfoEditShared& shared;
    lfoui::Tabs tabs;
    LfoEditor editor;
    juce::TextButton presets, flip, mirror, undo, snap, close;
    std::array<juce::TextButton, 3> brush;
    juce::ComboBox gridX, gridY;
    juce::Rectangle<int> gridXLabel, gridYLabel;
    std::array<lfoui::Controls, ModParams::numLfos> controls;
};

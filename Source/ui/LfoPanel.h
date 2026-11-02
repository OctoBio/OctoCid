#pragma once

#include <JuceHeader.h>
#include "Widgets.h"
#include "../Params.h"
#include "../dsp/ModLfo.h"
#include <array>

//==============================================================================
// Panneau LFO : 3 LFO assignables, un onglet par LFO (même place que l'ancien LFO).
//  - écran animé de la forme d'onde : on l'attrape et on le lâche sur un potard
//    pour créer une modulation ; clic sur « ± » : bipolaire / unipolaire ;
//  - Forme, Synchro, Mode ; Vitesse, Phase, Délai, Fondu, Lissage.
//==============================================================================
class LfoPanel : public juce::Component
{
public:
    using Views = std::array<std::atomic<float>, ModParams::numLfos>;

    LfoPanel (juce::AudioProcessorValueTreeState& s, Views& values, Views& phases)
        : state (s), lfoValues (values), lfoPhases (phases)
    {
        addAndMakeVisible (tabs);
        tabs.onChange = [this] (int i) { select (i); };
        for (int i = 0; i < ModParams::numLfos; ++i)
        {
            auto& c = controls[(size_t) i];
            auto combo = [&] (juce::ComboBox& box, const char* what, const juce::StringArray& items, const juce::String& tip,
                              std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& att)
            {
                box.addItemList (items, 1);
                box.setTooltip (tip);
                att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, ModParams::lfoId (i, what), box);
                addChildComponent (box);
            };
            juce::StringArray syncNames;
            for (auto* n : octocid::ModSync::names) syncNames.add (juce::String::fromUTF8 (n));
            combo (c.shape, "shape", ModParams::shapeNames, "Forme du LFO", c.shapeAtt);
            combo (c.sync, "sync", syncNames, juce::String::fromUTF8 ("Libre (Vitesse en Hz) ou calé sur le tempo"), c.syncAtt);
            combo (c.mode, "mode", ModParams::modeNames,
                   juce::String::fromUTF8 ("Libre : tourne en continu | Note : repart à chaque note détachée | Synchro : calé sur la "
                                           "lecture du DAW | Un coup : un seul cycle (enveloppe) | Aléatoire : phase au hasard à chaque note"),
                   c.modeAtt);
            auto knob = [&] (std::unique_ptr<octoui::Knob>& k, const char* what, const juce::String& name)
            {
                k = std::make_unique<octoui::Knob> (state, juce::ParameterID { ModParams::lfoId (i, what), 1 }, name);
                addChildComponent (*k);
            };
            knob (c.rate, "rate", "Vitesse");
            knob (c.phase, "phase", "Phase");
            knob (c.delay, "delay", juce::String::fromUTF8 ("Délai"));
            knob (c.fade, "fade", "Fondu");
            knob (c.smooth, "smooth", "Lissage");
        }
        addAndMakeVisible (display);
        display.owner = this;
        select (0);
    }

    int selected() const noexcept { return current; }

    void resized() override
    {
        auto r = getLocalBounds().reduced (12, 0);
        auto row1 = r.withTrimmedTop (22).withHeight (22);
        tabs.setBounds (row1.removeFromLeft (84));
        row1.removeFromLeft (6);
        for (auto& c : controls)
        {
            auto rr = row1;
            c.shape.setBounds (rr.removeFromLeft (88)); rr.removeFromLeft (4);
            c.sync.setBounds (rr.removeFromLeft (76));  rr.removeFromLeft (4);
            c.mode.setBounds (rr);
        }
        auto row2 = r.withTrimmedTop (50).withTrimmedBottom (6);
        display.setBounds (row2.removeFromLeft (104).reduced (0, 4));
        row2.removeFromLeft (4);
        const int kw = row2.getWidth() / 5;
        for (auto& c : controls)
        {
            auto rr = row2;
            for (auto* k : { c.rate.get(), c.phase.get(), c.delay.get(), c.fade.get(), c.smooth.get() })
                k->setBounds (rr.removeFromLeft (kw));
        }
    }

    // ~30 fois par seconde (timer de l'éditeur)
    void tick()
    {
        auto& c = controls[(size_t) current];
        const bool free = state.getRawParameterValue (ModParams::lfoId (current, "sync"))->load() < 0.5f;
        c.rate->setAlpha (free ? 1.0f : 0.35f);
        display.repaint();
        for (auto* k : { c.rate.get(), c.phase.get(), c.delay.get(), c.fade.get(), c.smooth.get() })
            k->modulationTick();
    }

private:
    //--------------------------------------------------------------------------
    struct Tabs : juce::Component
    {
        std::function<void (int)> onChange;
        int current = 0;
        void paint (juce::Graphics& g) override
        {
            const float w = (float) getWidth() / 3.0f;
            for (int i = 0; i < 3; ++i)
            {
                auto r = juce::Rectangle<float> (w * (float) i, 0.0f, w, (float) getHeight()).reduced (1.5f);
                const auto col = octoui::ModHost::lfoColour (i);
                g.setColour (i == current ? col : OctoColours::panelHi.withAlpha (0.9f));
                g.fillRect (r);
                g.setColour (i == current ? col.brighter (0.4f) : col.withAlpha (0.8f));
                g.drawRect (r, 1.5f);
                g.setColour (i == current ? OctoColours::ink : OctoColours::cream);
                g.setFont (OctoLook::monoFont (12.0f));
                g.drawText (juce::String (i + 1), r, juce::Justification::centred);
            }
        }
        void mouseDown (const juce::MouseEvent& e) override
        {
            current = juce::jlimit (0, 2, (int) (e.position.x / ((float) getWidth() / 3.0f)));
            repaint();
            if (onChange) onChange (current);
        }
    };

    //--------------------------------------------------------------------------
    // Écran : forme d'onde, point qui court, poignée de glisser-déposer
    struct Display : juce::Component, public juce::SettableTooltipClient
    {
        LfoPanel* owner = nullptr;
        Display()
        {
            setTooltip (juce::String::fromUTF8 ("Attrape l'écran et lâche-le sur un potard pour le moduler. Clic sur ± : bipolaire / unipolaire."));
            setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        }
        static float shapeAt (int shape, float x)
        {
            static const float rnd[8] = { 0.3f, -0.8f, 0.9f, -0.2f, 0.6f, -0.6f, 0.1f, -0.95f };
            switch (shape)
            {
                case 1: return x < 0.5f ? 4.0f * x - 1.0f : 3.0f - 4.0f * x;
                case 2: return 2.0f * x - 1.0f;
                case 3: return 1.0f - 2.0f * x;
                case 4: return x < 0.5f ? 1.0f : -1.0f;
                case 5: return rnd[(int) (x * 8.0f) & 7];
                case 6: { const float p = x * 8.0f; const int i = (int) p; const float t = 0.5f - 0.5f * std::cos (3.14159265f * (p - (float) i));
                          return rnd[i & 7] + (rnd[(i + 1) & 7] - rnd[i & 7]) * t; }
                default: return std::sin (6.28318531f * x);
            }
        }
        juce::Rectangle<float> polArea() const { return getLocalBounds().toFloat().removeFromTop (16.0f).removeFromRight (22.0f); }
        void paint (juce::Graphics& g) override
        {
            const int i = owner->current;
            auto& st = owner->state;
            const int shape = (int) st.getRawParameterValue (ModParams::lfoId (i, "shape"))->load();
            const bool uni = st.getRawParameterValue (ModParams::lfoId (i, "pol"))->load() > 0.5f;
            const auto col = octoui::ModHost::lfoColour (i);
            auto r = getLocalBounds().toFloat();
            g.setColour (juce::Colour (0xff141216).withAlpha (0.85f));
            g.fillRect (r);
            g.setColour (col.withAlpha (0.7f));
            g.drawRect (r, 1.5f);
            auto plot = r.reduced (6.0f, 10.0f).withTrimmedBottom (6.0f);
            g.setColour (col.withAlpha (0.15f));
            g.drawHorizontalLine ((int) (uni ? plot.getBottom() : plot.getCentreY()), plot.getX(), plot.getRight());
            juce::Path p;
            auto yOf = [&] (float v) { return uni ? plot.getBottom() - (0.5f * (v + 1.0f)) * plot.getHeight()
                                                  : plot.getCentreY() - v * plot.getHeight() * 0.5f; };
            for (int k = 0; k <= 80; ++k)
            {
                const float x = (float) k / 80.0f;
                const juce::Point<float> pt (plot.getX() + x * plot.getWidth(), yOf (shapeAt (shape, x)));
                if (k == 0) p.startNewSubPath (pt); else p.lineTo (pt);
            }
            g.setColour (col.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (5.0f));
            g.setColour (col);                   g.strokePath (p, juce::PathStrokeType (1.6f));
            // point qui suit la sortie réelle du LFO
            const float ph = owner->lfoPhases[(size_t) i].load (std::memory_order_relaxed);
            const float v = owner->lfoValues[(size_t) i].load (std::memory_order_relaxed);
            const juce::Point<float> dot (plot.getX() + ph * plot.getWidth(), uni ? plot.getBottom() - v * plot.getHeight() : plot.getCentreY() - v * plot.getHeight() * 0.5f);
            g.setColour (juce::Colours::white); g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (dot));
            // libellés
            g.setFont (OctoLook::monoFont (10.0f));
            g.setColour (col);
            g.drawText (uni ? "+" : juce::String::fromUTF8 ("±"), polArea(), juce::Justification::centred);
            g.setColour (col.withAlpha (0.75f));
            g.drawText ("LFO " + juce::String (i + 1), r.removeFromTop (16.0f).reduced (5.0f, 0.0f), juce::Justification::centredLeft);
            g.drawText (juce::String::fromUTF8 ("glisser ▸ potard"), getLocalBounds().toFloat().removeFromBottom (13.0f), juce::Justification::centred);
        }
        void mouseDown (const juce::MouseEvent& e) override
        {
            dragStarted = false;
            if (polArea().contains (e.position))
            {
                if (auto* prm = owner->state.getParameter (ModParams::lfoId (owner->current, "pol")))
                {
                    prm->beginChangeGesture();
                    prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f);
                    prm->endChangeGesture();
                }
                repaint();
            }
        }
        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (dragStarted || e.getDistanceFromDragStart() < 4 || polArea().contains (e.mouseDownPosition)) return;
            if (auto* dc = juce::DragAndDropContainer::findParentDragContainerFor (this))
            {
                dragStarted = true;
                juce::Image img (juce::Image::ARGB, 64, 22, true);
                {
                    juce::Graphics g (img);
                    const auto col = octoui::ModHost::lfoColour (owner->current);
                    g.setColour (col); g.fillRoundedRectangle (0, 0, 64, 22, 4);
                    g.setColour (OctoColours::ink); g.setFont (OctoLook::monoFont (12.0f));
                    g.drawText ("LFO " + juce::String (owner->current + 1), 0, 0, 64, 22, juce::Justification::centred);
                }
                dc->startDragging ("octolfo:" + juce::String (owner->current), this, juce::ScaledImage (img), true);
            }
        }
        bool dragStarted = false;
    };

    void select (int i)
    {
        current = juce::jlimit (0, ModParams::numLfos - 1, i);
        tabs.current = current;
        for (int k = 0; k < ModParams::numLfos; ++k)
        {
            auto& c = controls[(size_t) k];
            for (juce::Component* comp : { (juce::Component*) &c.shape, (juce::Component*) &c.sync, (juce::Component*) &c.mode,
                                           (juce::Component*) c.rate.get(), (juce::Component*) c.phase.get(), (juce::Component*) c.delay.get(),
                                           (juce::Component*) c.fade.get(), (juce::Component*) c.smooth.get() })
                comp->setVisible (k == current);
        }
        tabs.repaint();
        display.repaint();
    }

    struct Controls
    {
        juce::ComboBox shape, sync, mode;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAtt, syncAtt, modeAtt;
        std::unique_ptr<octoui::Knob> rate, phase, delay, fade, smooth;
    };

    juce::AudioProcessorValueTreeState& state;
    Views& lfoValues;
    Views& lfoPhases;
    Tabs tabs;
    Display display;
    std::array<Controls, ModParams::numLfos> controls;
    int current = 0;
};

#pragma once

#include <JuceHeader.h>
#include "Palette.h"

namespace octoui
{
    //==========================================================================
    // Panneau « cartoon » : fond arrondi, ombre portée, titre en petites capitales
    //==========================================================================
    inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, bool accent = false)
    {
        using namespace OctoColours;
        g.setColour (ink.withAlpha (0.45f));
        g.fillRoundedRectangle (r.translated (0.0f, 3.0f), 12.0f);
        g.setColour (accent ? panel.interpolatedWith (slime, 0.10f) : panel);
        g.fillRoundedRectangle (r, 12.0f);
        g.setColour (ink);
        g.drawRoundedRectangle (r, 12.0f, 2.0f);
        if (title.isNotEmpty())
        {
            g.setColour (accent ? slime : cream.withAlpha (0.55f));
            g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
            g.drawText (title, r.reduced (12.0f, 5.0f).withHeight (14.0f), juce::Justification::centredLeft);
        }
    }

    //==========================================================================
    // Curseur rotatif : double-clic = défaut (posé par l'éditeur), Ctrl / Cmd +
    // glisser = réglage fin, molette prise en charge (JUCE).
    //==========================================================================
    class FineSlider : public juce::Slider
    {
    public:
        FineSlider() : juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox)
        {
            setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
            setMouseDragSensitivity (220);
        }
        void mouseDown (const juce::MouseEvent& e) override { updateSensitivity (e); juce::Slider::mouseDown (e); }
        void mouseDrag (const juce::MouseEvent& e) override { updateSensitivity (e); juce::Slider::mouseDrag (e); }

    private:
        void updateSensitivity (const juce::MouseEvent& e)
        {
            setMouseDragSensitivity (e.mods.isCommandDown() || e.mods.isCtrlDown() ? 1400 : 220);
        }
    };

    //==========================================================================
    // Potard étiqueté : affiche son nom, et sa valeur au survol / pendant le réglage
    //==========================================================================
    class Knob : public juce::Component, private juce::Slider::Listener
    {
    public:
        Knob (juce::AudioProcessorValueTreeState& state, const juce::ParameterID& id, const juce::String& name, bool bipolar = false)
            : title (name)
        {
            slider.getProperties().set ("bipolar", bipolar);
            slider.addListener (this);
            slider.addMouseListener (this, false);
            addAndMakeVisible (slider);
            attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, id.getParamID(), slider);
            param = state.getParameter (id.getParamID());
            if (param != nullptr)
            {
                slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
                slider.setTooltip (name + " — double-clic : valeur par défaut, Ctrl + glisser : réglage fin");
            }
        }

        ~Knob() override { slider.removeListener (this); }

        juce::Slider& getSlider() noexcept { return slider; }

        void resized() override
        {
            auto r = getLocalBounds();
            r.removeFromBottom (16);
            const int d = juce::jmin (r.getWidth(), r.getHeight());
            slider.setBounds (r.withSizeKeepingCentre (d, d));
        }

        void paint (juce::Graphics& g) override
        {
            using namespace OctoColours;
            auto r = getLocalBounds().removeFromBottom (16);
            const bool showValue = slider.isMouseOverOrDragging() || slider.isMouseButtonDown();
            juce::String text = title;
            if (showValue && param != nullptr)
            {
                text = param->getCurrentValueAsText();
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
                    if (rp->getLabel().isNotEmpty()) text << " " << rp->getLabel();
            }
            g.setColour (showValue ? slime : cream.withAlpha (0.85f));
            g.setFont (juce::FontOptions (juce::jmin (13.0f, (float) getWidth() * 0.16f + 3.0f), juce::Font::bold));
            g.drawFittedText (text, r, juce::Justification::centred, 1, 0.8f);
        }

        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit (const juce::MouseEvent&) override { repaint(); }
        void mouseUp (const juce::MouseEvent&) override { repaint(); }

    private:
        void sliderValueChanged (juce::Slider*) override { repaint(); }

        FineSlider slider;
        juce::String title;
        juce::RangedAudioParameter* param = nullptr;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    //==========================================================================
    // Sélecteur segmenté relié à un paramètre à choix (Saw | Square, etc.)
    //==========================================================================
    class Segmented : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        Segmented (juce::AudioProcessorValueTreeState& state, const juce::ParameterID& id,
                   juce::StringArray labels, const juce::String& name = {})
            : texts (std::move (labels)), title (name)
        {
            param = dynamic_cast<juce::RangedAudioParameter*> (state.getParameter (id.getParamID()));
            jassert (param != nullptr);
            attachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
            {
                index = (int) std::lround (v);
                repaint();
            });
            attachment->sendInitialUpdate();
        }

        void paint (juce::Graphics& g) override
        {
            using namespace OctoColours;
            auto r = getLocalBounds().toFloat().reduced (1.0f);
            if (title.isNotEmpty())
            {
                g.setColour (cream.withAlpha (0.85f));
                g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
                g.drawText (title, r.removeFromBottom (15.0f), juce::Justification::centred);
                r.removeFromBottom (2.0f);
            }
            const int n = texts.size();
            const float rad = juce::jmin (7.0f, r.getHeight() * 0.35f);
            g.setColour (ink.withAlpha (0.5f));
            g.fillRoundedRectangle (r.translated (0.0f, 1.5f), rad);
            g.setColour (panelHi);
            g.fillRoundedRectangle (r, rad);

            const float w = r.getWidth() / (float) n;
            for (int i = 0; i < n; ++i)
            {
                auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight());
                if (i == index)
                {
                    g.setColour (slime);
                    g.fillRoundedRectangle (seg.reduced (2.0f), rad - 1.0f);
                }
                else if (i == hover)
                {
                    g.setColour (cream.withAlpha (0.07f));
                    g.fillRoundedRectangle (seg.reduced (2.0f), rad - 1.0f);
                }
                g.setColour (i == index ? onSlime() : cream.withAlpha (0.8f));
                g.setFont (juce::FontOptions (juce::jmin (13.0f, r.getHeight() * 0.5f), juce::Font::bold));
                g.drawFittedText (texts[i], seg.toNearestInt().reduced (2, 0), juce::Justification::centred, 1, 0.7f);
            }
            g.setColour (ink);
            g.drawRoundedRectangle (r, rad, 1.5f);
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            const int i = segmentAt (e.x);
            if (i >= 0 && i != index)
                attachment->setValueAsCompleteGesture ((float) i);
        }
        void mouseMove (const juce::MouseEvent& e) override { const int h = segmentAt (e.x); if (h != hover) { hover = h; repaint(); } }
        void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }

    private:
        int segmentAt (int x) const
        {
            const int n = texts.size();
            if (n <= 0) return -1;
            return juce::jlimit (0, n - 1, x * n / juce::jmax (1, getWidth()));
        }

        juce::StringArray texts;
        juce::String title;
        juce::RangedAudioParameter* param = nullptr;
        std::unique_ptr<juce::ParameterAttachment> attachment;
        int index = 0, hover = -1;
    };

    //==========================================================================
    // Logo OctoCid : texte épais et une goutte de bave sous le « C »
    //==========================================================================
    inline void drawLogo (juce::Graphics& g, juce::Rectangle<float> r)
    {
        using namespace OctoColours;
        juce::GlyphArrangement ga;
        const juce::Font f (juce::FontOptions (r.getHeight() * 0.78f, juce::Font::bold));
        ga.addLineOfText (f, "OctoCid", r.getX(), r.getY() + r.getHeight() * 0.75f);
        juce::Path text;
        ga.createPath (text);
        g.setColour (background);
        g.strokePath (text, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (cream);
        g.fillPath (text);

        // goutte sous le « C » de « Cid »
        const float cx = r.getX() + ga.getBoundingBox (4, 1, true).getCentreX();
        const float top = r.getY() + r.getHeight() * 0.78f;
        juce::Path drip;
        drip.startNewSubPath (cx - 5.0f, top);
        drip.cubicTo (cx - 4.0f, top + 6.0f, cx - 5.0f, top + 9.0f, cx, top + 12.0f);
        drip.cubicTo (cx + 5.0f, top + 9.0f, cx + 4.0f, top + 6.0f, cx + 5.0f, top);
        drip.closeSubPath();
        g.setColour (slime);
        g.fillPath (drip);
        g.setColour (ink);
        g.strokePath (drip, juce::PathStrokeType (1.5f));
    }
}

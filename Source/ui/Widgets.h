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
        const auto& L = OctoLook::get();
        const auto fill = accent ? panel.interpolatedWith (slime, 0.12f) : panel;
        // Titres « à cheval » sur le bord : le corps descend un peu pour rester dans les limites
        const bool straddle = title.isNotEmpty() && (L.title == 1 || L.title == 3);
        auto inner = r.withTrimmedRight (L.shadow == 1 ? 3.0f : 0.0f).withTrimmedBottom (L.shadow == 1 ? 3.0f : 0.0f)
                      .withTrimmedTop (straddle ? 8.0f : 0.0f);
        OctoLook::shape (g, inner, L.panelR, fill.withMultipliedAlpha (OctoDecor::get().panelAlpha));

        // Vis aux coins (style rack)
        if (L.screws)
            for (auto pt : { inner.getTopLeft().translated (7.0f, 7.0f), inner.getTopRight().translated (-7.0f, 7.0f),
                             inner.getBottomLeft().translated (7.0f, -7.0f), inner.getBottomRight().translated (-7.0f, -7.0f) })
            {
                g.setColour (panelHi);
                g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (pt));
                g.setColour (ink.withAlpha (0.8f));
                g.drawEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (pt), 1.0f);
                g.drawLine ({ pt.translated (-2.2f, 1.2f), pt.translated (2.2f, -1.2f) }, 1.0f);
            }

        if (title.isEmpty())
            return;

        const juce::Font f (juce::FontOptions (12.0f, juce::Font::bold));
        const float tw = juce::GlyphArrangement::getStringWidth (f, title) + 14.0f;
        switch (L.title)
        {
            case 1:   // étiquette pleine, posée sur le bord
            {
                auto tag = juce::Rectangle<float> (inner.getX() + 12.0f, inner.getY() - 7.0f, tw, 17.0f);
                g.setColour (accent ? slime : ink);
                g.fillRoundedRectangle (tag, 3.0f);
                g.setColour (accent ? onSlime() : panel);
                g.setFont (f);
                g.drawText (title, tag, juce::Justification::centred);
                break;
            }
            case 2:   // gravé, souligné
            {
                g.setColour (accent ? slime.darker (0.2f) : ink.withAlpha (0.8f));
                g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
                auto t = juce::Rectangle<float> (inner.getX() + 18.0f, inner.getY() + 4.0f, tw, 14.0f);
                g.drawText (title, t, juce::Justification::centredLeft);
                g.drawLine (t.getX(), t.getBottom() + 1.0f, t.getX() + tw - 12.0f, t.getBottom() + 1.0f, 1.0f);
                break;
            }
            case 3:   // bulle de BD
            {
                auto tag = juce::Rectangle<float> (inner.getX() + 14.0f, inner.getY() - 8.0f, tw + 6.0f, 19.0f);
                g.setColour (ink);
                g.fillRoundedRectangle (tag.translated (2.0f, 2.0f), 9.5f);
                g.setColour (accent ? slime : juce::Colour (0xfff6efdf));
                g.fillRoundedRectangle (tag, 9.5f);
                g.setColour (ink);
                g.drawRoundedRectangle (tag, 9.5f, 2.0f);
                g.setColour (accent ? onSlime() : ink);
                g.setFont (f);
                g.drawText (title, tag, juce::Justification::centred);
                break;
            }
            default:
                g.setColour (accent ? slime : cream.withAlpha (0.55f));
                g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
                g.drawText (title, inner.reduced (12.0f, 5.0f).withHeight (14.0f), juce::Justification::centredLeft);
                break;
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
            const float rad = juce::jmin (OctoLook::get().ctrlR, r.getHeight() * 0.5f);
            r = r.withTrimmedRight (2.0f).withTrimmedBottom (2.0f);
            OctoLook::shape (g, r, rad, panelHi, 0.75f);

            const float w = r.getWidth() / (float) n;
            for (int i = 0; i < n; ++i)
            {
                auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight());
                if (i == index)
                {
                    g.setColour (slime);
                    g.fillRoundedRectangle (seg.reduced (2.0f), juce::jmax (1.0f, rad - 1.5f));
                }
                else if (i == hover)
                {
                    g.setColour (cream.withAlpha (0.07f));
                    g.fillRoundedRectangle (seg.reduced (2.0f), juce::jmax (1.0f, rad - 1.5f));
                }
                g.setColour (i == index ? onSlime() : cream.withAlpha (0.8f));
                g.setFont (juce::FontOptions (juce::jmin (13.0f, r.getHeight() * 0.5f), juce::Font::bold));
                g.drawFittedText (texts[i], seg.toNearestInt().reduced (2, 0), juce::Justification::centred, 1, 0.7f);
            }
            g.setColour (ink);
            g.drawRoundedRectangle (r, rad, OctoLook::get().outline * 0.75f);
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

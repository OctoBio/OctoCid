#pragma once

#include <JuceHeader.h>
#include "Palette.h"
#include "KnobStyles.h"

//==============================================================================
// Style OctoCid : sobre, cartoon, tout en vectoriel (juce::Path).
// Contours épais, une seule couleur d'accent pour les états actifs.
//==============================================================================
class OctoLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OctoLookAndFeel() { refreshColours(); }

    void refreshColours()
    {
        using namespace OctoColours;
        setColour (juce::ResizableWindow::backgroundColourId, background);
        setColour (juce::Label::textColourId, cream);
        setColour (juce::Slider::textBoxTextColourId, cream);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, slime.withAlpha (0.4f));
        setColour (juce::ComboBox::backgroundColourId, panelHi);
        setColour (juce::ComboBox::textColourId, cream);
        setColour (juce::ComboBox::outlineColourId, ink);
        setColour (juce::ComboBox::arrowColourId, slime);
        setColour (juce::PopupMenu::backgroundColourId, panel);
        setColour (juce::PopupMenu::textColourId, cream);
        setColour (juce::PopupMenu::headerTextColourId, slime);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, slime.withAlpha (0.35f));
        setColour (juce::PopupMenu::highlightedTextColourId, cream);
        setColour (juce::TextButton::buttonColourId, panelHi);
        setColour (juce::TextButton::buttonOnColourId, slime);
        setColour (juce::TextButton::textColourOffId, cream);
        setColour (juce::TextButton::textColourOnId, onSlime());
        setColour (juce::TooltipWindow::backgroundColourId, panel);
        setColour (juce::TooltipWindow::textColourId, cream);
        setColour (juce::TooltipWindow::outlineColourId, slime);
        setColour (juce::BubbleComponent::backgroundColourId, panel);
        setColour (juce::BubbleComponent::outlineColourId, slime);
        setColour (juce::TextEditor::backgroundColourId, panelHi);
        setColour (juce::TextEditor::textColourId, cream);
        setColour (juce::TextEditor::highlightColourId, slime.withAlpha (0.4f));
        setColour (juce::CaretComponent::caretColourId, slime);
        auto scheme = getCurrentColourScheme();
        scheme.setUIColour (ColourScheme::widgetBackground, panelHi);
        scheme.setUIColour (ColourScheme::windowBackground, panel);
        scheme.setUIColour (ColourScheme::outline, ink);
        scheme.setUIColour (ColourScheme::defaultText, cream);
        scheme.setUIColour (ColourScheme::highlightedFill, slime);
        scheme.setUIColour (ColourScheme::menuBackground, panel);
        scheme.setUIColour (ColourScheme::menuText, cream);
        setColourScheme (scheme);
    }

    //==========================================================================
    // Potard : piste d'arc, arc de valeur vert, capuchon cerclé d'encre, index crème
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        const bool bipolar = (bool) s.getProperties().getWithDefault ("bipolar", false);
        KnobStyles::chicken (OctoLook::get().knob, g, juce::Rectangle<int> (x, y, w, h).toFloat().reduced (2.0f),
                             pos, startAngle, endAngle, bipolar, s.isEnabled(), (int) (reinterpret_cast<juce::pointer_sized_int> (&s) & 0xffff));
    }

    //==========================================================================
    // Curseur linéaire (séquenceur) : piste arrondie, remplissage vert, bouton crème
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float, float,
                           juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        using namespace OctoColours;
        if (style != juce::Slider::LinearHorizontal)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, 0, 0, style, s);
            return;
        }
        const float cy = (float) y + (float) h * 0.5f;
        auto track = juce::Rectangle<float> ((float) x, cy - 3.0f, (float) w, 6.0f);
        g.setColour (ink.withAlpha (0.6f));
        g.fillRoundedRectangle (track.expanded (1.0f), 4.0f);
        g.setColour (dim.withAlpha (0.5f));
        g.fillRoundedRectangle (track, 3.0f);
        const bool bipolar = (bool) s.getProperties().getWithDefault ("bipolar", false);
        const float from = bipolar ? (float) x + (float) w * 0.5f : (float) x;
        g.setColour (slime);
        g.fillRoundedRectangle (juce::Rectangle<float> (juce::jmin (from, sliderPos), cy - 3.0f, std::abs (sliderPos - from), 6.0f), 3.0f);
        g.setColour (cream);
        g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ sliderPos, cy }));
        g.setColour (ink);
        g.drawEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ sliderPos, cy }), 1.5f);
    }

    //==========================================================================
    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& bg,
                               bool over, bool down) override
    {
        using namespace OctoColours;
        auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedRight (2.0f).withTrimmedBottom (2.0f);
        const float rad = juce::jmin (OctoLook::get().ctrlR, r.getHeight() * 0.5f);
        auto fill = bg;
        if (down) { fill = fill.darker (0.15f); r = r.translated (1.0f, 1.0f); }
        else if (over) fill = fill.brighter (0.08f);
        OctoLook::shape (g, r, rad, fill, 0.75f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int h) override
    {
        return juce::FontOptions (juce::jmin (14.0f, (float) h * 0.55f), juce::Font::bold);
    }

    // Interrupteur en pilule (SEQ ON, Escargot)
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool) override
    {
        using namespace OctoColours;
        auto r = b.getLocalBounds().toFloat().reduced (2.0f);
        const float th = juce::jmin (r.getHeight(), 22.0f);
        auto pill = juce::Rectangle<float> (r.getX(), r.getCentreY() - th * 0.5f, th * 1.9f, th);
        const bool on = b.getToggleState();
        g.setColour (ink.withAlpha (0.5f));
        g.fillRoundedRectangle (pill.translated (0.0f, 1.5f), th * 0.5f);
        g.setColour (on ? slime : (over ? panelHi.brighter (0.1f) : panelHi));
        g.fillRoundedRectangle (pill, th * 0.5f);
        g.setColour (ink);
        g.drawRoundedRectangle (pill, th * 0.5f, 1.5f);
        const float d = th - 6.0f;
        const float kx = on ? pill.getRight() - 3.0f - d : pill.getX() + 3.0f;
        g.setColour (on ? onSlime() : cream.withAlpha (0.8f));
        g.fillEllipse (kx, pill.getY() + 3.0f, d, d);
        g.setColour (cream);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (pill.getWidth() + 6.0f), juce::Justification::centredLeft);
    }

    //==========================================================================
    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        using namespace OctoColours;
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f).withTrimmedRight (2.0f).withTrimmedBottom (2.0f);
        const float rad = juce::jmin (OctoLook::get().ctrlR, r.getHeight() * 0.5f);
        OctoLook::shape (g, r, rad, box.isMouseOver (true) ? panelHi.brighter (0.06f) : panelHi, 0.75f);
        juce::Path arrow;
        const float ax = (float) w - 13.0f, ay = (float) h * 0.5f;
        arrow.startNewSubPath (ax - 4.0f, ay - 2.0f);
        arrow.lineTo (ax, ay + 2.5f);
        arrow.lineTo (ax + 4.0f, ay - 2.0f);
        g.setColour (slime);
        g.strokePath (arrow, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::Font getComboBoxFont (juce::ComboBox& b) override
    {
        return juce::FontOptions (juce::jmin (14.0f, (float) b.getHeight() * 0.55f), juce::Font::bold);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (4, 1, box.getWidth() - 22, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    juce::Font getPopupMenuFont() override { return juce::FontOptions (14.0f); }

    juce::Font getLabelFont (juce::Label& l) override
    {
        return l.getFont();
    }
};

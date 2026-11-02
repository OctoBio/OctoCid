#pragma once

#include <JuceHeader.h>
#include "Palette.h"

namespace octoui
{
    //==========================================================================
    // Panneau « cartoon » : fond arrondi, ombre portée, titre en petites capitales
    //==========================================================================
    inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, bool accent = false,
                           juce::Point<float> origin = {}, float alphaOverride = -1.0f, bool strongFrost = false)
    {
        using namespace OctoColours;
        const auto& L = OctoLook::get();
        const auto fill = accent ? panel.interpolatedWith (slime, 0.12f) : panel;
        // Titres « à cheval » sur le bord : le corps descend un peu pour rester dans les limites
        const bool straddle = title.isNotEmpty() && (L.title == 1 || L.title == 3);
        auto inner = r.withTrimmedRight (L.shadow == 1 ? 3.0f : 0.0f).withTrimmedBottom (L.shadow == 1 ? 3.0f : 0.0f)
                      .withTrimmedTop (straddle ? 8.0f : 0.0f);
        const float alpha = alphaOverride >= 0.0f ? alphaOverride : (L.alpha >= 0.0f ? L.alpha : OctoDecor::get().panelAlpha);
        OctoLook::shape (g, inner, L.panelR, fill.withMultipliedAlpha (alpha), 1.0f, &origin, strongFrost);

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
            case 4:   // épuré : petites capitales espacées + filet fin jusqu'au bord
            {
                const juce::Font tf (juce::FontOptions (10.5f, juce::Font::bold));
                juce::String spaced; for (auto ch : title) { spaced << juce::String::charToString (ch) << " "; }
                spaced = spaced.trimEnd();
                const float w = juce::GlyphArrangement::getStringWidth (tf, spaced);
                auto t = juce::Rectangle<float> (inner.getX() + 12.0f, inner.getY() + 5.0f, w + 4.0f, 13.0f);
                g.setColour (accent ? slime.darker (0.3f) : ink.withAlpha (0.85f));
                g.setFont (tf);
                g.drawText (spaced, t, juce::Justification::centredLeft);
                g.setColour ((L.lightOutline ? juce::Colour (0xfffff8ec) : ink).withAlpha (0.35f));
                g.drawLine (t.getRight() + 6.0f, t.getCentreY(), inner.getRight() - 12.0f, t.getCentreY(), 1.0f);
                break;
            }
            case 5:   // pastille compacte à l'intérieur
            {
                const juce::Font tf (juce::FontOptions (10.5f, juce::Font::bold));
                const float w = juce::GlyphArrangement::getStringWidth (tf, title) + 14.0f;
                auto t = juce::Rectangle<float> (inner.getX() + 10.0f, inner.getY() + 5.0f, w, 15.0f);
                g.setColour (accent ? slime : ink.withAlpha (0.85f));
                g.fillRoundedRectangle (t, 7.5f);
                g.setColour (accent ? onSlime() : juce::Colour (0xfff6efdf));
                g.setFont (tf);
                g.drawText (title, t, juce::Justification::centred);
                break;
            }
            case 6:   // glitch épuré : « // TITRE » + filet
            case 7:   // glitch : bloc plein avec décalage cyan / magenta
            case 8:   // terminal : « > TITRE_ »
            {
                const juce::Font mono (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.5f, juce::Font::bold));
                const juce::String txt = L.title == 6 ? "// " + title : (L.title == 8 ? "> " + title + "_" : title);
                const float w = juce::GlyphArrangement::getStringWidth (mono, txt) + (L.title == 6 ? 4.0f : 14.0f);
                auto t = juce::Rectangle<float> (inner.getX() + 10.0f, inner.getY() + 5.0f, w, 15.0f);
                g.setFont (mono);
                if (L.title == 7)
                {
                    g.setColour (OctoLook::glitchCyan);    g.fillRect (t.translated (-2.0f, 0.0f));
                    g.setColour (OctoLook::glitchMagenta); g.fillRect (t.translated (2.0f, 0.0f));
                    g.setColour (accent ? slime : ink);    g.fillRect (t);
                    g.setColour (accent ? onSlime() : slime);
                    g.drawText (txt, t, juce::Justification::centred);
                }
                else if (L.title == 8)
                {
                    g.setColour (ink.withAlpha (0.85f)); g.fillRect (t);
                    g.setColour (slime.brighter (0.3f));
                    g.drawText (txt, t.reduced (6.0f, 0.0f), juce::Justification::centredLeft);
                    g.setColour (slime.withAlpha (0.6f));
                    g.drawLine (t.getRight() + 6.0f, t.getCentreY(), inner.getRight() - 12.0f, t.getCentreY(), 1.0f);
                }
                else
                {
                    g.setColour (OctoLook::glitchCyan.withAlpha (0.8f));    g.drawText (txt, t.translated (-1.2f, 0.0f), juce::Justification::centredLeft);
                    g.setColour (OctoLook::glitchMagenta.withAlpha (0.8f)); g.drawText (txt, t.translated (1.2f, 0.0f), juce::Justification::centredLeft);
                    g.setColour (accent ? slime.darker (0.3f) : ink);       g.drawText (txt, t, juce::Justification::centredLeft);
                    g.setColour ((L.lightOutline ? juce::Colour (0xfffff8ec) : ink).withAlpha (0.4f));
                    g.drawLine (t.getRight() + 6.0f, t.getCentreY(), inner.getRight() - 12.0f, t.getCentreY(), 1.0f);
                }
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
            setPopupMenuEnabled (false);
        }
        std::function<void (const juce::MouseEvent&)> onPopup;   // clic droit
        std::function<void (float)> onAltDrag;                    // Alt + glisser : quantité de modulation (en %)

        void mouseDown (const juce::MouseEvent& e) override
        {
            altMode = false;
            if (e.mods.isPopupMenu()) { if (onPopup) onPopup (e); return; }
            if (e.mods.isAltDown() && onAltDrag) { altMode = true; altY = e.position.y; return; }
            updateSensitivity (e); juce::Slider::mouseDown (e);
        }
        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (e.mods.isPopupMenu()) return;
            if (altMode) { onAltDrag ((altY - e.position.y) * (e.mods.isCtrlDown() ? 0.05f : 0.5f)); altY = e.position.y; return; }
            updateSensitivity (e); juce::Slider::mouseDrag (e);
        }
        void mouseUp (const juce::MouseEvent& e) override
        {
            if (altMode || e.mods.isPopupMenu()) { altMode = false; return; }
            juce::Slider::mouseUp (e);
        }

    private:
        void updateSensitivity (const juce::MouseEvent& e)
        {
            setMouseDragSensitivity (e.mods.isCommandDown() || e.mods.isCtrlDown() ? 1400 : 220);
        }
        bool altMode = false;
        float altY = 0.0f;
    };

    //==========================================================================
    // Hôte de modulation (l'éditeur) : les potards lui demandent leurs modulations
    //==========================================================================
    class ModHost
    {
    public:
        virtual ~ModHost() = default;
        struct Mod { int slot = 0, lfo = 0; float amount = 0.0f; bool bipolar = true; };   // amount : -1..1 (part de la course du potard)
        virtual int modDestFor (const juce::String& paramId) = 0;      // 0 : pas modulable
        virtual void modsFor (int dest, juce::Array<Mod>& out) = 0;
        virtual float modOffsetNow (int dest) = 0;                     // décalage actuel (-1..1)
        virtual void modAssign (int lfo, int dest) = 0;
        virtual void modRemove (int dest) = 0;
        virtual void modSetAmount (int slot, float percent) = 0;
        virtual void modNudge (int dest, float deltaPercent) = 0;

        static juce::Colour lfoColour (int lfo)
        {
            static const juce::Colour c[3] = { juce::Colour (0xff8ef01e), juce::Colour (0xff00e8ff), juce::Colour (0xffff2ac8) };
            return c[juce::jlimit (0, 2, lfo)];
        }
    };

    //==========================================================================
    // Potard étiqueté : affiche son nom, et sa valeur au survol / pendant le réglage
    //==========================================================================
    class Knob : public juce::Component, public juce::DragAndDropTarget, private juce::Slider::Listener
    {
    public:
        Knob (juce::AudioProcessorValueTreeState& state, const juce::ParameterID& id, const juce::String& name, bool bipolar = false)
            : title (name), paramId (id.getParamID())
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
                slider.setTooltip (name + juce::String::fromUTF8 (" — double-clic : valeur par défaut, Ctrl + glisser : réglage fin, clic droit : modulation"));
            }
            slider.onPopup = [this] (const juce::MouseEvent&) { showModMenu(); };
            slider.onAltDrag = [this] (float d) { if (auto* h = host()) if (dest() > 0) { h->modNudge (dest(), d); repaint(); } };
        }

        ~Knob() override { slider.removeListener (this); }

        juce::Slider& getSlider() noexcept { return slider; }
        const juce::String& getParamId() const noexcept { return paramId; }

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

        // Modulation : un arc par LFO (plage balayée) et un point qui suit la valeur modulée
        void paintOverChildren (juce::Graphics& g) override
        {
            if (dropHover)
            {
                g.setColour (OctoColours::slime.withAlpha (0.35f));
                g.fillEllipse (slider.getBounds().toFloat().reduced (2.0f));
            }
            auto* h = host();
            if (h == nullptr || dest() <= 0 || param == nullptr) return;
            juce::Array<ModHost::Mod> mods;
            h->modsFor (dest(), mods);
            if (mods.isEmpty()) return;
            const auto rp = slider.getRotaryParameters();
            const auto b = slider.getBounds().toFloat().reduced (2.0f);
            const auto c = b.getCentre();
            const float base = param->getValue();
            auto angle = [&] (float v) { return rp.startAngleRadians + juce::jlimit (0.0f, 1.0f, v) * (rp.endAngleRadians - rp.startAngleRadians); };
            float rad = b.getWidth() * 0.5f - 1.0f;
            for (auto& m : mods)
            {
                const float a0 = angle (m.bipolar ? base - m.amount : base), a1 = angle (base + m.amount);
                if (std::abs (a1 - a0) > 0.001f)
                {
                    juce::Path arc;
                    arc.addCentredArc (c.x, c.y, rad, rad, 0.0f, std::min (a0, a1), std::max (a0, a1), true);
                    g.setColour (ModHost::lfoColour (m.lfo).withAlpha (0.9f));
                    g.strokePath (arc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                }
                rad -= 3.5f;
            }
            const float now = angle (base + h->modOffsetNow (dest()));
            const auto pt = c.getPointOnCircumference (b.getWidth() * 0.5f - 1.0f, now);
            g.setColour (juce::Colours::white);
            g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (pt));
            g.setColour (ModHost::lfoColour (mods.getFirst().lfo));
            g.drawEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (pt), 1.5f);
        }

        // appelé par l'éditeur ~30 fois par seconde : ne redessine que s'il y a une modulation
        void modulationTick()
        {
            if (auto* h = host())
                if (dest() > 0)
                {
                    juce::Array<ModHost::Mod> mods;
                    h->modsFor (dest(), mods);
                    if (! mods.isEmpty() || wasModulated) repaint();
                    wasModulated = ! mods.isEmpty();
                }
        }

        // Glisser-déposer d'un LFO
        bool isInterestedInDragSource (const SourceDetails& d) override
        { return d.description.toString().startsWith ("octolfo:") && dest() > 0; }
        void itemDragEnter (const SourceDetails&) override { dropHover = true; repaint(); }
        void itemDragExit (const SourceDetails&) override { dropHover = false; repaint(); }
        void itemDropped (const SourceDetails& d) override
        {
            dropHover = false;
            if (auto* h = host())
                h->modAssign (d.description.toString().fromFirstOccurrenceOf (":", false, false).getIntValue(), dest());
            repaint();
        }

        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit (const juce::MouseEvent&) override { repaint(); }
        void mouseUp (const juce::MouseEvent&) override { repaint(); }

    private:
        ModHost* host() const
        {
            for (auto* c = getParentComponent(); c != nullptr; c = c->getParentComponent())
                if (auto* h = dynamic_cast<ModHost*> (c)) return h;
            return nullptr;
        }
        int dest()
        {
            if (destIndex < 0) { auto* h = host(); if (h == nullptr) return 0; destIndex = h->modDestFor (paramId); }
            return destIndex;
        }
        void showModMenu()
        {
            auto* h = host();
            if (h == nullptr || dest() <= 0) return;
            juce::Array<ModHost::Mod> mods;
            h->modsFor (dest(), mods);
            juce::PopupMenu m;
            m.addSectionHeader (title + juce::String::fromUTF8 (" — modulation"));
            for (int i = 0; i < 3; ++i)
            {
                bool has = false;
                for (auto& md : mods) has = has || md.lfo == i;
                m.addColouredItem (1 + i, "Moduler par LFO " + juce::String (i + 1), ModHost::lfoColour (i), true, has);
            }
            for (auto& md : mods)
            {
                juce::PopupMenu amt;
                for (int v : { 100, 50, 25, 10, 5, -5, -10, -25, -50, -100 })
                    amt.addItem (100 + md.slot * 64 + (v + 100) / 5, (v > 0 ? "+" : "") + juce::String (v) + " %", true,
                                 std::abs (md.amount * 100.0f - (float) v) < 0.5f);
                m.addSubMenu (juce::String::fromUTF8 ("Quantité LFO ") + juce::String (md.lfo + 1) + " (" + juce::String (md.amount * 100.0f, 1) + " %)", amt);
            }
            if (! mods.isEmpty()) m.addItem (99, "Retirer la modulation");
            m.addSeparator();
            m.addItem (98, juce::String::fromUTF8 ("Astuce : glisser un LFO sur le potard, Alt + glisser règle la quantité"), false);
            juce::Component::SafePointer<Knob> safe (this);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&slider), [safe] (int r)
            {
                if (safe == nullptr || r <= 0) return;
                auto* hh = safe->host();
                if (hh == nullptr) return;
                if (r >= 1 && r <= 3) hh->modAssign (r - 1, safe->dest());
                else if (r == 99) hh->modRemove (safe->dest());
                else if (r >= 100) { const int slot = (r - 100) / 64, step = (r - 100) % 64; hh->modSetAmount (slot, (float) (step * 5 - 100)); }
                safe->repaint();
            });
        }
        void sliderValueChanged (juce::Slider*) override { repaint(); }

        FineSlider slider;
        juce::String title, paramId;
        int destIndex = -1;
        bool dropHover = false, wasModulated = false;
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
            const bool gl = OctoLook::get().glitchControls && ! OctoLook::isPlain (this);
            for (int i = 0; i < n; ++i)
            {
                auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight());
                if (gl)
                {
                    auto sr = seg.reduced (2.5f, 2.5f);
                    if (i == index)
                    {
                        g.setColour (OctoLook::glitchCyan);    g.fillRect (sr.translated (-2.0f, 0.0f));
                        g.setColour (OctoLook::glitchMagenta); g.fillRect (sr.translated (2.0f, 0.0f));
                        g.setColour (slime);                   g.fillRect (sr);
                    }
                    else if (i == hover)
                    {
                        g.setColour (cream.withAlpha (0.08f)); g.fillRect (sr);
                    }
                    if (i > 0)
                    {
                        g.setColour (ink.withAlpha (0.25f));
                        g.drawVerticalLine ((int) seg.getX(), seg.getY() + 4.0f, seg.getBottom() - 4.0f);
                    }
                    g.setColour (i == index ? ink : cream.withAlpha (0.8f));
                    g.setFont (OctoLook::monoFont (juce::jmin (12.0f, r.getHeight() * 0.5f)));
                    g.drawFittedText (texts[i].toUpperCase(), seg.toNearestInt().reduced (2, 0), juce::Justification::centred, 1, 0.6f);
                    continue;
                }
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
            if (! gl)
            {
                g.setColour (ink);
                g.drawRoundedRectangle (r, rad, OctoLook::get().outline * 0.75f);
            }
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
    inline bool showLogo = true;   // (captures de test)
    inline juce::Image logoImage;  // logo illustré (intégré au plugin)
    inline void drawLogo (juce::Graphics& g, juce::Rectangle<float> r)
    {
        if (! showLogo) return;
        if (logoImage.isValid())
        {
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImage (logoImage, r, juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yMid | juce::RectanglePlacement::onlyReduceInSize);
            return;
        }
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

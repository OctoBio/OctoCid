#pragma once

#include <JuceHeader.h>
#include "Palette.h"

//==============================================================================
// Zone mascotte : l'escargot (version fixe ; il s'animera à l'étape 6).
// Escargot cartoon original : gros yeux au bout des pédoncules, coquille en
// spirale, traînée de bave. Tout en vectoriel.
//==============================================================================
class MascotView : public juce::Component
{
public:
    // Couleurs propres à l'escargot : il garde son allure dans toutes les palettes
    static inline const juce::Colour kBody  { 0xffdfe6b8 };
    static inline const juce::Colour kShell { 0xff6b5a44 };

    void paint (juce::Graphics& g) override
    {
        using namespace OctoColours;
        auto r = getLocalBounds().toFloat();

        // Fond de la zone : panneau en creux
        g.setColour (ink.withAlpha (0.45f));
        g.fillRoundedRectangle (r.reduced (1.0f).translated (0.0f, 3.0f), 14.0f);
        g.setColour (background.interpolatedWith (panel, 0.5f));
        g.fillRoundedRectangle (r.reduced (1.0f), 14.0f);
        g.setColour (ink);
        g.drawRoundedRectangle (r.reduced (1.0f), 14.0f, 2.0f);

        const float s = juce::jmin (r.getWidth() / 200.0f, r.getHeight() / 140.0f);
        const auto o = r.getCentre().translated (-100.0f * s, -70.0f * s);
        auto P = [&] (float x, float y) { return juce::Point<float> (o.x + x * s, o.y + y * s); };
        const float stroke = 2.4f * s;

        // Sol + traînée de bave
        juce::Path trail;
        trail.addRoundedRectangle (P (14, 118).x, P (14, 118).y, 90 * s, 7 * s, 3.5f * s);
        g.setColour (slime.withAlpha (0.55f));
        g.fillPath (trail);

        // Corps (pied allongé) avec la tête à droite
        juce::Path body;
        body.startNewSubPath (P (60, 120));
        body.cubicTo (P (40, 120), P (38, 104), P (60, 102));
        body.lineTo (P (128, 100));
        body.cubicTo (P (140, 100), P (144, 86), P (146, 70));       // cou
        body.cubicTo (P (148, 56), P (176, 54), P (178, 74));        // tête
        body.cubicTo (P (180, 92), P (176, 112), P (160, 118));
        body.cubicTo (P (140, 122), P (100, 121), P (60, 120));
        body.closeSubPath();
        g.setColour (kBody);
        g.fillPath (body);
        g.setColour (ink);
        g.strokePath (body, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Coquille en spirale
        const auto sc = P (88, 74);
        const float shellR = 36.0f * s;
        g.setColour (kShell.interpolatedWith (slime, 0.25f));
        g.fillEllipse (juce::Rectangle<float> (shellR * 2.0f, shellR * 2.0f).withCentre (sc));
        juce::Path spiral;
        const int segs = 90;
        for (int i = 0; i <= segs; ++i)
        {
            const float t = (float) i / (float) segs;
            const float a = t * juce::MathConstants<float>::twoPi * 2.6f;
            const float rr = shellR * (0.92f - 0.85f * t);
            const auto pt = sc.translated (std::cos (a) * rr, std::sin (a) * rr);
            if (i == 0) spiral.startNewSubPath (pt); else spiral.lineTo (pt);
        }
        g.setColour (ink);
        g.strokePath (spiral, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawEllipse (juce::Rectangle<float> (shellR * 2.0f, shellR * 2.0f).withCentre (sc), stroke);

        // Pédoncules + yeux
        auto eye = [&] (juce::Point<float> base, juce::Point<float> tip, float er)
        {
            g.setColour (ink);
            g.drawLine ({ base, tip }, stroke * 1.4f);
            g.setColour (kBody);
            g.drawLine ({ base, tip }, stroke * 0.6f);
            g.setColour (juce::Colour (0xfffbf8ef));
            g.fillEllipse (juce::Rectangle<float> (er * 2.0f, er * 2.0f).withCentre (tip));
            g.setColour (ink);
            g.drawEllipse (juce::Rectangle<float> (er * 2.0f, er * 2.0f).withCentre (tip), stroke);
            g.fillEllipse (juce::Rectangle<float> (er * 0.9f, er * 0.9f).withCentre (tip.translated (er * 0.25f, er * 0.1f)));
        };
        eye (P (160, 60), P (154, 24), 11.0f * s);
        eye (P (170, 62), P (182, 30), 9.5f * s);

        // Sourire
        juce::Path smile;
        smile.startNewSubPath (P (160, 88));
        smile.quadraticTo (P (168, 95), P (175, 86));
        g.setColour (ink);
        g.strokePath (smile, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
};

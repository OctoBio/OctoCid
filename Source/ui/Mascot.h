#pragma once

#include <JuceHeader.h>
#include "Palette.h"
#include "Snail.h"

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
        const auto org = getPosition().toFloat();
        OctoLook::shape (g, r.reduced (1.0f).withTrimmedRight (2.0f).withTrimmedBottom (2.0f),
                         OctoLook::get().panelR, background.interpolatedWith (panel, 0.5f).withMultipliedAlpha (OctoDecor::get().panelAlpha),
                         1.0f, &org);

        OctoSnail::draw (g, r.reduced (6.0f, 4.0f));
    }
};

#pragma once

#include <JuceHeader.h>
#include "Palette.h"
#include "Snail.h"

//==============================================================================
// Zone mascotte : l'escargot (version fixe ; il s'animera à l'étape 6).
// Escargot cartoon original : gros yeux au bout des pédoncules, coquille en
// spirale, traînée de bave. Tout en vectoriel.
//==============================================================================
class OctoCidAudioProcessor;

class MascotView : public juce::Component, private juce::Timer
{
public:
    // Accès aux valeurs, sans dépendance au processeur (atomiques fournies par l'éditeur)
    struct Sources
    {
        juce::RangedAudioParameter *cutoff = nullptr, *resonance = nullptr, *envmod = nullptr, *decay = nullptr,
                                   *drive = nullptr, *driveType = nullptr, *tune = nullptr, *lfoDepth = nullptr, *anim = nullptr;
        std::atomic<int>*   noteCount = nullptr;
        std::atomic<bool>*  lastAccent = nullptr;
        std::atomic<bool>*  lastSlide = nullptr;
        std::atomic<int>*   lastNote = nullptr;
        std::atomic<float>* lfo = nullptr;
    };

    MascotView() { setInterceptsMouseClicks (false, false); }
    ~MascotView() override { stopTimer(); }

    void setSources (const Sources& src)
    {
        sources = src;
        if (sources.noteCount != nullptr) lastCount = sources.noteCount->load();
        startTimerHz (30);
    }

    // Zone de dessin de l'escargot (dans le composant, qui déborde un peu pour les sauts)
    void setSnailArea (juce::Rectangle<float> r) { snailArea = r; staticCache = {}; repaint(); }

    void paint (juce::Graphics& g) override
    {
        const auto& st = OctoSnail::styles[OctoSnail::mascotStyle];
        auto area = snailArea.isEmpty() ? getLocalBounds().toFloat() : snailArea;

        OctoSnail::draw (g, area, st, anim, OctoSnail::kUnder);

        // Corps et coquille : rendus une fois dans une image (à la résolution réelle), puis déplacés
        const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
        const auto wantW = juce::roundToInt (getWidth() * scale), wantH = juce::roundToInt (getHeight() * scale);
        if (! staticCache.isValid() || staticCache.getWidth() != wantW || staticCache.getHeight() != wantH)
        {
            staticCache = juce::Image (juce::Image::ARGB, juce::jmax (1, wantW), juce::jmax (1, wantH), true);
            juce::Graphics gc (staticCache);
            gc.addTransform (juce::AffineTransform::scale (scale));
            OctoSnail::draw (gc, area, st, {}, OctoSnail::kStatic);
        }
        const float s = juce::jmin (area.getWidth() / 256.0f, area.getHeight() / 186.0f);
        g.setOpacity (1.0f);   // drawImage utilise l'opacité courante
        g.drawImageTransformed (staticCache, juce::AffineTransform::scale (1.0f / scale)
                                               .translated (anim.slideX * s, anim.jumpY * s));

        OctoSnail::draw (g, area, st, anim, OctoSnail::kOver);
    }

private:
    static float norm (juce::RangedAudioParameter* p, float fallback) { return p != nullptr ? p->getValue() : fallback; }

    void timerCallback() override
    {
        const float dt = 1.0f / 30.0f;
        const bool animOn = sources.anim == nullptr || sources.anim->getValue() > 0.5f;
        if (! animOn)
        {
            if (! paused) { anim = {}; paused = true; repaint(); }
            return;
        }
        paused = false;
        t += dt;
        anim.time = t;
        ++anim.frame;

        auto smooth = [dt] (float& v, float target, float tau) { v += (target - v) * (1.0f - std::exp (-dt / tau)); };

        // Réglages -> attitude
        smooth (anim.pupil,  norm (sources.cutoff, 0.5f), 0.12f);
        const float reso = norm (sources.resonance, 0.0f);
        anim.jitter = reso * reso * 1.8f;
        smooth (anim.slime, 0.55f + 1.0f * norm (sources.envmod, 0.5f), 0.2f);
        smooth (anim.drool, 0.6f + 1.3f * norm (sources.decay, 0.3f), 0.2f);
        const int dtype = sources.driveType != nullptr ? juce::roundToInt (sources.driveType->convertFrom0to1 (sources.driveType->getValue())) : 0;
        const float heatTarget = dtype == 0 ? 0.0f : norm (sources.drive, 0.0f) * (dtype == 2 ? 1.0f : 0.65f);
        smooth (anim.heat, heatTarget, 0.25f);

        // Pédoncules : LFO (ondulent en sens opposés) + Tune (penchent)
        const float lfo = sources.lfo != nullptr ? sources.lfo->load() : 0.0f;
        const float depth = norm (sources.lfoDepth, 0.0f);
        const float tuneTilt = (norm (sources.tune, 0.5f) - 0.5f) * 0.6f;
        smooth (anim.stalkL, tuneTilt + lfo * depth * 0.30f, 0.05f);
        smooth (anim.stalkR, tuneTilt - lfo * depth * 0.30f, 0.05f);

        // Notes : saut sur accent, petit hochement sinon, glissade sur slide
        const int count = sources.noteCount != nullptr ? sources.noteCount->load() : lastCount;
        if (count != lastCount)
        {
            lastCount = count;
            idle = 0.0f;
            const bool acc = sources.lastAccent != nullptr && sources.lastAccent->load();
            const bool sl  = sources.lastSlide  != nullptr && sources.lastSlide->load();
            if (sl)  { slideKick = 1.0f; }
            else     { jumpT = 0.0f; jumpAmp = acc ? 12.0f : 2.5f; }
            if (acc) { anim.gazeY = -0.15f; }
        }
        if (jumpT >= 0.0f)
        {
            jumpT += dt;
            const float T = jumpAmp > 5.0f ? 0.30f : 0.14f;
            anim.jumpY = jumpT < T ? -jumpAmp * std::sin (juce::MathConstants<float>::pi * jumpT / T) : 0.0f;
            if (jumpT >= T) jumpT = -1.0f;
        }
        slideKick *= std::exp (-dt / 0.45f);
        anim.slideX = 6.0f * slideKick;
        anim.trailGlow = slideKick;
        smooth (anim.gazeY, -0.1f * (norm (sources.cutoff, 0.5f) - 0.5f), 0.3f);
        anim.gazeX = 0.12f * slideKick;

        // Clignements et sommeil
        const bool playing = sources.lastNote != nullptr && sources.lastNote->load() >= 0;
        idle = playing ? 0.0f : idle + dt;
        smooth (anim.sleep, idle > 20.0f ? 1.0f : 0.0f, idle > 20.0f ? 1.5f : 0.15f);
        blinkTimer -= dt;
        if (blinkTimer <= 0.0f) { blinkT = 0.0f; blinkTimer = 2.5f + rnd.nextFloat() * 4.0f; }
        if (blinkT >= 0.0f)
        {
            blinkT += dt;
            anim.blink = blinkT < 0.16f ? std::sin (juce::MathConstants<float>::pi * blinkT / 0.16f) : 0.0f;
            if (blinkT >= 0.16f) blinkT = -1.0f;
        }

        repaint();
    }

    Sources sources;
    OctoSnail::Anim anim;
    juce::Image staticCache;
    juce::Rectangle<float> snailArea;
    juce::Random rnd { 42 };
    float t = 0.0f, idle = 0.0f, jumpT = -1.0f, jumpAmp = 0.0f, slideKick = 0.0f, blinkTimer = 2.0f, blinkT = -1.0f;
    int lastCount = 0;
    bool paused = false;
};

#pragma once

#include <JuceHeader.h>
#include "Palette.h"

//==============================================================================
// Styles de potards proposés (tout en juce::Path, aucune image).
//  0. Linogravure  : imprimé à l'encre, décalage d'impression vert, graduations
//  1. Bakélite 303 : jupe noire crantée, capuchon, trait d'index crème
//  2. Tête de poulet : pointeur vintage allongé sur graduations
//  3. Bauhaus      : disque plat, point d'index, fine couronne de valeur
//  4. BD           : contour dessiné à la main, ombre en trame de points
//  5. Cannelé      : bouton cannelé façon studio, insert métal
//==============================================================================
namespace KnobStyles
{
    inline constexpr int count = 6;
    inline const char* names[count] = { "Linogravure", "Bakelite 303", "Tete de poulet", "Bauhaus", "BD (dessin main)", "Cannele studio" };

    // Graduations autour du potard (traits radiaux)
    inline void drawTicks (juce::Graphics& g, juce::Point<float> c, float r0, float r1, float start, float end,
                           int n, juce::Colour col, float thick, bool majorEvery5 = true)
    {
        g.setColour (col);
        for (int i = 0; i < n; ++i)
        {
            const float t = (float) i / (float) (n - 1);
            const float a = start + t * (end - start);
            const bool major = majorEvery5 && (i % 5 == 0);
            const float rr1 = major ? r1 + (r1 - r0) * 0.6f : r1;
            g.drawLine ({ c.getPointOnCircumference (r0, a), c.getPointOnCircumference (rr1, a) }, major ? thick * 1.3f : thick);
        }
    }

    // Cercle « dessiné à la main » : rayon légèrement irrégulier, reproductible
    inline juce::Path wobblyCircle (juce::Point<float> c, float r, float amount, int seed)
    {
        juce::Path p;
        const int n = 64;
        juce::Random rnd (seed);
        float ph1 = rnd.nextFloat() * 6.28f, ph2 = rnd.nextFloat() * 6.28f;
        for (int i = 0; i <= n; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / (float) n;
            const float rr = r * (1.0f + amount * (0.6f * std::sin (3.0f * a + ph1) + 0.4f * std::sin (5.0f * a + ph2)));
            const auto pt = c.getPointOnCircumference (rr, a);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        p.closeSubPath();
        return p;
    }

    //==========================================================================
    inline void draw (int style, juce::Graphics& g, juce::Rectangle<float> bounds, float pos,
                      float start, float end, bool bipolar, bool enabled, int seed = 1)
    {
        using namespace OctoColours;
        const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const float a = start + pos * (end - start);
        const auto paper = juce::Colour (0xfff6efdf);
        const auto inkC = enabled ? ink : dim;
        const float lw = juce::jmax (1.5f, r * 0.07f);

        switch (style)
        {
            //------------------------------------------------------------------
            case 0:   // Linogravure
            {
                drawTicks (g, c, r * 0.80f, r * 0.92f, start, end, 11, inkC.withAlpha (0.85f), lw * 0.55f);
                const float kr = r * 0.66f;
                // décalage d'impression (aplat vert décalé)
                g.setColour (slime.withAlpha (enabled ? 0.9f : 0.4f));
                g.fillEllipse (juce::Rectangle<float> (kr * 2, kr * 2).withCentre (c.translated (r * 0.11f, r * 0.12f)));
                g.setColour (paper);
                g.fillEllipse (juce::Rectangle<float> (kr * 2, kr * 2).withCentre (c));
                g.setColour (inkC);
                g.drawEllipse (juce::Rectangle<float> (kr * 2, kr * 2).withCentre (c), lw * 1.2f);
                // entaille d'index
                juce::Path notch;
                notch.addRoundedRectangle (-lw * 0.9f, -kr * 0.95f, lw * 1.8f, kr * 0.62f, lw * 0.9f);
                g.fillPath (notch, juce::AffineTransform::rotation (a).translated (c));
                // repère de valeur : petit triangle vert sur la graduation
                juce::Path tri;
                tri.addTriangle (0.0f, -r * 0.99f, -r * 0.07f, -r * 0.82f, r * 0.07f, -r * 0.82f);
                g.setColour (slime);
                g.fillPath (tri, juce::AffineTransform::rotation (a).translated (c));
                break;
            }

            //------------------------------------------------------------------
            case 1:   // Bakélite 303 : jupe crantée + capuchon
            {
                drawTicks (g, c, r * 0.86f, r * 0.97f, start, end, 11, inkC.withAlpha (0.75f), lw * 0.5f);
                const float sr = r * 0.80f;
                juce::Path skirt;
                const int teeth = 28;
                for (int i = 0; i <= teeth * 2; ++i)
                {
                    const float aa = juce::MathConstants<float>::twoPi * (float) i / (float) (teeth * 2);
                    const float rr = (i % 2 == 0) ? sr : sr * 0.93f;
                    const auto pt = c.getPointOnCircumference (rr, aa);
                    if (i == 0) skirt.startNewSubPath (pt); else skirt.lineTo (pt);
                }
                skirt.closeSubPath();
                g.setColour (juce::Colour (0x55000000));
                g.fillPath (skirt, juce::AffineTransform::translation (0.0f, r * 0.06f));
                g.setColour (juce::Colour (0xff2a2725));
                g.fillPath (skirt);
                const float cr = sr * 0.66f;
                g.setColour (juce::Colour (0xff3b3734));
                g.fillEllipse (juce::Rectangle<float> (cr * 2, cr * 2).withCentre (c));
                g.setColour (juce::Colour (0x22ffffff));
                g.drawEllipse (juce::Rectangle<float> (cr * 2, cr * 2).withCentre (c), lw * 0.6f);
                g.setColour (juce::Colour (0x18ffffff));
                g.fillEllipse (juce::Rectangle<float> (cr * 1.2f, cr * 0.8f).withCentre (c.translated (-cr * 0.2f, -cr * 0.35f)));
                // trait d'index crème sur la jupe et le capuchon
                g.setColour (enabled ? juce::Colour (0xfff2ead6) : dim);
                g.drawLine ({ c.getPointOnCircumference (cr * 0.25f, a), c.getPointOnCircumference (sr * 0.97f, a) }, lw * 1.1f);
                break;
            }

            //------------------------------------------------------------------
            case 2:   // Tête de poulet
            {
                drawTicks (g, c, r * 0.84f, r * 0.96f, start, end, 11, inkC.withAlpha (0.85f), lw * 0.55f);
                const float br = r * 0.42f;
                juce::Path head;
                // corps arrondi + pointe allongée vers le haut (orientation 0 = midi)
                head.addEllipse (-br, -br, br * 2, br * 2);
                juce::Path beak;
                beak.startNewSubPath (-br * 0.62f, -br * 0.55f);
                beak.quadraticTo (-br * 0.35f, -r * 0.70f, 0.0f, -r * 0.80f);
                beak.quadraticTo (br * 0.35f, -r * 0.70f, br * 0.62f, -br * 0.55f);
                beak.closeSubPath();
                head.addPath (beak);
                const auto xf = juce::AffineTransform::rotation (a).translated (c);
                g.setColour (juce::Colour (0x44000000));
                g.fillPath (head, xf.translated (0.0f, r * 0.05f));
                g.setColour (enabled ? juce::Colour (0xff2a2725) : dim);
                g.fillPath (head, xf);
                // filet crème le long de la pointe
                g.setColour (paper);
                g.drawLine ({ c.getPointOnCircumference (br * 0.2f, a), c.getPointOnCircumference (r * 0.74f, a) }, lw * 0.9f);
                g.setColour (slime);
                g.fillEllipse (juce::Rectangle<float> (lw * 1.6f, lw * 1.6f).withCentre (c));
                break;
            }

            //------------------------------------------------------------------
            case 3:   // Bauhaus : disque plat + point + fine couronne
            {
                const float tr = r * 0.92f;
                juce::Path track;
                track.addCentredArc (c.x, c.y, tr, tr, 0.0f, start, end, true);
                g.setColour (dim.withAlpha (0.5f));
                g.strokePath (track, juce::PathStrokeType (lw * 0.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                const float from = bipolar ? (start + end) * 0.5f : start;
                if (std::abs (a - from) > 0.01f)
                {
                    juce::Path val;
                    val.addCentredArc (c.x, c.y, tr, tr, 0.0f, juce::jmin (from, a), juce::jmax (from, a), true);
                    g.setColour (enabled ? slime : dim);
                    g.strokePath (val, juce::PathStrokeType (lw * 1.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                }
                const float kr = r * 0.74f;
                g.setColour (inkC);
                g.fillEllipse (juce::Rectangle<float> (kr * 2, kr * 2).withCentre (c));
                g.setColour (paper);
                const float d = kr * 0.26f;
                g.fillEllipse (juce::Rectangle<float> (d, d).withCentre (c.getPointOnCircumference (kr * 0.66f, a)));
                break;
            }

            //------------------------------------------------------------------
            case 4:   // BD : contour main levée, trame de points, gros point d'index
            {
                // petites graduations « tracées à la main »
                g.setColour (inkC.withAlpha (0.8f));
                for (int i = 0; i < 9; ++i)
                {
                    const float t = (float) i / 8.0f;
                    const float aa = start + t * (end - start);
                    g.drawLine ({ c.getPointOnCircumference (r * 0.86f, aa), c.getPointOnCircumference (r * (i % 4 == 0 ? 1.0f : 0.95f), aa) }, lw * 0.7f);
                }
                const float kr = r * 0.70f;
                // ombre en trame de points (en bas à droite)
                {
                    juce::Graphics::ScopedSaveState ss (g);
                    juce::Path sh = wobblyCircle (c.translated (r * 0.14f, r * 0.15f), kr, 0.012f, seed + 7);
                    g.reduceClipRegion (sh);
                    g.setColour (inkC.withAlpha (0.8f));
                    const float step = juce::jmax (3.0f, r * 0.1f);
                    for (float yy = c.y - kr; yy < c.y + kr * 1.3f; yy += step)
                        for (float xx = c.x - kr; xx < c.x + kr * 1.3f; xx += step)
                            g.fillEllipse (xx, yy, step * 0.5f, step * 0.5f);
                }
                juce::Path body = wobblyCircle (c, kr, 0.012f, seed);
                g.setColour (paper.interpolatedWith (slime, 0.18f));
                g.fillPath (body);
                g.setColour (inkC);
                g.strokePath (body, juce::PathStrokeType (lw * 1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                // reflet en virgule
                juce::Path hl;
                hl.addCentredArc (c.x, c.y, kr * 0.72f, kr * 0.72f, 0.0f, -2.4f, -1.5f, true);
                g.setColour (juce::Colours::white.withAlpha (0.7f));
                g.strokePath (hl, juce::PathStrokeType (lw * 0.9f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                // index : gros point + petit trait
                const auto tip = c.getPointOnCircumference (kr * 0.62f, a);
                g.setColour (inkC);
                g.drawLine ({ c.getPointOnCircumference (kr * 0.18f, a), tip }, lw * 1.1f);
                const float d = kr * 0.30f;
                g.fillEllipse (juce::Rectangle<float> (d, d).withCentre (tip));
                break;
            }

            //------------------------------------------------------------------
            case 5:   // Cannelé studio : bouton cannelé noir + insert métal
            default:
            {
                drawTicks (g, c, r * 0.86f, r * 0.97f, start, end, 11, inkC.withAlpha (0.7f), lw * 0.5f);
                const float sr = r * 0.78f;
                juce::Path flutes;
                const int n = 16;
                for (int i = 0; i < n; ++i)
                {
                    const float a0 = juce::MathConstants<float>::twoPi * (float) i / (float) n;
                    const float a1 = juce::MathConstants<float>::twoPi * ((float) i + 0.5f) / (float) n;
                    if (i == 0) flutes.startNewSubPath (c.getPointOnCircumference (sr, a0));
                    flutes.lineTo (c.getPointOnCircumference (sr, a0));
                    flutes.quadraticTo (c.getPointOnCircumference (sr * 0.86f, a1),
                                        c.getPointOnCircumference (sr, juce::MathConstants<float>::twoPi * (float) (i + 1) / (float) n));
                }
                flutes.closeSubPath();
                g.setColour (juce::Colour (0x55000000));
                g.fillPath (flutes, juce::AffineTransform::translation (0.0f, r * 0.06f));
                g.setColour (juce::Colour (0xff1f1d1b));
                g.fillPath (flutes);
                // insert métal (dégradé brossé)
                const float mr = sr * 0.66f;
                juce::ColourGradient grad (juce::Colour (0xfff1efe9), c.x - mr, c.y - mr, juce::Colour (0xff9d9a93), c.x + mr, c.y + mr, false);
                g.setGradientFill (grad);
                g.fillEllipse (juce::Rectangle<float> (mr * 2, mr * 2).withCentre (c));
                g.setColour (juce::Colour (0x33000000));
                g.drawEllipse (juce::Rectangle<float> (mr * 2, mr * 2).withCentre (c), lw * 0.5f);
                g.setColour (enabled ? ink : dim);
                g.drawLine ({ c.getPointOnCircumference (mr * 0.15f, a), c.getPointOnCircumference (mr * 0.9f, a) }, lw * 1.0f);
                g.setColour (slime);
                g.fillEllipse (juce::Rectangle<float> (lw * 1.4f, lw * 1.4f).withCentre (c.getPointOnCircumference (sr * 1.08f, a)));
                break;
            }
        }
    }
}

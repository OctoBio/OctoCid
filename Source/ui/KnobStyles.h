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

    //==========================================================================
    // Variantes « tête de poulet »
    //==========================================================================
    inline constexpr int chickenCount = 10;
    inline const char* chickenNames[chickenCount] = {
        "A  Bakelite noire", "B  Imprime creme", "C  Vert bave", "D  Fin + echelle 0-10",
        "E  Jupe graduee", "F  BD main levee", "G  Geometrique", "H  Glitch split", "I  Glitch neon", "J  Glitch pixel" };

    // Silhouette : disque + pointe vers le haut (angle 0 = midi)
    inline juce::Path chickenShape (float br, float tip, float width = 0.62f)
    {
        juce::Path head;
        head.addEllipse (-br, -br, br * 2, br * 2);
        juce::Path beak;
        beak.startNewSubPath (-br * width, -br * 0.55f);
        beak.quadraticTo (-br * width * 0.55f, -tip * 0.88f, 0.0f, -tip);
        beak.quadraticTo (br * width * 0.55f, -tip * 0.88f, br * width, -br * 0.55f);
        beak.closeSubPath();
        head.addPath (beak);
        return head;
    }

    // Arc de valeur fin (depuis le début, ou depuis midi en bipolaire)
    inline void valueArc (juce::Graphics& g, juce::Point<float> c, float rr, float start, float end, float a,
                          bool bipolar, juce::Colour col, float thick)
    {
        const float from = bipolar ? (start + end) * 0.5f : start;
        if (std::abs (a - from) < 0.01f) return;
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, rr, rr, 0.0f, juce::jmin (from, a), juce::jmax (from, a), true);
        g.setColour (col);
        g.strokePath (arc, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    inline void chicken (int v, juce::Graphics& g, juce::Rectangle<float> bounds, float pos,
                         float start, float end, bool bipolar, bool enabled, int seed = 1)
    {
        using namespace OctoColours;
        const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const float a = start + pos * (end - start);
        const auto paper = juce::Colour (0xfff6efdf);
        const auto bake  = juce::Colour (0xff2a2725);
        const auto inkC = enabled ? ink : dim;
        const auto acc  = enabled ? slime : dim;
        const float lw = juce::jmax (1.4f, r * 0.07f);
        const auto xf = juce::AffineTransform::rotation (a).translated (c);

        switch (juce::jlimit (0, chickenCount - 1, v))
        {
            case 0:   // A — bakélite noire, filet crème (la version choisie)
            {
                drawTicks (g, c, r * 0.84f, r * 0.96f, start, end, 11, inkC.withAlpha (0.85f), lw * 0.55f);
                valueArc (g, c, r * 0.76f, start, end, a, bipolar, acc.withAlpha (0.8f), lw * 0.6f);
                const float br = r * 0.42f;
                auto head = chickenShape (br, r * 0.74f);
                g.setColour (juce::Colour (0x44000000));
                g.fillPath (head, xf.translated (0.0f, r * 0.05f));
                g.setColour (enabled ? bake : dim);
                g.fillPath (head, xf);
                g.setColour (juce::Colour (0x16ffffff));
                g.fillEllipse (juce::Rectangle<float> (br * 1.1f, br * 0.7f).withCentre (c.translated (-br * 0.25f, -br * 0.35f)));
                g.setColour (paper);
                g.drawLine ({ c.getPointOnCircumference (br * 0.15f, a), c.getPointOnCircumference (r * 0.68f, a) }, lw * 0.9f);
                break;
            }

            case 1:   // B — imprimé : corps crème, contour encre, ombre pleine décalée
            {
                drawTicks (g, c, r * 0.84f, r * 0.96f, start, end, 11, inkC, lw * 0.6f);
                valueArc (g, c, r * 0.77f, start, end, a, bipolar, acc, lw * 0.75f);
                const float br = r * 0.44f;
                auto head = chickenShape (br, r * 0.72f, 0.66f);
                g.setColour (inkC);
                g.fillPath (head, xf.translated (r * 0.07f, r * 0.08f));
                g.strokePath (head, juce::PathStrokeType (lw * 1.7f, juce::PathStrokeType::curved), xf);
                g.setColour (paper);
                g.fillPath (head, xf);
                g.setColour (inkC);
                g.drawLine ({ c.getPointOnCircumference (br * 0.1f, a), c.getPointOnCircumference (r * 0.62f, a) }, lw * 1.1f);
                break;
            }

            case 2:   // C — corps vert bave, filet encre
            {
                drawTicks (g, c, r * 0.84f, r * 0.96f, start, end, 11, inkC.withAlpha (0.85f), lw * 0.55f);
                const float br = r * 0.44f;
                auto head = chickenShape (br, r * 0.76f);
                g.setColour (juce::Colour (0x40000000));
                g.fillPath (head, xf.translated (0.0f, r * 0.06f));
                g.setColour (inkC);
                g.strokePath (head, juce::PathStrokeType (lw * 1.4f), xf);
                g.setColour (acc);
                g.fillPath (head, xf);
                g.setColour (juce::Colour (0x30ffffff));
                g.fillEllipse (juce::Rectangle<float> (br * 1.0f, br * 0.6f).withCentre (c.translated (-br * 0.25f, -br * 0.4f)));
                g.setColour (inkC);
                g.drawLine ({ c.getPointOnCircumference (br * 0.1f, a), c.getPointOnCircumference (r * 0.68f, a) }, lw * 1.0f);
                break;
            }

            case 3:   // D — fin, longue pointe, échelle numérotée 0-10
            {
                const float tr = r * 0.80f;
                drawTicks (g, c, tr * 0.93f, tr, start, end, 21, inkC.withAlpha (0.7f), lw * 0.4f, false);
                g.setFont (juce::FontOptions (juce::jmax (7.0f, r * 0.20f), juce::Font::bold));
                g.setColour (inkC.withAlpha (0.85f));
                for (int i = 0; i <= 10; i += 2)
                {
                    const float aa = start + (float) i / 10.0f * (end - start);
                    const auto pt = c.getPointOnCircumference (r * 0.94f, aa);
                    const int lab = bipolar ? i - 5 : i;
                    g.drawText (juce::String (lab), juce::Rectangle<float> (r * 0.4f, r * 0.24f).withCentre (pt),
                                juce::Justification::centred);
                }
                const float br = r * 0.34f;
                auto head = chickenShape (br, r * 0.74f, 0.5f);
                g.setColour (juce::Colour (0x44000000));
                g.fillPath (head, xf.translated (0.0f, r * 0.05f));
                g.setColour (enabled ? bake : dim);
                g.fillPath (head, xf);
                g.setColour (acc);
                g.drawLine ({ c.getPointOnCircumference (br * 0.4f, a), c.getPointOnCircumference (r * 0.70f, a) }, lw * 0.75f);
                break;
            }

            case 4:   // E — jupe graduée (disque crème gradué qui tourne) + tête noire
            {
                const float sr = r * 0.86f;
                g.setColour (juce::Colour (0x40000000));
                g.fillEllipse (juce::Rectangle<float> (sr * 2, sr * 2).withCentre (c.translated (0.0f, r * 0.05f)));
                g.setColour (panelHi.interpolatedWith (paper, 0.5f));
                g.fillEllipse (juce::Rectangle<float> (sr * 2, sr * 2).withCentre (c));
                g.setColour (inkC);
                g.drawEllipse (juce::Rectangle<float> (sr * 2, sr * 2).withCentre (c), lw * 0.6f);
                // graduations fixes sur la jupe, l'arc de valeur en vert
                drawTicks (g, c, sr * 0.80f, sr * 0.93f, start, end, 11, inkC.withAlpha (0.7f), lw * 0.45f, false);
                valueArc (g, c, sr * 0.72f, start, end, a, bipolar, acc, lw * 0.7f);
                const float br = r * 0.36f;
                auto head = chickenShape (br, sr * 0.95f, 0.55f);
                g.setColour (enabled ? bake : dim);
                g.fillPath (head, xf);
                g.setColour (paper);
                g.drawLine ({ c.getPointOnCircumference (br * 0.2f, a), c.getPointOnCircumference (sr * 0.86f, a) }, lw * 0.85f);
                break;
            }

            case 5:   // F — BD : contour main levée, trame de points en ombre
            {
                drawTicks (g, c, r * 0.84f, r * 0.96f, start, end, 11, inkC, lw * 0.6f);
                valueArc (g, c, r * 0.77f, start, end, a, bipolar, acc, lw * 0.8f);
                const float br = r * 0.45f;
                juce::ignoreUnused (seed);
                juce::Path head;
                head.addEllipse (-br, -br, br * 2, br * 2);
                juce::Path beak;
                beak.startNewSubPath (-br * 0.62f, -br * 0.56f);
                beak.quadraticTo (-br * 0.30f, -r * 0.62f, 0.0f, -r * 0.73f);
                beak.quadraticTo (br * 0.30f, -r * 0.62f, br * 0.62f, -br * 0.56f);
                beak.closeSubPath();
                head.addPath (beak);
                // trame (points) décalée = ombre
                {
                    juce::Graphics::ScopedSaveState ss (g);
                    juce::Path sh (head);
                    sh.applyTransform (xf.translated (r * 0.09f, r * 0.10f));
                    g.reduceClipRegion (sh);
                    g.setColour (inkC.withAlpha (0.75f));
                    const float step = juce::jmax (2.6f, r * 0.11f);
                    auto bb = sh.getBounds();
                    for (float yy = bb.getY(); yy < bb.getBottom(); yy += step)
                        for (float xx = bb.getX() + (std::fmod (yy, step * 2.0f) < step ? 0.0f : step * 0.5f); xx < bb.getRight(); xx += step)
                            g.fillEllipse (xx, yy, step * 0.45f, step * 0.45f);
                }
                g.setColour (inkC);
                g.strokePath (head, juce::PathStrokeType (lw * 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), xf);
                g.setColour (paper);
                g.fillPath (head, xf);
                g.setColour (inkC);
                g.fillEllipse (juce::Rectangle<float> (lw * 2.2f, lw * 2.2f).withCentre (c.getPointOnCircumference (r * 0.50f, a)));
                break;
            }

            case 7:   // H — glitch « split » : tête crème, fantômes cyan / magenta, arc en blocs
            case 8:   // I — glitch néon : tête noire, contour vert lumineux
            case 9:   // J — glitch pixel : tête rayée de balayage, tranche décalée, arc en pixels
            {
                const auto cyan = juce::Colour (0xff00e8ff), mag = juce::Colour (0xffff2ac8);
                // arc segmenté (blocs)
                const int nb = 15;
                const float from = bipolar ? 0.5f : 0.0f;
                auto blocks = [&] (juce::Point<float> off, juce::Colour on, juce::Colour offc)
                {
                    for (int i = 0; i < nb; ++i)
                    {
                        const float t = (float) i / (float) (nb - 1);
                        const bool lit = enabled && (t >= juce::jmin (from, pos) - 0.001f && t <= juce::jmax (from, pos) + 0.001f);
                        const float aa = start + t * (end - start);
                        juce::Path b;
                        const float bw = r * (v == 9 ? 0.13f : 0.09f), bh = r * (v == 9 ? 0.13f : 0.17f);
                        b.addRectangle (-bw * 0.5f, -r * 0.97f, bw, bh);
                        if (! lit && offc.isTransparent()) continue;
                        g.setColour (lit ? on : offc);
                        g.fillPath (b, juce::AffineTransform::rotation (aa).translated (c + off));
                    }
                };
                if (v == 8)
                {
                    blocks ({ -r * 0.05f, 0 }, cyan.withAlpha (0.7f), juce::Colours::transparentBlack);
                    blocks ({  r * 0.05f, 0 }, mag.withAlpha (0.7f), juce::Colours::transparentBlack);
                }
                blocks ({}, acc, inkC.withAlpha (0.22f));

                const float br = r * 0.44f;
                auto head = chickenShape (br, r * 0.72f, 0.62f);
                if (v == 7)
                {
                    g.setColour (cyan.withAlpha (0.9f)); g.fillPath (head, xf.translated (-r * 0.13f, r * 0.02f));
                    g.setColour (mag.withAlpha (0.9f));  g.fillPath (head, xf.translated ( r * 0.13f, -r * 0.02f));
                    g.setColour (inkC); g.strokePath (head, juce::PathStrokeType (lw * 1.6f), xf);
                    g.setColour (paper); g.fillPath (head, xf);
                    g.setColour (inkC);
                    g.drawLine ({ c.getPointOnCircumference (br * 0.1f, a), c.getPointOnCircumference (r * 0.62f, a) }, lw * 1.1f);
                }
                else if (v == 8)
                {
                    g.setColour (acc.withAlpha (0.25f)); g.strokePath (head, juce::PathStrokeType (lw * 3.0f), xf);
                    g.setColour (juce::Colour (0xff141216)); g.fillPath (head, xf);
                    g.setColour (acc); g.strokePath (head, juce::PathStrokeType (lw * 0.8f), xf);
                    g.drawLine ({ c.getPointOnCircumference (br * 0.1f, a), c.getPointOnCircumference (r * 0.64f, a) }, lw * 1.0f);
                    g.setColour (cyan.withAlpha (0.6f));
                    g.drawLine ({ c.getPointOnCircumference (br * 0.1f, a).translated (-1.2f, 0), c.getPointOnCircumference (r * 0.64f, a).translated (-1.2f, 0) }, lw * 0.5f);
                }
                else
                {
                    g.setColour (inkC); g.strokePath (head, juce::PathStrokeType (lw * 1.6f), xf);
                    g.setColour (paper); g.fillPath (head, xf);
                    {
                        juce::Graphics::ScopedSaveState ss (g);
                        juce::Path hp (head); hp.applyTransform (xf);
                        g.reduceClipRegion (hp);
                        g.setColour (inkC.withAlpha (0.22f));
                        const auto hb = hp.getBounds();
                        for (float y = hb.getY(); y < hb.getBottom(); y += juce::jmax (2.0f, r * 0.08f)) g.fillRect (hb.getX(), y, hb.getWidth(), 1.0f);
                    }
                    // tranche décalée
                    {
                        juce::Graphics::ScopedSaveState ss (g);
                        g.reduceClipRegion (juce::Rectangle<float> (c.x - r, c.y - r * 0.12f, r * 2.0f, r * 0.14f).toNearestInt());
                        g.setColour (mag.withAlpha (0.85f)); g.fillPath (head, xf.translated (r * 0.12f, 0));
                    }
                    g.setColour (inkC);
                    g.drawLine ({ c.getPointOnCircumference (br * 0.1f, a), c.getPointOnCircumference (r * 0.62f, a) }, lw * 1.1f);
                }
                break;
            }

            default:  // G — géométrique : disque plat + triangle franc, sans contour
            {
                drawTicks (g, c, r * 0.86f, r * 0.96f, start, end, 11, inkC.withAlpha (0.6f), lw * 0.5f, false);
                valueArc (g, c, r * 0.78f, start, end, a, bipolar, acc, lw * 0.9f);
                const float br = r * 0.44f;
                juce::Path head;
                head.addEllipse (-br, -br, br * 2, br * 2);
                head.addTriangle (br * 0.7f, -br * 0.5f, -br * 0.7f, -br * 0.5f, 0.0f, -r * 0.74f);
                head.setUsingNonZeroWinding (false);
                { juce::Path t; t.addEllipse (-br, -br, br * 2, br * 2); juce::Path tri; tri.addTriangle (-br * 0.7f, -br * 0.5f, br * 0.7f, -br * 0.5f, 0.0f, -r * 0.74f); head = t; g.setColour (enabled ? ink : dim); g.fillPath (tri, xf); }
                g.setColour (enabled ? ink : dim);
                g.fillPath (head, xf);
                g.setColour (acc);
                g.fillEllipse (juce::Rectangle<float> (lw * 2.0f, lw * 2.0f).withCentre (c.getPointOnCircumference (r * 0.58f, a)));
                break;
            }
        }
    }
}

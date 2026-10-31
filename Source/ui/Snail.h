#pragma once

#include <JuceHeader.h>

//==============================================================================
// L'escargot d'OctoCid, version « méchant et défoncé » : cartoon détaillé,
// coquille fissurée qui fuit de l'acide, yeux injectés aux paupières lourdes,
// rictus édenté qui bave. Tout en vectoriel, couleurs fixes (indépendantes de la palette).
// Espace de dessin : 240 x 170, mis à l'échelle dans la zone donnée.
//==============================================================================
namespace OctoSnail
{
    inline const juce::Colour kInk      { 0xff1d1a17 };
    inline const juce::Colour kAcid     { 0xff8ef01e };
    inline const juce::Colour kAcidHi   { 0xffe6ff9a };
    inline const juce::Colour kSkinHi   { 0xffd6e27a };
    inline const juce::Colour kSkin     { 0xffa3bb3e };
    inline const juce::Colour kSkinLo   { 0xff4d6416 };

    inline void draw (juce::Graphics& g, juce::Rectangle<float> area)
    {
        constexpr float DW = 250.0f, DH = 172.0f;
        const float s = juce::jmin (area.getWidth() / DW, area.getHeight() / DH);
        const auto o = area.getCentre().translated (-DW * 0.5f * s, -DH * 0.5f * s);
        auto P = [&] (float x, float y) { return juce::Point<float> (o.x + (x + 2.0f) * s, o.y + (y + 6.0f) * s); };
        auto R = [&] (float cx, float cy, float w, float h) { return juce::Rectangle<float> (w * s, h * s).withCentre (P (cx, cy)); };
        auto stroke = [&] (float w) { return juce::PathStrokeType (w * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded); };
        juce::Random rnd (1303);

        //======================================================================
        // Ombre au sol
        for (int i = 0; i < 6; ++i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.07f));
            g.fillEllipse (R (128, 153, 210 - i * 18, 16 - i * 1.5f));
        }

        //======================================================================
        // Flaque / traînée d'acide derrière lui
        {
            juce::Path trail;
            trail.startNewSubPath (P (2, 150));
            trail.cubicTo (P (10, 143), P (30, 145), P (46, 144));
            trail.cubicTo (P (58, 143), P (66, 147), P (80, 147));
            trail.lineTo (P (80, 154));
            trail.cubicTo (P (60, 157), P (40, 152), P (26, 156));
            trail.cubicTo (P (14, 158), P (4, 156), P (2, 150));
            trail.closeSubPath();
            g.setGradientFill (juce::ColourGradient (kAcid.withAlpha (0.25f), P (0, 150), kAcid.withAlpha (0.85f), P (80, 150), false));
            g.fillPath (trail);
            g.setColour (kInk.withAlpha (0.55f));
            g.strokePath (trail, stroke (1.2f));
            g.setColour (juce::Colours::white.withAlpha (0.7f));
            juce::Path shine;
            shine.startNewSubPath (P (20, 148)); shine.quadraticTo (P (40, 146), P (62, 147.5f));
            g.strokePath (shine, stroke (1.1f));
            for (auto d : { juce::Point<float> (10, 160), juce::Point<float> (34, 161), juce::Point<float> (58, 159) })
            {
                g.setColour (kAcid.withAlpha (0.8f)); g.fillEllipse (R (d.x, d.y, 4, 3));
                g.setColour (juce::Colours::white.withAlpha (0.8f)); g.fillEllipse (R (d.x - 0.8f, d.y - 0.6f, 1.2f, 1));
            }
        }

        //======================================================================
        // Corps : pied visqueux + cou dressé + tête
        juce::Path body;
        body.startNewSubPath (P (20, 147));
        body.cubicTo (P (40, 136), P (70, 128), P (110, 127));
        body.lineTo (P (150, 125));
        body.cubicTo (P (162, 110), P (158, 82), P (174, 66));
        body.cubicTo (P (186, 53), P (212, 55), P (221, 72));
        body.cubicTo (P (229, 88), P (229, 116), P (221, 130));
        body.cubicTo (P (217, 140), P (226, 145), P (229, 150));
        body.cubicTo (P (200, 157), P (80, 157), P (30, 153));
        body.cubicTo (P (22, 152), P (15, 150), P (20, 147));
        body.closeSubPath();

        g.setColour (kInk);
        g.strokePath (body, stroke (5.5f));
        g.setGradientFill (juce::ColourGradient (kSkinHi, P (190, 60), kSkinLo, P (150, 158), false));
        g.fillPath (body);
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (body);
            // bande ventrale plus sombre
            g.setColour (kSkinLo.withAlpha (0.65f));
            g.fillRect (juce::Rectangle<float> (P (0, 146), P (240, 170)));
            g.setColour (kInk.withAlpha (0.35f));
            juce::Path sole; sole.startNewSubPath (P (30, 147)); sole.cubicTo (P (90, 150), P (170, 150), P (226, 146));
            g.strokePath (sole, stroke (1.0f));
            // reflet gluant
            juce::Path gloss;
            gloss.startNewSubPath (P (40, 140)); gloss.cubicTo (P (80, 131), P (120, 131), P (150, 129));
            g.setColour (juce::Colours::white.withAlpha (0.45f));
            g.strokePath (gloss, stroke (2.0f));
            juce::Path gloss2;
            gloss2.startNewSubPath (P (172, 76)); gloss2.cubicTo (P (180, 64), P (198, 60), P (210, 64));
            g.strokePath (gloss2, stroke (2.4f));
            // taches sombres
            for (auto t : { juce::Point<float> (70, 140), juce::Point<float> (120, 142), juce::Point<float> (166, 132),
                            juce::Point<float> (200, 120), juce::Point<float> (183, 94) })
            {
                g.setColour (kSkinLo.withAlpha (0.55f));
                g.fillEllipse (R (t.x, t.y, 9 + rnd.nextFloat() * 6, 5 + rnd.nextFloat() * 4));
            }
            // pustules / verrues
            for (int i = 0; i < 90; ++i)
            {
                const float x = 30 + rnd.nextFloat() * 200, y = 60 + rnd.nextFloat() * 92;
                if (! body.contains (P (x, y))) continue;
                const float rr = 0.8f + rnd.nextFloat() * 2.0f;
                g.setColour (kSkinLo.withAlpha (0.55f));
                g.fillEllipse (R (x + 0.4f, y + 0.5f, rr * 2, rr * 2));
                g.setColour (kSkinHi.brighter (0.2f).withAlpha (0.8f));
                g.fillEllipse (R (x - 0.3f, y - 0.4f, rr, rr));
            }
            // rides du cou
            for (int i = 0; i < 4; ++i)
            {
                juce::Path w;
                const float y = 92 + i * 9.0f;
                w.startNewSubPath (P (160 + i * 0.5f, y)); w.quadraticTo (P (168, y - 3), P (176, y + 1));
                g.setColour (kInk.withAlpha (0.4f));
                g.strokePath (w, stroke (1.1f));
            }
        }

        //======================================================================
        // Coquille fissurée
        const auto sc = P (98, 80);
        const float SR = 58.0f * s;
        juce::Path shell;
        shell.addEllipse (juce::Rectangle<float> (SR * 2.05f, SR * 1.95f).withCentre (sc));
        // ébréchure en haut à droite
        juce::Path chip;
        chip.startNewSubPath (P (128, 28)); chip.lineTo (P (138, 30)); chip.lineTo (P (134, 37)); chip.lineTo (P (142, 38)); chip.lineTo (P (152, 44));
        chip.lineTo (P (170, 20)); chip.lineTo (P (120, 10)); chip.closeSubPath();

        {
            juce::Graphics::ScopedSaveState ss (g);
            // ombre portée de la coquille sur le corps
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillPath (shell, juce::AffineTransform::translation (4 * s, 5 * s));
            g.setColour (kInk);
            g.strokePath (shell, stroke (6.0f));
            juce::ColourGradient sg (juce::Colour (0xffe0a25a), P (74, 46), juce::Colour (0xff2e140a), P (140, 135), true);
            sg.addColour (0.45, juce::Colour (0xff8f4a22));
            sg.addColour (0.75, juce::Colour (0xff55260f));
            g.setGradientFill (sg);
            g.fillPath (shell);

            g.reduceClipRegion (shell);

            // spirale logarithmique : bandes, sillons, stries de croissance
            const float turns = 2.8f;
            auto spiralPt = [&] (float t)   // t 0 = bord, 1 = centre
            {
                const float a = -0.6f + t * juce::MathConstants<float>::twoPi * turns;
                const float rr = SR * 0.98f * std::pow (0.07f, t);
                return sc.translated (std::cos (a) * rr * 1.02f, std::sin (a) * rr * 0.97f);
            };
            const int N = 420;
            juce::Path groove;
            for (int i = 0; i <= N; ++i)
            {
                const auto pt = spiralPt ((float) i / (float) N);
                if (i == 0) groove.startNewSubPath (pt); else groove.lineTo (pt);
            }
            // bandes plus sombres entre les tours
            g.setColour (juce::Colour (0xff3a1a0a).withAlpha (0.35f));
            g.strokePath (groove, juce::PathStrokeType (SR * 0.22f, juce::PathStrokeType::curved), juce::AffineTransform::rotation (0.5f, sc.x, sc.y));
            // stries de croissance (perpendiculaires au sillon)
            for (int i = 4; i < N - 30; i += 3)
            {
                const float t = (float) i / (float) N;
                const auto p0 = spiralPt (t);
                const float a = -0.6f + t * juce::MathConstants<float>::twoPi * turns;
                const float rr = SR * 0.98f * std::pow (0.07f, t);
                const float inner = rr * std::pow (0.07f, 1.0f / turns);
                const auto p1 = sc.translated (std::cos (a) * inner * 1.02f, std::sin (a) * inner * 0.97f);
                g.setColour ((i % 9 == 0 ? juce::Colour (0xff1e0c05) : juce::Colour (0xffffd9a0)).withAlpha (i % 9 == 0 ? 0.35f : 0.12f));
                g.drawLine ({ p0, p0 + (p1 - p0) * (0.75f + rnd.nextFloat() * 0.2f) }, (i % 9 == 0 ? 1.0f : 0.7f) * s);
            }
            // sillon : creux sombre + lèvre éclairée
            g.setColour (kInk.withAlpha (0.9f));
            g.strokePath (groove, stroke (3.2f));
            g.setColour (juce::Colour (0xfff3c58a).withAlpha (0.55f));
            g.strokePath (groove, stroke (1.1f), juce::AffineTransform::translation (-1.4f * s, -1.4f * s));

            // reflet spéculaire
            juce::Path spec;
            spec.addCentredArc (sc.x - 6 * s, sc.y - 6 * s, SR * 0.80f, SR * 0.78f, 0.0f, -2.5f, -1.2f, true);
            g.setColour (juce::Colours::white.withAlpha (0.30f));
            g.strokePath (spec, juce::PathStrokeType (7 * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillEllipse (R (68, 40, 7, 4));
            g.fillEllipse (R (60, 50, 3, 3));

            // ébréchure
            g.setColour (juce::Colour (0xff1a0c06));
            g.fillPath (chip);
            g.setColour (juce::Colour (0xffc48a52));
            juce::Path chipEdge; chipEdge.startNewSubPath (P (128, 28)); chipEdge.lineTo (P (138, 30)); chipEdge.lineTo (P (134, 37));
            chipEdge.lineTo (P (142, 38)); chipEdge.lineTo (P (152, 44));
            g.strokePath (chipEdge, stroke (1.6f));
        }
        g.setColour (kInk);
        g.strokePath (shell, stroke (3.0f));

        // fissures qui laissent fuir l'acide (lueur)
        auto crack = [&] (juce::Point<float> a, juce::Point<float> b, int seed)
        {
            juce::Random cr (seed);
            juce::Path c; c.startNewSubPath (P (a.x, a.y));
            const int n = 9;
            const auto d = b - a;
            const juce::Point<float> nrm (-d.y, d.x);
            const float len = d.getDistanceFromOrigin();
            for (int i = 1; i <= n; ++i)
            {
                const float t = (float) i / (float) n;
                const float j = (i == n) ? 0.0f : (cr.nextFloat() - 0.5f) * 0.35f;
                const auto p = a + d * t + nrm * (j * 1.0f);
                c.lineTo (P (p.x, p.y));
                if (i == 4 || i == 6)   // petites ramifications
                {
                    const auto br = p + d * 0.18f + nrm * ((cr.nextFloat() - 0.5f) * 0.6f);
                    juce::Path b2; b2.startNewSubPath (P (p.x, p.y)); b2.lineTo (P (br.x, br.y));
                    g.setColour (kInk); g.strokePath (b2, stroke (1.2f));
                    g.setColour (kAcidHi.withAlpha (0.8f)); g.strokePath (b2, stroke (0.4f));
                }
            }
            juce::ignoreUnused (len);
            g.setColour (kAcid.withAlpha (0.22f)); g.strokePath (c, stroke (5.0f));
            g.setColour (kInk);                   g.strokePath (c, stroke (2.0f));
            g.setColour (kAcidHi);                g.strokePath (c, stroke (0.7f));
        };
        crack ({ 50, 48 }, { 64, 84 }, 3);
        crack ({ 142, 90 }, { 122, 112 }, 5);
        crack ({ 151, 40 }, { 143, 60 }, 9);
        crack ({ 76, 130 }, { 90, 104 }, 11);

        // coulure d'acide depuis la fissure de gauche
        {
            juce::Path drip;
            drip.startNewSubPath (P (61, 82));
            drip.cubicTo (P (60, 94), P (58, 104), P (58.5f, 114));
            drip.cubicTo (P (58.5f, 118), P (54, 121), P (54.5f, 126));
            drip.cubicTo (P (55, 132), P (63, 132), P (62.5f, 126));
            drip.cubicTo (P (62, 121), P (60.5f, 118), P (61, 113));
            drip.cubicTo (P (61.5f, 102), P (63, 92), P (64, 84));
            drip.closeSubPath();
            g.setColour (kInk); g.strokePath (drip, stroke (1.8f));
            g.setGradientFill (juce::ColourGradient (kAcidHi, P (58, 90), juce::Colour (0xff3f9a10), P (60, 132), false));
            g.fillPath (drip);
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillEllipse (R (56.8f, 125.5f, 1.6f, 2.6f));
        }
        // bulles d'acide qui s'échappent
        for (auto b : { juce::Point<float> (40, 40), juce::Point<float> (32, 26), juce::Point<float> (44, 16), juce::Point<float> (152, 30) })
        {
            const float br = 2.5f + rnd.nextFloat() * 3.0f;
            g.setColour (kAcid.withAlpha (0.35f)); g.fillEllipse (R (b.x, b.y, br * 2, br * 2));
            g.setColour (kInk.withAlpha (0.8f));   g.drawEllipse (R (b.x, b.y, br * 2, br * 2), 0.9f * s);
            g.setColour (juce::Colours::white);    g.fillEllipse (R (b.x - br * 0.35f, b.y - br * 0.35f, br * 0.6f, br * 0.6f));
        }

        //======================================================================
        // Pédoncules (le droit s'affaisse : il est défoncé)
        auto stalk = [&] (juce::Point<float> a, juce::Point<float> c, juce::Point<float> b, float w)
        {
            juce::Path st; st.startNewSubPath (P (a.x, a.y)); st.quadraticTo (P (c.x, c.y), P (b.x, b.y));
            g.setColour (kInk);  g.strokePath (st, stroke (w + 3.4f));
            g.setColour (kSkin); g.strokePath (st, stroke (w));
            g.setColour (kSkinHi.withAlpha (0.8f)); g.strokePath (st, stroke (w * 0.3f), juce::AffineTransform::translation (-w * 0.2f * s, 0));
            // anneaux / rides
            for (int i = 1; i < 5; ++i)
            {
                const float t = (float) i / 5.0f;
                const auto pt = P ((1 - t) * (1 - t) * a.x + 2 * (1 - t) * t * c.x + t * t * b.x,
                                   (1 - t) * (1 - t) * a.y + 2 * (1 - t) * t * c.y + t * t * b.y);
                g.setColour (kInk.withAlpha (0.45f));
                g.drawLine (pt.x - w * 0.4f * s, pt.y, pt.x + w * 0.4f * s, pt.y + 0.8f * s, 0.9f * s);
            }
        };
        stalk ({ 194, 64 }, { 190, 42 }, { 175, 26 }, 6.0f);
        stalk ({ 210, 66 }, { 222, 54 }, { 229, 45 }, 5.2f);

        //======================================================================
        // Yeux injectés, pupilles dilatées, paupières lourdes et sourcils froncés
        auto eye = [&] (float cx, float cy, float r, float lidAngle, float lookX, float lookY, int seed)
        {
            juce::Random er (seed);
            const auto c = P (cx, cy);
            const float rr = r * s;
            auto ball = juce::Rectangle<float> (rr * 2, rr * 2).withCentre (c);
            // cernes
            g.setColour (juce::Colour (0xff5a2a52).withAlpha (0.55f));
            g.fillEllipse (ball.expanded (rr * 0.18f).translated (0, rr * 0.22f));
            g.setColour (kInk);
            g.fillEllipse (ball.expanded (2.2f * s));
            juce::ColourGradient eg (juce::Colour (0xfffffbe6), c.translated (-rr * 0.3f, -rr * 0.3f), juce::Colour (0xffe9b49a), c.translated (rr, rr), true);
            g.setGradientFill (eg);
            g.fillEllipse (ball);
            // veines
            {
                juce::Graphics::ScopedSaveState ss (g);
                juce::Path clip; clip.addEllipse (ball); g.reduceClipRegion (clip);
                for (int i = 0; i < 9; ++i)
                {
                    float a = er.nextFloat() * juce::MathConstants<float>::twoPi;
                    auto p = c.getPointOnCircumference (rr * 1.02f, a);
                    juce::Path v; v.startNewSubPath (p);
                    for (int k = 0; k < 4; ++k)
                    {
                        a += (er.nextFloat() - 0.5f) * 0.7f;
                        const float dist = rr * (0.95f - 0.15f * (float) (k + 1));
                        p = c.getPointOnCircumference (dist, a);
                        v.lineTo (p);
                        if (k == 1)
                        {
                            juce::Path br; br.startNewSubPath (p);
                            br.lineTo (c.getPointOnCircumference (dist * 0.85f, a + 0.35f));
                            g.setColour (juce::Colour (0xffc0202a).withAlpha (0.7f)); g.strokePath (br, stroke (0.5f));
                        }
                    }
                    g.setColour (juce::Colour (0xffc0202a).withAlpha (0.85f));
                    g.strokePath (v, stroke (0.75f));
                }
                // iris vert acide + pupille dilatée
                const auto ic = c.translated (lookX * rr, lookY * rr);
                const float ir = rr * 0.50f;
                juce::ColourGradient ig (kAcidHi, ic.translated (-ir * 0.3f, -ir * 0.3f), juce::Colour (0xff2c6a08), ic.translated (ir, ir), true);
                g.setGradientFill (ig);
                g.fillEllipse (juce::Rectangle<float> (ir * 2, ir * 2).withCentre (ic));
                g.setColour (juce::Colour (0xff1c4a04));
                for (int k = 0; k < 14; ++k)
                    g.drawLine ({ ic.getPointOnCircumference (ir * 0.55f, (float) k * 0.45f), ic.getPointOnCircumference (ir * 0.95f, (float) k * 0.45f) }, 0.5f * s);
                g.setColour (kInk);
                g.drawEllipse (juce::Rectangle<float> (ir * 2, ir * 2).withCentre (ic), 0.9f * s);
                g.fillEllipse (juce::Rectangle<float> (ir * 1.35f, ir * 1.35f).withCentre (ic));
                g.setColour (juce::Colours::white);
                g.fillEllipse (juce::Rectangle<float> (ir * 0.42f, ir * 0.42f).withCentre (ic.translated (-ir * 0.3f, -ir * 0.35f)));
                g.setColour (juce::Colours::white.withAlpha (0.6f));
                g.fillEllipse (juce::Rectangle<float> (ir * 0.18f, ir * 0.18f).withCentre (ic.translated (ir * 0.32f, ir * 0.25f)));

                // paupière supérieure lourde, inclinée (air mauvais)
                juce::Path lid;
                // corde à -0.22 r (paupières à moitié fermées), inclinée
                const auto rot = juce::AffineTransform::rotation (lidAngle, c.x, c.y);
                const auto l0 = c.translated (-rr * 1.3f, -rr * 0.22f).transformedBy (rot);
                const auto l1 = c.translated ( rr * 1.3f, -rr * 0.22f).transformedBy (rot);
                const auto mid = c.translated (0.0f, -rr * 0.22f).transformedBy (rot);
                lid.startNewSubPath (l0);
                lid.quadraticTo (mid.translated (0.0f, rr * 0.18f), l1);
                lid.lineTo (c.translated (rr * 1.6f, -rr * 1.8f).transformedBy (rot));
                lid.lineTo (c.translated (-rr * 1.6f, -rr * 1.8f).transformedBy (rot));
                lid.closeSubPath();
                g.setGradientFill (juce::ColourGradient (kSkinHi, c.translated (0, -rr), kSkin.darker (0.2f), c, false));
                g.fillPath (lid);
                g.setColour (kInk);
                juce::Path lidEdge; lidEdge.startNewSubPath (l0); lidEdge.quadraticTo (mid.translated (0, rr * 0.18f), l1);
                g.strokePath (lidEdge, stroke (2.4f));
                g.setColour (kInk.withAlpha (0.4f));
                g.strokePath (lidEdge, stroke (1.0f), juce::AffineTransform::translation (0, -rr * 0.22f));
                // paupière inférieure gonflée
                juce::Path low; low.addCentredArc (c.x, c.y + rr * 0.05f, rr * 0.92f, rr * 0.92f, 0.0f, 2.3f, 3.98f, true);
                g.setColour (kInk.withAlpha (0.7f)); g.strokePath (low, stroke (1.2f));
            }
            g.setColour (kInk);
            g.drawEllipse (ball, 2.6f * s);
            // sourcil froncé
            const auto rot = juce::AffineTransform::rotation (lidAngle * 1.3f, c.x, c.y);
            juce::Path brow;
            brow.startNewSubPath (c.translated (-rr * 1.15f, -rr * 1.05f).transformedBy (rot));
            brow.quadraticTo (c.translated (0.0f, -rr * 1.45f).transformedBy (rot), c.translated (rr * 1.15f, -rr * 1.1f).transformedBy (rot));
            g.setColour (kInk);     g.strokePath (brow, stroke (5.2f));
            g.setColour (kSkinLo);  g.strokePath (brow, stroke (2.6f));
        };
        eye (174, 24, 15.0f,  0.32f,  0.22f,  0.20f, 7);    // gauche : regard en coin, mauvais
        eye (230, 44, 12.5f, -0.28f, -0.28f,  0.28f, 13);   // droit : tombant, qui louche

        //======================================================================
        // Bouche : rictus en coin, dents cassées, bave d'acide
        {
            juce::Path mouth;
            mouth.startNewSubPath (P (194, 104));
            mouth.cubicTo (P (202, 99), P (214, 98), P (226, 92));     // lèvre sup (coin relevé à droite)
            mouth.cubicTo (P (226, 104), P (218, 116), P (206, 117));  // lèvre inf
            mouth.cubicTo (P (199, 117), P (195, 111), P (194, 104));
            mouth.closeSubPath();
            g.setColour (juce::Colour (0xff2a0808));
            g.fillPath (mouth);
            {
                juce::Graphics::ScopedSaveState ss (g);
                g.reduceClipRegion (mouth);
                g.setColour (juce::Colour (0xff8a1e2a));      // langue
                g.fillEllipse (R (208, 116, 22, 10));
                // dents du haut, de travers, une manquante
                const float tx[] = { 198, 204, 210, 222 };
                const float th[] = { 6, 8, 5, 7 };
                for (int i = 0; i < 4; ++i)
                {
                    juce::Path t;
                    const float y0 = 100.0f - (tx[i] - 194) * 0.22f;
                    t.addTriangle (P (tx[i] - 2.8f, y0 - 2), P (tx[i] + 2.8f, y0 - 2.5f), P (tx[i] + 0.5f, y0 + th[i]));
                    g.setColour (juce::Colour (0xffe9dca2)); g.fillPath (t);
                    g.setColour (kInk); g.strokePath (t, stroke (0.8f));
                }
                juce::Path t2; t2.addTriangle (P (212, 118), P (217, 117), P (215, 110));
                g.setColour (juce::Colour (0xffd8c88a)); g.fillPath (t2);
                g.setColour (kInk); g.strokePath (t2, stroke (0.8f));
            }
            g.setColour (kInk);
            g.strokePath (mouth, stroke (2.6f));
            // ride de rictus
            juce::Path sneer; sneer.startNewSubPath (P (222, 84)); sneer.quadraticTo (P (229, 88), P (228, 96));
            g.strokePath (sneer, stroke (1.6f));

            // bave d'acide qui pend
            juce::Path drool;
            drool.startNewSubPath (P (201, 114));
            drool.cubicTo (P (201, 124), P (199.5f, 132), P (200, 138));
            drool.cubicTo (P (196, 141), P (197, 148), P (201.5f, 148));
            drool.cubicTo (P (206, 148), P (207, 141), P (203, 138));
            drool.cubicTo (P (203, 131), P (204, 123), P (205.5f, 115));
            drool.closeSubPath();
            g.setColour (kAcid.withAlpha (0.3f)); g.strokePath (drool, stroke (4.0f));
            g.setColour (kInk); g.strokePath (drool, stroke (1.6f));
            g.setGradientFill (juce::ColourGradient (kAcidHi.withAlpha (0.95f), P (200, 115), kAcid, P (202, 146), false));
            g.fillPath (drool);
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.fillEllipse (R (199.8f, 142.5f, 1.6f, 2.6f));
        }

        // Rides du front (froncé)
        for (int i = 0; i < 3; ++i)
        {
            juce::Path f; f.startNewSubPath (P (198 + i * 5.0f, 70 + i * 2.0f)); f.quadraticTo (P (204 + i * 5.0f, 66 + i * 2.0f), P (209 + i * 5.0f, 71 + i * 2.0f));
            g.setColour (kInk.withAlpha (0.55f)); g.strokePath (f, stroke (1.2f));
        }
    }
}

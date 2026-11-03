#pragma once
// Maquettes « réalistes / dégoûtantes » du blob (style dessin animé adulte, créature originale).
// Accent = bave (filets depuis la bouche, gouttes qui coulent du corps, flaque).

namespace OctoBlob
{
    struct EyeDef { float x, y, r, lookX, lookY; };
    struct GrossStyle
    {
        const char* name;
        int numEyes; EyeDef eyes[3];
        float droop;          // paupières tombantes 0..1
        float mouthW, mouthY, mouthTilt;
        int teeth;            // nombre de dents du haut
        int warts, drools;
        bool veins, translucent, stubble;
        float sat, val;       // couleur plus terne / plus sale
    };

    inline const GrossStyle grossStyles[] = {
        { "1 - Gluant", 2, { { -0.33f, -0.30f, 0.22f, 0.1f, 0.2f }, { 0.27f, -0.36f, 0.27f, -0.1f, 0.25f }, {} },
          0.45f, 0.78f, 0.22f, 0.06f, 7, 7, 3, false, false, false, 0.85f, 0.85f },
        { "2 - Mutant", 3, { { -0.42f, -0.22f, 0.17f, 0.3f, -0.2f }, { 0.08f, -0.47f, 0.25f, 0.0f, 0.3f }, { 0.47f, -0.12f, 0.13f, -0.5f, 0.1f } },
          0.3f, 0.86f, 0.25f, -0.05f, 11, 15, 2, true, false, false, 0.8f, 0.8f },
        { "3 - Toxique", 1, { { 0.0f, -0.4f, 0.28f, 0.15f, 0.15f }, {}, {} },
          0.55f, 0.55f, 0.36f, 0.08f, 5, 0, 4, true, true, false, 0.9f, 0.95f },
        { "4 - Clochard", 2, { { -0.3f, -0.28f, 0.21f, -0.6f, 0.3f }, { 0.3f, -0.3f, 0.21f, 0.6f, -0.2f }, {} },
          0.62f, 0.62f, 0.24f, -0.14f, 6, 9, 3, false, false, true, 0.55f, 0.72f },
    };
    inline constexpr int numGrossStyles = 4;

    // filet de bave : part de p0, pend (gravité), se balance, se termine par une goutte
    inline void drawDrool (juce::Graphics& g, juce::Point<float> p0, float len, float width, float sway, juce::Colour tint, float R)
    {
        if (len < 1.5f) return;
        const auto p2 = p0.translated (sway, len);
        const auto c1 = p0.translated (sway * 0.1f + width * 0.8f, len * 0.35f);   // la corde se courbe
        const auto c2 = p2.translated (-sway * 0.4f - width, -len * 0.35f);
        const float bulb = width * (1.0f + std::min (1.2f, len / R));
        juce::Path rope;
        rope.startNewSubPath (p0);
        rope.cubicTo (c1, c2, p2);
        // corde épaisse en haut, fine au milieu : deux traits superposés
        g.setColour (tint.withAlpha (0.5f));
        g.strokePath (rope, juce::PathStrokeType (width * 1.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.fillEllipse (juce::Rectangle<float> (width * 2.6f, width * 1.8f).withCentre (p0.translated (0.0f, width * 0.4f)));
        g.setColour (kInk.withAlpha (0.45f));
        juce::Path outline;
        juce::PathStrokeType (width * 1.1f + 1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath (outline, rope);
        g.strokePath (outline, juce::PathStrokeType (0.9f));
        // goutte au bout
        auto drop = juce::Rectangle<float> (bulb * 2.0f, bulb * 2.4f).withCentre (p2.translated (0.0f, bulb * 0.6f));
        g.setColour (tint.withAlpha (0.72f));
        g.fillEllipse (drop);
        g.setColour (kInk.withAlpha (0.6f));
        g.drawEllipse (drop, 1.1f);
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.fillEllipse (juce::Rectangle<float> (bulb * 0.55f, bulb * 0.7f).withCentre (drop.getCentre().translated (-bulb * 0.4f, -bulb * 0.4f)));
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.strokePath (rope, juce::PathStrokeType (std::max (0.8f, width * 0.25f)), juce::AffineTransform::translation (-width * 0.25f, 0.0f));
    }


    //==========================================================================
    // Peau réaliste : ombrage pixel par pixel (relief, lumière, reflets mouillés, transparence)
    struct FleshNoise
    {
        static constexpr int N = 256;
        std::vector<float> fbm, lumps;
        FleshNoise() : fbm (N * N), lumps (N * N)
        {
            uint32_t sd = 991u;
            auto r = [&sd] { sd = sd * 1664525u + 1013904223u; return (float) (sd >> 8) / 16777216.0f; };
            // bruit de valeur tuilable, 5 octaves
            std::vector<float> acc (N * N, 0.0f);
            float amp = 1.0f, tot = 0.0f;
            for (int o = 0; o < 5; ++o)
            {
                const int G = 4 << o;
                std::vector<float> grid (G * G);
                for (auto& v : grid) v = r();
                for (int y = 0; y < N; ++y)
                    for (int x = 0; x < N; ++x)
                    {
                        const float gx = (float) x * G / N, gy = (float) y * G / N;
                        const int x0 = (int) gx, y0 = (int) gy;
                        float fx = gx - x0, fy = gy - y0;
                        fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
                        auto at = [&] (int xx, int yy) { return grid[(size_t) ((yy % G) * G + (xx % G))]; };
                        const float v = (at (x0, y0) * (1 - fx) + at (x0 + 1, y0) * fx) * (1 - fy) + (at (x0, y0 + 1) * (1 - fx) + at (x0 + 1, y0 + 1) * fx) * fy;
                        acc[(size_t) (y * N + x)] += v * amp;
                    }
                tot += amp; amp *= 0.5f;
            }
            for (int i = 0; i < N * N; ++i) fbm[(size_t) i] = acc[(size_t) i] / tot;
            // bosses (cellules de Voronoï inversées)
            const int P = 40; float px[P], py[P];
            for (int i = 0; i < P; ++i) { px[i] = r(); py[i] = r(); }
            for (int y = 0; y < N; ++y)
                for (int x = 0; x < N; ++x)
                {
                    const float u = (float) x / N, v = (float) y / N;
                    float f1 = 9.0f;
                    for (int i = 0; i < P; ++i)
                    {
                        float dx = std::abs (u - px[i]), dy = std::abs (v - py[i]);
                        dx = std::min (dx, 1 - dx); dy = std::min (dy, 1 - dy);
                        f1 = std::min (f1, dx * dx + dy * dy);
                    }
                    { const float l = std::max (0.0f, 1.0f - std::sqrt (f1) / 0.12f); lumps[(size_t) (y * N + x)] = l * l * (3.0f - 2.0f * l); }
                }
        }
        static float sample (const std::vector<float>& t, float u, float v)
        {
            u -= std::floor (u); v -= std::floor (v);
            const float fx = u * N, fy = v * N;
            const int x0 = (int) fx % N, y0 = (int) fy % N, x1 = (x0 + 1) % N, y1 = (y0 + 1) % N;
            const float ax = fx - std::floor (fx), ay = fy - std::floor (fy);
            return (t[(size_t) (y0 * N + x0)] * (1 - ax) + t[(size_t) (y0 * N + x1)] * ax) * (1 - ay)
                 + (t[(size_t) (y1 * N + x0)] * (1 - ax) + t[(size_t) (y1 * N + x1)] * ax) * ay;
        }
        static const FleshNoise& get() { static FleshNoise n; return n; }
    };

    // skin : 0 peau humide bosselée, 1 gelée translucide, 2 couche de mucus
    inline void drawFlesh (juce::Graphics& g, const Shape& s, Frame f, float nrm, const Sim& sim, int skin,
                           juce::Colour base, juce::Colour light, juce::Colour dark, float scale)
    {
        const auto& nz = FleshNoise::get();
        // contour en polaire autour du centre
        constexpr int B = 720;
        std::array<float, B> rb {};
        std::vector<juce::Point<float>> poly;
        for (int i = 0; i < 360; ++i)
        {
            const float th = juce::MathConstants<float>::twoPi * (float) i / 360.0f;
            poly.push_back (bodyPoint ({ std::cos (th), std::sin (th) }, s, nrm, f));
        }
        float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
        for (size_t i = 0; i < poly.size(); ++i)
        {
            const auto a = poly[i] - f.c, b = poly[(i + 1) % poly.size()] - f.c;
            minX = std::min (minX, poly[i].x); maxX = std::max (maxX, poly[i].x); minY = std::min (minY, poly[i].y); maxY = std::max (maxY, poly[i].y);
            for (int k = 0; k < 8; ++k)
            {
                const auto p = a + (b - a) * ((float) k / 8.0f);
                float ph = std::atan2 (p.y, p.x); if (ph < 0) ph += juce::MathConstants<float>::twoPi;
                const int bin = juce::jlimit (0, B - 1, (int) (ph / juce::MathConstants<float>::twoPi * B));
                rb[(size_t) bin] = std::max (rb[(size_t) bin], p.getDistanceFromOrigin());
            }
        }
        for (int pass = 0; pass < 3; ++pass)          // bouche les trous
            for (int i = 0; i < B; ++i) if (rb[(size_t) i] <= 0.0f) rb[(size_t) i] = std::max (rb[(size_t) ((i + B - 1) % B)], rb[(size_t) ((i + 1) % B)]);

        const auto box = juce::Rectangle<float>::leftTopRightBottom (minX, minY, maxX, maxY).expanded (3.0f).getSmallestIntegerContainer();
        const float ps = std::max (0.5f, scale);   // pixels physiques par unité logique (zoom, écran Retina)
        const int W = (int) std::ceil ((float) box.getWidth() * ps), Hh = (int) std::ceil ((float) box.getHeight() * ps);
        auto LX = [&] (int x) { return (float) box.getX() + ((float) x + 0.5f) / ps; };
        auto LY = [&] (int y) { return (float) box.getY() + ((float) y + 0.5f) / ps; };
        if (W <= 2 || Hh <= 2) return;
        std::vector<float> hgt ((size_t) (W * Hh), -1.0f), tval ((size_t) (W * Hh), 2.0f);
        const float R = f.R, ca = std::cos (sim.texRot * 0.5f), sa = std::sin (sim.texRot * 0.5f);
        const float bumpAmp = skin == 0 ? 0.22f : (skin == 1 ? 0.05f : 0.1f);
        for (int y = 0; y < Hh; ++y)
            for (int x = 0; x < W; ++x)
            {
                const float px = LX (x) - f.c.x, py = LY (y) - f.c.y;
                float ph = std::atan2 (py, px); if (ph < 0) ph += juce::MathConstants<float>::twoPi;
                const float fb = ph / juce::MathConstants<float>::twoPi * B;
                const int b0 = (int) fb % B, b1 = (b0 + 1) % B;
                const float rad = rb[(size_t) b0] + (rb[(size_t) b1] - rb[(size_t) b0]) * (fb - std::floor (fb));
                const float t = std::sqrt (px * px + py * py) / std::max (1.0f, rad);
                tval[(size_t) (y * W + x)] = t;
                if (t >= 1.0f) continue;
                // relief : dôme + bosses + fines rides, dans le repère du corps (tourne avec lui)
                const float u = (px * ca - py * sa) / R, v = (px * sa + py * ca) / R;
                const float dome = std::pow (1.0f - t * t, 0.6f);
                const float lump = FleshNoise::sample (nz.lumps, u * 0.42f + 0.3f, v * 0.42f + 0.7f);
                const float fine = FleshNoise::sample (nz.fbm, u * 0.9f, v * 0.9f);
                hgt[(size_t) (y * W + x)] = dome + bumpAmp * (lump * lump * 0.8f + fine * 0.4f) * std::min (1.0f, (1.0f - t) * 6.0f);
            }

        juce::Image img (juce::Image::ARGB, W, Hh, true);
        {
            juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
            const float Lx = -0.5f, Ly = -0.68f, Lz = 0.54f;
            const float Hx = Lx, Hy = Ly, Hz = Lz + 1.0f, hl = std::sqrt (Hx * Hx + Hy * Hy + Hz * Hz);
            const float br = base.getFloatRed(), bg = base.getFloatGreen(), bb2 = base.getFloatBlue();
            const float lr = light.getFloatRed(), lg = light.getFloatGreen(), lb = light.getFloatBlue();
            const float dr = dark.getFloatRed(), dg = dark.getFloatGreen(), db = dark.getFloatBlue();
            const float k = R * ps * 0.125f;     // pente du relief (même rendu à tous les zooms)
            for (int y = 0; y < Hh; ++y)
                for (int x = 0; x < W; ++x)
                {
                    const float h = hgt[(size_t) (y * W + x)];
                    if (h < 0.0f) continue;
                    auto H = [&] (int xx, int yy) { xx = juce::jlimit (0, W - 1, xx); yy = juce::jlimit (0, Hh - 1, yy);
                                                    const float v2 = hgt[(size_t) (yy * W + xx)]; return v2 < 0.0f ? 0.0f : v2; };
                    float nx = -(H (x + 1, y) - H (x - 1, y)) * k * 0.5f, ny = -(H (x, y + 1) - H (x, y - 1)) * k * 0.5f, nzv = 1.0f;
                    const float nl = std::sqrt (nx * nx + ny * ny + nzv * nzv); nx /= nl; ny /= nl; nzv /= nl;
                    const float diff = std::max (0.0f, nx * Lx + ny * Ly + nzv * Lz);
                    const float nh = std::max (0.0f, (nx * Hx + ny * Hy + nzv * Hz) / hl);
                    const float spec = std::pow (nh, skin == 1 ? 90.0f : 55.0f) * (skin == 0 ? 0.9f : 1.3f) + std::pow (nh, 10.0f) * 0.12f;
                    const float fres = std::pow (1.0f - nzv, 2.5f);
                    const float t = tval[(size_t) (y * W + x)];
                    const float thin = 1.0f - std::min (1.0f, h * 1.6f);          // bords fins : la lumière traverse
                    const float py = (LY (y) - f.c.y) / R;
                    const float ao = 1.0f - 0.35f * juce::jlimit (0.0f, 1.0f, py * 0.9f);  // dessous plus sombre
                    float r_, g_, b_;
                    // diffuse « chair » : de sombre (creux) à la couleur, puis clair
                    const float d2 = 0.18f + 0.95f * diff;
                    r_ = (dr + (br - dr) * std::min (1.0f, d2)) * ao; g_ = (dg + (bg - dg) * std::min (1.0f, d2)) * ao; b_ = (db + (bb2 - db) * std::min (1.0f, d2)) * ao;
                    if (d2 > 1.0f) { const float e = (d2 - 1.0f) * 1.5f; r_ += (lr - br) * e; g_ += (lg - bg) * e; b_ += (lb - bb2) * e; }
                    // diffusion sous la peau (bords lumineux et saturés)
                    const float sss = thin * (skin == 1 ? 0.85f : 0.45f);
                    r_ += br * sss * 0.6f; g_ += bg * sss * 0.6f; b_ += bb2 * sss * 0.6f;
                    // cavités entre les bosses
                    if (skin != 1)
                    {
                        const float u = ((LX (x) - f.c.x) * ca - (LY (y) - f.c.y) * sa) / R;
                        const float v = ((LX (x) - f.c.x) * sa + (LY (y) - f.c.y) * ca) / R;
                        const float lump = FleshNoise::sample (nz.lumps, u * 0.42f + 0.3f, v * 0.42f + 0.7f);
                        const float cav = (1.0f - std::min (1.0f, lump * 3.0f)) * 0.35f;
                        r_ *= 1.0f - cav; g_ *= 1.0f - cav; b_ *= 1.0f - cav;
                    }
                    // mucus : reflets étirés (couche liquide)
                    float extra = 0.0f;
                    if (skin == 2)
                    {
                        const float u = (LX (x) - f.c.x) / R, v = (LY (y) - f.c.y) / R;
                        const float streak = FleshNoise::sample (nz.fbm, u * 0.35f, v * 2.2f + sim.t * 0.05f);
                        extra = std::pow (juce::jlimit (0.0f, 1.0f, (streak - 0.55f) * 4.0f), 2.0f) * std::pow (nh, 6.0f) * 0.8f;
                    }
                    const float sp = spec + extra + fres * 0.25f;
                    r_ += sp; g_ += sp; b_ += sp * 0.95f;
                    float a = juce::jlimit (0.0f, 1.0f, (1.0f - t) * std::max (1.0f, R * ps * 0.9f));    // bord net, anticrénelé
                    if (skin == 1) a *= 0.82f + 0.18f * std::min (1.0f, h * 2.0f);
                    auto c = juce::Colour::fromFloatRGBA (juce::jlimit (0.0f, 1.0f, r_), juce::jlimit (0.0f, 1.0f, g_), juce::jlimit (0.0f, 1.0f, b_), a);
                    bd.setPixelColour (x, y, c);
                }
        }
        g.setOpacity (1.0f);   // l'image prend sinon l'opacité de la dernière couleur
        g.drawImageTransformed (img, juce::AffineTransform::scale (1.0f / ps).translated ((float) box.getX(), (float) box.getY()));
    }

    inline void drawGross (juce::Graphics& g, juce::Rectangle<float> area, const Sim& sim, const GrossStyle& st, int skin = -1, float scale = 1.0f)
    {
        const auto& s = sim.shape;
        const auto f = frameFor (area, s);
        const float R = f.R, rev = sim.rev, reso = sim.reso, acc = sim.acc;
        const float hue = hueFor (reso);
        const auto base  = juce::Colour::fromHSV (hue, 0.86f * st.sat, 0.85f * st.val, 1.0f);
        const auto light = juce::Colour::fromHSV (std::fmod (hue + 0.97f, 1.0f), 0.5f * st.sat, std::min (1.0f, 1.05f * st.val), 1.0f);
        const auto dark  = juce::Colour::fromHSV (std::fmod (hue + 0.06f, 1.0f), 0.95f, 0.3f * st.val, 1.0f);
        const auto slime = juce::Colour::fromHSV (0.24f, 0.45f, 0.95f, 1.0f).interpolatedWith (base, 0.25f);   // bave vert pâle
        uint32_t seed = 12345u;
        auto rnd = [&seed] { seed = seed * 1664525u + 1013904223u; return (float) (seed >> 8) / 16777216.0f; };

        auto body = membrane (s, f);
        const float nrm = shapeNorm (s);
        auto BP = [&] (juce::Point<float> p) { return bodyPoint (p, s, nrm, f); };

        // gouttes qui coulent du bas du corps (accent)
        for (int k = 0; k < 3; ++k)
        {
            const float th = juce::MathConstants<float>::pi * (0.3f + 0.2f * (float) k + 0.05f * std::sin (sim.t + (float) k));
            const auto e = BP ({ std::cos (th) * 0.97f, std::sin (th) * 0.97f });
            const float len = R * acc * (0.1f + 0.3f * (0.5f + 0.5f * std::sin (sim.t * 0.7f + (float) k * 2.1f)));
            drawDrool (g, e.translated (0.0f, -R * 0.04f), len, R * 0.06f, R * 0.03f * (float) (k - 1), base.interpolatedWith (slime, 0.5f), R);
        }

        // reverb : ondes qui s'éloignent et échos fantômes qui le suivent
        for (auto& rp : sim.ripples)
            if (rp.age >= 0.0f && rp.age < 1.0f && rev > 0.02f)
            {
                const float rr = R * (1.05f + 1.3f * rp.age);
                const juce::Point<float> c (area.getCentreX() + rp.c.x * R, area.getCentreY() + (0.12f + rp.c.y) * R);
                g.setColour (light.withAlpha (rev * (1.0f - rp.age) * 0.5f));
                g.drawEllipse (juce::Rectangle<float> (rr * 2.0f, rr * 1.7f).withCentre (c), 1.0f + 2.0f * (1.0f - rp.age));
            }
        if (rev > 0.02f)
            for (int i = 3; i >= 1; --i)
            {
                const auto& e = sim.echo ((int) std::round ((3.0f + 9.0f * rev) * (float) i));
                auto gp = membrane (e, frameFor (area, e), 1.0f + 0.05f * (float) i * rev);
                const float a2 = rev * (0.45f - 0.11f * (float) i);
                g.setColour (base.withAlpha (a2 * 0.5f));
                g.fillPath (gp);
                g.setColour (light.withAlpha (a2));
                g.strokePath (gp, juce::PathStrokeType (1.3f));
            }

        // halo humide
        g.setColour (base.withAlpha (0.14f + 0.1f * rev));
        g.strokePath (body, juce::PathStrokeType (R * (0.12f + 0.3f * rev)));

        if (skin >= 0)
        {
            drawFlesh (g, s, f, nrm, sim, skin, base, light, dark, scale);
            if (skin == 1)        // gelée : bulles et noyau qu'on voit à travers
            {
                juce::Graphics::ScopedSaveState ss (g);
                g.reduceClipRegion (body);
                for (int k = 0; k < 10; ++k)
                {
                    const auto vc = BP ({ rnd() * 1.3f - 0.65f, rnd() * 1.0f - 0.25f });
                    const float vr = R * (0.03f + 0.06f * rnd());
                    g.setColour (dark.withAlpha (0.3f));
                    g.fillEllipse (juce::Rectangle<float> (vr * 2.0f, vr * 2.0f).withCentre (vc));
                    g.setColour (juce::Colours::white.withAlpha (0.55f));
                    g.fillEllipse (juce::Rectangle<float> (vr * 0.6f, vr * 0.6f).withCentre (vc.translated (-vr * 0.35f, -vr * 0.35f)));
                }
            }
            g.setColour (dark.darker (0.6f).withAlpha (0.85f));
            g.strokePath (body, juce::PathStrokeType (std::max (1.2f, R * 0.025f)));
        }
        else
        {
        // corps
        {
            juce::ColourGradient gr (light, f.c.x - 0.4f * R, f.c.y - 0.55f * R, dark, f.c.x + 0.5f * R, f.c.y + 1.1f * R, true);
            gr.addColour (0.4, base);
            gr.multiplyOpacity (st.translucent ? 0.78f : 1.0f);
            g.setGradientFill (gr);
            g.fillPath (body);
        }
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (body);
            if (st.translucent)
            {
                // organes visibles : noyau, vacuoles, filaments
                const auto nc = BP ({ 0.12f, 0.25f });
                g.setColour (dark.withAlpha (0.55f));
                g.fillEllipse (juce::Rectangle<float> (R * 0.62f, R * 0.48f).withCentre (nc));
                g.setColour (juce::Colour::fromHSV (std::fmod (hue + 0.6f, 1.0f), 0.6f, 0.5f, 0.6f));
                g.fillEllipse (juce::Rectangle<float> (R * 0.22f, R * 0.2f).withCentre (nc.translated (R * 0.05f, -R * 0.03f)));
                for (int k = 0; k < 7; ++k)
                {
                    const auto vc = BP ({ rnd() * 1.4f - 0.7f, rnd() * 1.0f - 0.2f });
                    const float vr = R * (0.05f + 0.07f * rnd());
                    g.setColour (light.withAlpha (0.5f));
                    g.drawEllipse (juce::Rectangle<float> (vr * 2.0f, vr * 1.7f).withCentre (vc), 1.2f);
                    g.setColour (dark.withAlpha (0.25f));
                    g.fillEllipse (juce::Rectangle<float> (vr * 1.2f, vr).withCentre (vc));
                }
            }
            // cellules / pores
            const auto tf = juce::AffineTransform::scale (R * 1.5f / 256.0f).rotated (sim.texRot + s.twist * 0.6f).translated (f.c.x, f.c.y);
            juce::FillType dt (texture (false), tf); dt.setOpacity (st.translucent ? 0.15f : 0.26f);
            g.setFillType (dt); g.fillPath (body);
            juce::FillType lt (texture (true), juce::AffineTransform::scale (R * 0.9f / 256.0f).translated (f.c.x + 13.0f, f.c.y - 7.0f)); lt.setOpacity (0.25f);
            g.setFillType (lt); g.fillPath (body);
            for (int k = 0; k < 60; ++k)
            {
                const auto pc = BP ({ rnd() * 2.0f - 1.0f, rnd() * 2.0f - 1.0f });
                g.setColour (dark.withAlpha (0.35f));
                g.fillEllipse (juce::Rectangle<float> (R * 0.025f, R * 0.02f).withCentre (pc));
            }
            // veines
            if (st.veins)
                for (int v = 0; v < 5; ++v)
                {
                    juce::Path vp;
                    float x = rnd() * 1.4f - 0.7f, y = rnd() * 0.8f + 0.0f, a = rnd() * 6.28f;
                    vp.startNewSubPath (BP ({ x, y }));
                    for (int k = 0; k < 6; ++k)
                    {
                        a += (rnd() - 0.5f) * 1.6f;
                        x += std::cos (a) * 0.12f; y += std::sin (a) * 0.12f;
                        vp.lineTo (BP ({ x, y }));
                    }
                    g.setColour (juce::Colour::fromHSV (0.98f, 0.7f, 0.45f, 0.4f));
                    g.strokePath (vp, juce::PathStrokeType (R * 0.022f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                }
            // verrues
            for (int k = 0; k < st.warts; ++k)
            {
                const float ang = rnd() * 6.28f, rr = 0.35f + 0.55f * rnd();
                const auto wc = BP ({ std::cos (ang) * rr, std::sin (ang) * rr * 0.9f + 0.1f });
                const float wr = R * (0.035f + 0.05f * rnd());
                juce::ColourGradient wg (light.withAlpha (0.9f), wc.x - wr * 0.4f, wc.y - wr * 0.4f, dark.withAlpha (0.9f), wc.x + wr, wc.y + wr, true);
                g.setGradientFill (wg);
                g.fillEllipse (juce::Rectangle<float> (wr * 2.0f, wr * 1.8f).withCentre (wc));
                g.setColour (kInk.withAlpha (0.45f));
                g.drawEllipse (juce::Rectangle<float> (wr * 2.0f, wr * 1.8f).withCentre (wc), 0.9f);
            }
            if (st.stubble)
                for (int k = 0; k < 70; ++k)
                {
                    const auto sc = BP ({ rnd() * 1.1f - 0.55f, 0.2f + rnd() * 0.45f });
                    g.setColour (kInk.withAlpha (0.45f));
                    g.fillEllipse (juce::Rectangle<float> (1.6f, 1.6f).withCentre (sc));
                }
            // cœur chaud (drive) + ombre interne + reflet irisé
            if (sim.drv > 0.01f)
            {
                const auto hot = juce::Colour::fromHSV (0.04f, 0.9f, 1.0f, 1.0f);
                juce::ColourGradient hg (hot.withAlpha (0.55f * sim.drv), f.c.x, f.c.y + 0.25f * R, hot.withAlpha (0.0f), f.c.x, f.c.y + 1.0f * R, true);
                g.setGradientFill (hg); g.fillPath (body);
            }
            g.setColour (dark.withAlpha (0.55f));
            g.strokePath (body, juce::PathStrokeType (R * 0.26f));
            if (reso > 0.05f)
            {
                const auto iri = juce::Colour::fromHSV (std::fmod (hue + 0.5f, 1.0f), 0.6f, 1.0f, 1.0f);
                g.setColour (iri.withAlpha (0.4f * reso));
                g.strokePath (body, juce::PathStrokeType (R * 0.05f), juce::AffineTransform::translation (-0.04f * R, -0.05f * R));
            }
        }
        g.setColour (kInk);
        g.strokePath (body, juce::PathStrokeType (std::max (2.0f, R * 0.07f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // reflets mouillés : longue traînée + gouttelettes
        {
            juce::Path streak;
            streak.startNewSubPath (BP ({ -0.75f, -0.15f }));
            streak.quadraticTo (BP ({ -0.72f, -0.62f }), BP ({ -0.3f, -0.78f }));
            g.setColour (juce::Colours::white.withAlpha (0.7f));
            g.strokePath (streak, juce::PathStrokeType (R * 0.07f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            for (int k = 0; k < 9; ++k)
            {
                const auto dc = BP ({ rnd() * 1.5f - 0.75f, rnd() * 1.2f - 0.6f });
                const float dr = R * (0.015f + 0.03f * rnd());
                g.setColour (juce::Colours::white.withAlpha (0.8f));
                g.fillEllipse (juce::Rectangle<float> (dr * 2.0f, dr * 2.0f).withCentre (dc));
            }
        }

        }
        // gouttelettes projetées sur les 4 axes (notes)
        for (auto& d : sim.drops)
            if (d.life > 0.0f)
            {
                const juce::Point<float> c (area.getCentreX() + d.pos.x * R, area.getCentreY() + (0.12f + d.pos.y) * R);
                const float r = d.r * R * (0.6f + 0.4f * d.life);
                const float st = juce::jlimit (1.0f, 2.2f, 1.0f + d.vel.getDistanceFromOrigin() * 0.15f);
                const float ang = std::atan2 (d.vel.y, d.vel.x);
                juce::Path dp; dp.addEllipse (-r * st, -r, r * 2.0f * st, r * 2.0f);
                dp.applyTransform (juce::AffineTransform::rotation (ang).translated (c));
                const float al = juce::jlimit (0.0f, 1.0f, d.life * 1.5f);
                juce::ColourGradient dg (light.withAlpha (0.95f * al), c.x - r * 0.4f, c.y - r * 0.4f, dark.withAlpha (0.9f * al), c.x + r, c.y + r, true);
                dg.addColour (0.5, base.withAlpha (0.92f * al));
                g.setGradientFill (dg); g.fillPath (dp);
                g.setColour (dark.darker (0.6f).withAlpha (0.8f * al)); g.strokePath (dp, juce::PathStrokeType (std::max (1.0f, R * 0.02f)));
                g.setColour (juce::Colours::white.withAlpha (0.85f * al));
                g.fillEllipse (juce::Rectangle<float> (r * 0.5f, r * 0.5f).withCentre (c.translated (-r * 0.35f, -r * 0.35f)));
            }

        // yeux : globuleux, injectés de sang, paupières lourdes ; pupilles minuscules -> dilatées (cutoff)
        const float pk = juce::jlimit (0.0f, 1.0f, sim.pupil);
        for (int i = 0; i < st.numEyes; ++i)
        {
            const auto& ed = st.eyes[i];
            const auto ec = BP ({ ed.x, ed.y });
            const float er = R * ed.r;
            auto eye = juce::Rectangle<float> (er * 2.0f, er * 1.9f).withCentre (ec);
            // poche sous l'œil
            juce::Path bag; bag.addCentredArc (ec.x, ec.y + er * 0.25f, er * 1.05f, er * 1.0f, 0.0f, juce::MathConstants<float>::pi * 0.65f, juce::MathConstants<float>::pi * 1.35f, true);
            g.setColour (dark.withAlpha (0.7f));
            g.strokePath (bag, juce::PathStrokeType (R * 0.03f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (kInk); g.fillEllipse (eye.expanded (R * 0.035f));
            juce::ColourGradient sg (juce::Colour (0xfffbf6dc), ec.x - er * 0.3f, ec.y - er * 0.3f, juce::Colour (0xffd9c98e), ec.x + er, ec.y + er, true);
            g.setGradientFill (sg); g.fillEllipse (eye);
            {
                juce::Graphics::ScopedSaveState ss (g);
                juce::Path eyeClip; eyeClip.addEllipse (eye);
                g.reduceClipRegion (eyeClip);
                for (int k = 0; k < 6; ++k)            // vaisseaux
                {
                    const float a = rnd() * 6.28f;
                    juce::Path vp; auto p = ec + juce::Point<float> (std::cos (a), std::sin (a)) * er;
                    vp.startNewSubPath (p);
                    for (int j = 0; j < 3; ++j) { p = p + (ec - p) * 0.22f + juce::Point<float> ((rnd() - 0.5f) * er * 0.25f, (rnd() - 0.5f) * er * 0.25f); vp.lineTo (p); }
                    g.setColour (juce::Colour (0xffd0303a).withAlpha (0.7f));
                    g.strokePath (vp, juce::PathStrokeType (std::max (0.8f, er * 0.045f)));
                }
                const auto pc = ec + juce::Point<float> (ed.lookX + sim.gazeX * 0.3f, ed.lookY + sim.gazeY * 0.3f) * er * 0.45f;
                const float pr = er * (0.09f + 0.42f * pk * pk);
                g.setColour (kInk);
                g.fillEllipse (juce::Rectangle<float> (pr * 2.0f, pr * 2.0f).withCentre (pc));
                g.setColour (juce::Colours::white.withAlpha (0.85f));
                g.fillEllipse (juce::Rectangle<float> (std::max (1.2f, pr * 0.45f), std::max (1.2f, pr * 0.45f)).withCentre (pc.translated (-pr * 0.4f, -pr * 0.45f)));
                // paupière lourde
                const float lid = juce::jlimit (0.0f, 1.0f, st.droop + sim.blink * (1.0f - st.droop));
                const float ly = eye.getY() + eye.getHeight() * lid;
                if (skin >= 0)
                {
                    juce::ColourGradient lg2 (light, ec.x - er * 0.4f, eye.getY(), dark, ec.x + er * 0.5f, ly + er * 0.1f, false);
                    lg2.addColour (0.45, base);
                    g.setGradientFill (lg2);
                }
                else g.setColour (base.darker (0.15f));
                juce::Path lidPath;
                lidPath.startNewSubPath (eye.getX() - 2.0f, eye.getY() - 2.0f);
                lidPath.lineTo (eye.getRight() + 2.0f, eye.getY() - 2.0f);
                lidPath.lineTo (eye.getRight() + 2.0f, ly - er * 0.12f);
                lidPath.quadraticTo (ec.x, ly + er * 0.18f, eye.getX() - 2.0f, ly - er * 0.12f);
                lidPath.closeSubPath();
                g.fillPath (lidPath);
                juce::Path edge;
                edge.startNewSubPath (eye.getX(), ly - er * 0.12f);
                edge.quadraticTo (ec.x, ly + er * 0.18f, eye.getRight(), ly - er * 0.12f);
                g.setColour (kInk);
                g.strokePath (edge, juce::PathStrokeType (std::max (1.5f, R * 0.04f)));
            }
            if (skin >= 0)
            {
                juce::ColourGradient shade (juce::Colours::transparentBlack, ec.x - er * 0.3f, ec.y - er * 0.4f, juce::Colours::black.withAlpha (0.45f), ec.x + er * 0.6f, ec.y + er * 0.9f, true);
                g.setGradientFill (shade); g.fillEllipse (eye);
                g.setColour (juce::Colours::white.withAlpha (0.85f));
                g.fillEllipse (juce::Rectangle<float> (er * 0.35f, er * 0.22f).withCentre (ec.translated (-er * 0.38f, -er * 0.05f + er * 0.0f)));
            }
            g.setColour (skin >= 0 ? dark.darker (0.7f) : kInk);
            g.drawEllipse (eye, std::max (1.5f, R * (skin >= 0 ? 0.025f : 0.035f)));
        }

        // bouche : de travers, dents jaunes inégales, langue ; s'ouvre avec les notes et l'accent
        {
            const auto mc = BP ({ 0.02f, st.mouthY });
            const float mw = R * st.mouthW;
            const float open = R * (0.08f + 0.16f * sim.mouth + 0.12f * acc);
            const float tilt = st.mouthTilt * R;
            const juce::Point<float> L (mc.x - mw * 0.5f, mc.y + tilt), Rt (mc.x + mw * 0.5f, mc.y - tilt);
            juce::Path m;
            m.startNewSubPath (L);
            m.cubicTo (L.translated (mw * 0.25f, -R * 0.05f), Rt.translated (-mw * 0.3f, R * 0.04f), Rt);
            m.cubicTo (Rt.translated (-mw * 0.1f, open * 0.9f), L.translated (mw * 0.35f, open * 1.3f), L);
            m.closeSubPath();
            g.setColour (juce::Colour (0xff2a0a0e)); g.fillPath (m);
            {
                juce::Graphics::ScopedSaveState ss (g);
                g.reduceClipRegion (m);
                g.setColour (juce::Colour (0xffc2414f));       // langue
                g.fillEllipse (juce::Rectangle<float> (mw * 0.55f, open * 0.9f).withCentre ({ mc.x + mw * 0.06f, mc.y + open * 0.85f }));
                g.setColour (juce::Colour (0xff8c2433));
                g.drawLine (mc.x + mw * 0.06f, mc.y + open * 0.5f, mc.x + mw * 0.08f, mc.y + open * 1.1f, 1.2f);
                for (int k = 0; k < st.teeth; ++k)              // dents du haut
                {
                    if (k == st.teeth / 2 + 1) continue;          // il en manque une
                    const float t0 = ((float) k + 0.1f) / (float) st.teeth, t1 = ((float) k + 0.9f) / (float) st.teeth;
                    const float x0 = L.x + (Rt.x - L.x) * t0, x1 = L.x + (Rt.x - L.x) * t1;
                    const float yTop = L.y + (Rt.y - L.y) * (t0 + t1) * 0.5f - R * 0.04f;
                    const float h = R * (0.06f + 0.06f * rnd());
                    juce::Path tooth; tooth.addRoundedRectangle (x0, yTop, x1 - x0, h + R * 0.04f, 1.5f);
                    g.setColour (juce::Colour (0xffe9dc9a).interpolatedWith (juce::Colour (0xffb59a44), rnd() * 0.6f));
                    g.fillPath (tooth);
                    g.setColour (kInk.withAlpha (0.8f)); g.strokePath (tooth, juce::PathStrokeType (0.9f));
                }
            }
            g.setColour (kInk);
            g.strokePath (m, juce::PathStrokeType (std::max (1.8f, R * 0.05f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            // filets de bave (accent) depuis la lèvre du bas
            for (int k = 0; k < st.drools; ++k)
            {
                const float t = (0.2f + 0.6f * (float) k / (float) juce::jmax (1, st.drools - 1));
                const auto p0 = L + (Rt - L) * t + juce::Point<float> (0.0f, open * (0.55f + 0.45f * std::sin (t * 3.14159f)) * 1.0f);
                const float vary = 0.25f + 0.75f * std::abs (std::sin ((float) k * 2.3f + sim.t * 0.5f));
                const float len = R * (0.05f + acc * (k == 0 ? 1.05f : 0.55f) * vary);
                drawDrool (g, p0, len, R * (0.04f + 0.03f * acc), (k % 2 ? 1.0f : -1.0f) * R * 0.08f * acc - s.stretch * R * 0.8f, slime, R);
            }
        }
    }
}

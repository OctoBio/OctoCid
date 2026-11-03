#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>

//==============================================================================
// OctoBlob : mascotte organique (alternative à l'escargot).
//  - se balade dans tous les sens (errance + coups de pied à chaque note), saute sur
//    les accents, s'écrase / s'étire, mais garde TOUJOURS la même surface (pas de pics,
//    pas de changement de taille) ;
//  - Resonance : couleur (vert acide -> jaune -> orange -> rose) + reflets irisés ;
//  - Cutoff : déformation n°1 (fermé = mou, affaissé, lent ; ouvert = membrane nerveuse,
//    ondulations rapides) + pupilles qui se dilatent ;
//  - Reverb : déformation n°2 (échos fantômes qui le suivent, ondes, halo, translucide) ;
//  - Drive : bulles qui bouillonnent à l'intérieur, cœur qui chauffe ;
//  - Env Mod : les cellules tournent ; Decay : flaque de slime ; LFO : balancement.
//==============================================================================
namespace OctoBlob
{
    inline const juce::Colour kInk { 0xff1d1a17 };
    constexpr int kHarm = 6;          // harmoniques 2..7 de la membrane
    constexpr int kHist = 48;

    struct Inputs
    {
        float cutoff = 0.5f, reso = 0.0f, reverb = 0.0f, drive = 0.0f, envmod = 0.5f, decay = 0.3f, lfo = 0.0f, accent = 0.5f;
        bool hardDrive = false;
    };

    struct Shape   // ce qu'il faut pour redessiner la membrane (aussi pour les échos)
    {
        juce::Point<float> pos;
        float jump = 0.0f, squash = 0.0f, stretch = 0.0f, stretchAngle = 0.0f, sag = 0.0f, turb = 0.0f, time = 0.0f;
        float twist = 0.0f, jelly = 0.0f, jellyPhase = 0.0f, wave = 0.0f, wavePhase = 0.0f, dome = 1.12f;
        float splash = 0.0f;   // éclaboussure sur les 4 axes (note jouée)
        std::array<float, kHarm> amp {}, phase {};
    };

    //--------------------------------------------------------------------------
    class Sim
    {
    public:
        Sim() { for (int i = 0; i < kHarm; ++i) shape.phase[(size_t) i] = (float) i * 1.7f; }

        void noteOn (bool accent, bool slide)
        {
            const float a = rnd() * juce::MathConstants<float>::twoPi;
            const float kick = slide ? 0.08f : (accent ? 0.22f : 0.12f);   // presque sur place : il se déforme plutôt qu'il ne bouge
            vel += { std::cos (a) * kick, std::sin (a) * kick * 0.35f };
            squashVel += accent ? 7.0f : (slide ? 1.5f : 4.0f);
            for (int i = 0; i < kHarm; ++i) ampVel[(size_t) i] += (rnd() - 0.5f) * (accent ? 3.0f : 1.7f) / (1.0f + 0.3f * (float) i);
            jellyVel += (rnd() < 0.5f ? -1.0f : 1.0f) * (accent ? 4.0f : (slide ? 1.2f : 2.2f));
            // éclate comme une goutte d'eau : 4 lobes (haut, bas, gauche, droite) + gouttelettes
            splashVel += accent ? 5.5f : (slide ? 1.2f : 3.5f);
            if (! slide)
            {
                const int n = accent ? 8 : 4;
                const auto origin = shape.pos + juce::Point<float> (0.0f, shape.jump);
                for (int k = 0; k < n; ++k)
                {
                    const float a = juce::MathConstants<float>::halfPi * (float) (k % 4) + (k >= 4 ? 0.25f * (rnd() - 0.5f) : 0.12f * (rnd() - 0.5f));
                    const juce::Point<float> dir (std::cos (a), std::sin (a));
                    for (auto& d : drops)
                        if (d.life <= 0.0f)
                        {
                            const float sp = (accent ? 3.6f : 2.6f) * (0.75f + 0.5f * rnd()) * (k >= 4 ? 0.7f : 1.0f);
                            d = { origin + dir * 1.05f, dir * sp + juce::Point<float> (0.0f, -0.6f), 1.0f, (accent ? 0.11f : 0.08f) * (0.7f + 0.6f * rnd()) };
                            break;
                        }
                }
            }
            twistVel += (rnd() - 0.5f) * (accent ? 6.0f : 3.0f);
            if (accent && shape.jump > -0.01f) jumpVel = -0.9f;          // tout petit rebond
            mouth = std::max (mouth, accent ? 1.0f : 0.45f);
            for (auto& r : ripples) if (r.age < 0.0f || r.age > 1.0f) { r.age = 0.0f; r.c = shape.pos; r.c.y += shape.jump; break; }
        }

        void step (float dt, const Inputs& in)
        {
            dt = juce::jlimit (0.001f, 0.1f, dt);
            t += dt;
            shape.time = t;
            auto sm = [dt] (float& v, float target, float tau) { v += (target - v) * (1.0f - std::exp (-dt / tau)); };
            sm (acc, in.accent, 0.2f); sm (cut, in.cutoff, 0.12f); sm (reso, in.reso, 0.15f); sm (rev, in.reverb, 0.2f);
            sm (drv, in.drive, 0.2f);   sm (env, in.envmod, 0.3f); sm (dec, in.decay, 0.3f);
            hard = in.hardDrive;

            // Membrane : cutoff fermé = mou et lent, ouvert = nerveux (harmoniques hautes)
            static const float base[kHarm] = { 0.100f, 0.075f, 0.050f, 0.035f, 0.022f, 0.014f };
            float total = 0.0f;
            for (int i = 0; i < kHarm; ++i)
            {
                const int k = i + 2;
                float target = base[i] * (0.55f + 0.9f * cut);
                if (k >= 4) target += 0.05f * cut * cut * (float) (k - 3) / 4.0f;
                target *= 0.7f + 0.3f * std::sin (t * 0.6f * (float) k + (float) i);
                const float speed = (0.5f + 0.32f * (float) k) * (0.35f + 1.9f * cut) * ((i & 1) ? -1.0f : 1.0f);
                shape.phase[(size_t) i] += dt * speed;
                auto& a = shape.amp[(size_t) i];
                auto& av = ampVel[(size_t) i];
                av += ((target - a) * 70.0f - av * 7.0f) * dt;
                a += av * dt;
                total += std::abs (a);
            }
            if (total > 0.5f) for (auto& a : shape.amp) a *= 0.5f / total;      // jamais de pics
            // Cutoff fermé : il fond, s'étale et s'aplatit ; ouvert : dôme nerveux
            const float closed = (1.0f - cut) * (1.0f - cut);
            shape.sag = 0.02f + 0.34f * closed;
            shape.dome = 1.16f - 0.3f * closed;
            shape.turb = hard ? 0.028f * drv : 0.0f;

            // Gelée : cisaillement qui tremble (notes), plus vif quand le filtre s'ouvre
            jellyVel += (-55.0f * jellySpring - 2.6f * jellyVel) * dt;
            jellySpring += jellyVel * dt;
            shape.jellyPhase += dt * (5.0f + 6.0f * cut);
            shape.jelly = juce::jlimit (-0.45f, 0.45f, 0.05f + 0.12f * env + jellySpring * 0.12f);
            // Reverb : le corps se tord en spirale et ondule comme une flaque
            twistVel += (-30.0f * twistSpring - 3.0f * twistVel) * dt;
            twistSpring += twistVel * dt;
            shape.twist = juce::jlimit (-1.2f, 1.2f, rev * 0.85f * std::sin (t * 0.9f) + twistSpring * 0.15f);
            shape.wave = 0.045f * rev;
            splashVel += (-260.0f * splashSpring - 14.0f * splashVel) * dt;     // ressort vif : éclate puis se referme vite
            splashSpring = juce::jlimit (-0.2f, 0.32f, splashSpring + splashVel * dt);
            shape.splash = splashSpring;
            for (auto& d : drops)
                if (d.life > 0.0f)
                {
                    d.vel.y += 9.0f * dt;                    // gravité
                    d.vel *= std::exp (-dt * 1.2f);
                    d.pos += d.vel * dt;
                    d.life -= dt / 0.7f;
                }
            shape.wavePhase += dt * (2.0f + 4.0f * rev);

            // Presque fixe : il ondule légèrement sur place (ce sont les déformations qui vivent)
            const juce::Point<float> target (0.05f * std::sin (t * 0.31f) + 0.03f * std::sin (t * 0.77f + 2.0f) + 0.06f * in.lfo,
                                             0.03f * std::sin (t * 0.53f + 1.0f));
            vel += ((target - shape.pos) * 3.0f - vel * 1.9f) * dt;
            shape.pos += vel * dt;
            auto bounce = [this] (float& p, float& v, float lim)
            {
                if (p > lim)  { p = lim;  v = -std::abs (v) * 0.5f; squashVel -= 2.0f; }
                if (p < -lim) { p = -lim; v =  std::abs (v) * 0.5f; squashVel -= 2.0f; }
            };
            bounce (shape.pos.x, vel.x, 0.12f);
            bounce (shape.pos.y, vel.y, 0.05f);

            // Saut (accents)
            if (shape.jump < 0.0f || jumpVel < 0.0f)
            {
                jumpVel += 20.0f * dt;
                shape.jump += jumpVel * dt;
                if (shape.jump >= 0.0f) { shape.jump = 0.0f; if (jumpVel > 1.0f) squashVel += jumpVel * 0.9f; jumpVel = 0.0f; }
            }

            // Écrasement (ressort amorti), étirement dans le sens du mouvement
            squashVel += (-130.0f * shape.squash - 9.0f * squashVel) * dt;
            shape.squash = juce::jlimit (-0.32f, 0.32f, shape.squash + squashVel * dt);
            const float speed = vel.getDistanceFromOrigin() + std::abs (jumpVel) * 0.5f;
            shape.stretch = juce::jlimit (0.0f, 0.05f, speed * 0.05f);
            if (speed > 0.05f) shape.stretchAngle = std::atan2 (vel.y + jumpVel * 0.5f, vel.x);

            hist[(size_t) (histPos = (histPos + 1) % kHist)] = shape;
            if (histCount < kHist) ++histCount;

            for (auto& r : ripples) if (r.age >= 0.0f) r.age += dt / (0.6f + 1.6f * rev);

            // Yeux
            sm (pupil, in.cutoff, 0.1f);
            sm (gazeX, juce::jlimit (-1.0f, 1.0f, vel.x * 0.6f), 0.15f);
            sm (gazeY, juce::jlimit (-1.0f, 1.0f, vel.y * 0.6f + jumpVel * 0.15f), 0.15f);
            blinkT -= dt;
            if (blinkT < -0.16f) blinkT = 2.0f + 3.0f * rnd();
            blink = blinkT < 0.0f ? std::sin (juce::MathConstants<float>::pi * (-blinkT / 0.16f)) : 0.0f;
            mouth *= std::exp (-dt * 5.0f);

            // Bulles (drive) et cellules (env mod)
            texRot += dt * (0.04f + 0.45f * env);
            texDrift += { dt * 0.05f * std::sin (t * 0.2f), dt * 0.04f };
            bubbleAcc += dt * drv * 14.0f;
            for (auto& b : bubbles)
            {
                if (b.life <= 0.0f && bubbleAcc >= 1.0f)
                {
                    bubbleAcc -= 1.0f;
                    b = { (rnd() - 0.5f) * 1.0f, 0.55f, 0.04f + 0.06f * rnd(), 0.35f + 0.6f * rnd() * (0.5f + drv), 1.0f };
                }
                if (b.life > 0.0f) { b.y -= b.speed * dt; b.x += std::sin (t * 4.0f + b.r * 50.0f) * 0.1f * dt; if (b.y < -0.65f) b.life = 0.0f; }
            }
            if (bubbleAcc > 2.0f) bubbleAcc = 2.0f;
        }

        const Shape& echo (int framesAgo) const
        {
            framesAgo = juce::jlimit (0, juce::jmax (0, histCount - 1), framesAgo);
            return hist[(size_t) ((histPos - framesAgo + kHist) % kHist)];
        }

        Shape shape;
        float t = 0.0f, acc = 0.5f, cut = 0.5f, reso = 0.0f, rev = 0.0f, drv = 0.0f, env = 0.5f, dec = 0.3f;
        bool hard = false;
        float pupil = 0.5f, gazeX = 0.0f, gazeY = 0.0f, blink = 0.0f, mouth = 0.0f;
        float texRot = 0.0f;
        juce::Point<float> texDrift;
        struct Ripple { float age = -1.0f; juce::Point<float> c; };
        std::array<Ripple, 4> ripples {};
        struct Bubble { float x = 0, y = 0, r = 0, speed = 0, life = 0; };
        struct Drop { juce::Point<float> pos, vel; float life = 0.0f, r = 0.0f; };   // gouttelettes projetées
        std::array<Drop, 16> drops {};
        std::array<Bubble, 10> bubbles {};

    private:
        float rnd() { seed = seed * 1664525u + 1013904223u; return (float) (seed >> 8) / 16777216.0f; }
        uint32_t seed = 0x1234567u;
        juce::Point<float> vel;
        float jumpVel = 0.0f, squashVel = 0.0f, blinkT = 1.5f, bubbleAcc = 0.0f;
        float jellySpring = 0.0f, jellyVel = 0.0f, twistSpring = 0.0f, twistVel = 0.0f, splashSpring = 0.0f, splashVel = 0.0f;
        std::array<float, kHarm> ampVel {};
        std::array<Shape, kHist> hist {};
        int histPos = 0, histCount = 0;
    };

    //--------------------------------------------------------------------------
    // Texture organique (cellules / vacuoles), calculée une fois, tuilable
    inline const juce::Image& texture (bool light)
    {
        static juce::Image dark, bright;
        if (! dark.isValid())
        {
            const int N = 256, P = 26;
            dark = juce::Image (juce::Image::ARGB, N, N, true);
            bright = juce::Image (juce::Image::ARGB, N, N, true);
            float px[P], py[P], pr[P];
            uint32_t s = 77u;
            auto r = [&s] { s = s * 1664525u + 1013904223u; return (float) (s >> 8) / 16777216.0f; };
            for (int i = 0; i < P; ++i) { px[i] = r(); py[i] = r(); pr[i] = 0.8f + 0.4f * r(); }
            for (int y = 0; y < N; ++y)
                for (int x = 0; x < N; ++x)
                {
                    const float u = (float) x / N, v = (float) y / N;
                    float f1 = 9.0f, f2 = 9.0f;
                    for (int i = 0; i < P; ++i)
                    {
                        float dx = std::abs (u - px[i]), dy = std::abs (v - py[i]);
                        dx = std::min (dx, 1.0f - dx); dy = std::min (dy, 1.0f - dy);
                        const float d = std::sqrt (dx * dx + dy * dy) * pr[i];
                        if (d < f1) { f2 = f1; f1 = d; } else if (d < f2) f2 = d;
                    }
                    const float edge = juce::jlimit (0.0f, 1.0f, (f2 - f1) / 0.10f);
                    const float speck = (std::sin (u * 157.0f + std::sin (v * 91.0f) * 3.0f) * std::sin (v * 143.0f + std::sin (u * 73.0f) * 3.0f));
                    const float da = juce::jlimit (0.0f, 1.0f, (1.0f - edge) * 0.85f + (speck > 0.92f ? 0.35f : 0.0f) + f1 * 0.8f);
                    const float c = juce::jlimit (0.0f, 1.0f, 1.0f - f1 / 0.13f);
                    const float la = juce::jlimit (0.0f, 1.0f, c * c * 0.6f + (speck < -0.95f ? 0.5f : 0.0f));
                    dark.setPixelAt (x, y, juce::Colour::fromFloatRGBA (0.02f, 0.03f, 0.05f, da));
                    bright.setPixelAt (x, y, juce::Colour::fromFloatRGBA (1.0f, 1.0f, 0.95f, la));
                }
        }
        return light ? bright : dark;
    }

    //--------------------------------------------------------------------------
    struct Frame { juce::Point<float> c; float R; };

    // transforme un point local (rayon 1) selon écrasement / étirement
    inline juce::Point<float> deform (juce::Point<float> p, const Shape& s)
    {
        p.x *= 1.0f + s.squash; p.y /= 1.0f + s.squash;
        const float ca = std::cos (s.stretchAngle), sa = std::sin (s.stretchAngle);
        const float along = p.x * ca + p.y * sa, perp = -p.x * sa + p.y * ca;
        const float a2 = along * (1.0f + s.stretch), p2 = perp / (1.0f + s.stretch);
        return { a2 * ca - p2 * sa, a2 * sa + p2 * ca };
    }

    inline Frame frameFor (juce::Rectangle<float> area, const Shape& s)
    {
        const float R = std::min (area.getWidth() * 0.2f, area.getHeight() * 0.29f);
        return { { area.getCentreX() + s.pos.x * R, area.getCentreY() + (0.12f + s.pos.y + s.jump) * R }, R };
    }

    // Déformation du corps (bord ET intérieur : yeux, bouche, bulles suivent la chair).
    // Toutes ces déformations sont lisses ; la surface est ensuite remise à pi (norm).
    inline juce::Point<float> bodyRaw (juce::Point<float> p, const Shape& s)
    {
        const float r = std::sqrt (p.x * p.x + p.y * p.y);
        if (r > 1.0e-5f)
        {
            const float th = std::atan2 (p.y, p.x);
            float rf = 0.0f;
            for (int k = 0; k < kHarm; ++k) rf += s.amp[(size_t) k] * std::cos ((float) (k + 2) * th + s.phase[(size_t) k]);
            rf += s.turb * std::sin (9.0f * th + s.time * 7.0f);
            rf += s.wave * std::sin (8.0f * th - s.wavePhase);
            if (s.splash != 0.0f)   // 4 lobes arrondis sur les axes (haut, bas, gauche, droite)
            {
                const float c = 0.5f + 0.5f * std::cos (4.0f * th);
                rf += s.splash * (c * c * 1.25f - 0.45f);
            }
            p *= 1.0f + rf * std::min (1.0f, r);
        }
        float x = p.x, y = p.y;
        if (y > 0.0f) { const float k = std::min (1.0f, y); y *= 1.0f - s.sag; x *= 1.0f + s.sag * 0.7f * k; }   // s'affaisse, s'étale
        else y *= s.dome;                                                                                       // dôme
        // torsion en spirale (reverb) : rotation qui croît avec le rayon (conserve la surface)
        const float a = s.twist * (x * x + y * y) * 0.6f, ca = std::cos (a), sa = std::sin (a);
        const float tx = x * ca - y * sa, ty = x * sa + y * ca;
        x = tx; y = ty;
        // gelée : deux cisaillements qui tremblent (conservent la surface)
        x += s.jelly * 0.32f * std::sin (2.3f * y + s.jellyPhase);
        y += s.jelly * 0.18f * std::sin (2.0f * x - s.jellyPhase * 1.3f + 1.0f);
        return { x, y };
    }

    inline float shapeNorm (const Shape& s)
    {
        constexpr int N = 90;
        double area2 = 0.0;
        juce::Point<float> prev = bodyRaw ({ 1.0f, 0.0f }, s), first = prev;
        for (int i = 1; i <= N; ++i)
        {
            const float th = juce::MathConstants<float>::twoPi * (float) i / (float) N;
            const auto q = i == N ? first : bodyRaw ({ std::cos (th), std::sin (th) }, s);
            area2 += (double) (prev.x * q.y - q.x * prev.y);
            prev = q;
        }
        return std::sqrt (juce::MathConstants<float>::pi / (float) std::max (0.1, std::abs (area2) * 0.5));   // surface constante
    }

    // point du corps -> écran
    inline juce::Point<float> bodyPoint (juce::Point<float> p, const Shape& s, float norm, Frame f, float grow = 1.0f)
    {
        return f.c + deform (bodyRaw (p, s) * norm, s) * (f.R * grow);
    }

    inline juce::Path membrane (const Shape& s, Frame f, float grow = 1.0f)
    {
        constexpr int N = 96;
        const float norm = shapeNorm (s);
        std::array<juce::Point<float>, N> pts;
        for (int i = 0; i < N; ++i)
        {
            const float th = juce::MathConstants<float>::twoPi * (float) i / (float) N;
            pts[(size_t) i] = bodyPoint ({ std::cos (th), std::sin (th) }, s, norm, f, grow);
        }
        juce::Path p;
        auto P = [&] (int i) { return pts[(size_t) ((i + N) % N)]; };
        auto mid = [] (juce::Point<float> a, juce::Point<float> b) { return (a + b) * 0.5f; };
        p.startNewSubPath (mid (P (N - 1), P (0)));
        for (int i = 0; i < N; ++i) p.quadraticTo (P (i), mid (P (i), P (i + 1)));
        p.closeSubPath();
        return p;
    }

    inline float hueFor (float reso)
    {
        float h = 0.27f - 0.36f * reso;      // vert acide -> jaune -> orange -> rouge -> rose
        return h < 0.0f ? h + 1.0f : h;
    }

    //--------------------------------------------------------------------------
    inline void draw (juce::Graphics& g, juce::Rectangle<float> area, const Sim& sim)
    {
        const auto& s = sim.shape;
        const auto f = frameFor (area, s);
        const float R = f.R, rev = sim.rev, reso = sim.reso;
        const float hue = hueFor (reso);
        const auto base  = juce::Colour::fromHSV (hue, 0.86f, 0.93f, 1.0f);
        const auto light = juce::Colour::fromHSV (std::fmod (hue + 0.97f, 1.0f), 0.45f, 1.0f, 1.0f);
        const auto dark  = juce::Colour::fromHSV (std::fmod (hue + 0.05f, 1.0f), 0.95f, 0.38f, 1.0f);
        const float groundY = area.getCentreY() + (0.12f + s.pos.y + 0.92f) * R;

        // Flaque (decay) et ombre
        {
            const float w = R * (0.9f + 0.7f * sim.dec);
            g.setColour (dark.withAlpha (0.45f));
            g.fillEllipse (juce::Rectangle<float> (w * 2.0f, R * 0.32f).withCentre ({ f.c.x - s.pos.x * R * 0.1f, groundY }));
            g.setColour (base.withAlpha (0.35f));
            g.fillEllipse (juce::Rectangle<float> (w * 1.7f, R * 0.18f).withCentre ({ f.c.x, groundY - R * 0.03f }));
            const float j = juce::jlimit (0.0f, 1.0f, -s.jump * 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.35f * (1.0f - 0.5f * j)));
            g.fillEllipse (juce::Rectangle<float> (R * 1.9f * (1.0f - 0.35f * j), R * 0.22f).withCentre ({ f.c.x, groundY + R * 0.02f }));
        }

        // Reverb : ondes qui s'éloignent
        for (auto& rp : sim.ripples)
            if (rp.age >= 0.0f && rp.age < 1.0f && rev > 0.02f)
            {
                const float rr = R * (1.05f + 1.3f * rp.age);
                const juce::Point<float> c (area.getCentreX() + rp.c.x * R, area.getCentreY() + (0.12f + rp.c.y) * R);
                g.setColour (light.withAlpha (rev * (1.0f - rp.age) * 0.55f));
                g.drawEllipse (juce::Rectangle<float> (rr * 2.0f, rr * 1.7f).withCentre (c), 1.0f + 2.0f * (1.0f - rp.age));
            }

        // Reverb : échos fantômes qui le suivent
        if (rev > 0.02f)
            for (int i = 3; i >= 1; --i)
            {
                const auto& e = sim.echo ((int) std::round ((3.0f + 9.0f * rev) * (float) i));
                auto gp = membrane (e, frameFor (area, e), 1.0f + 0.05f * (float) i * rev);
                const float a = rev * (0.5f - 0.12f * (float) i);
                g.setColour (base.withAlpha (a * 0.55f));
                g.fillPath (gp);
                g.setColour (light.withAlpha (a));
                g.strokePath (gp, juce::PathStrokeType (1.4f));
            }

        auto body = membrane (s, f);
        const float nrm = shapeNorm (s);
        auto BP = [&] (juce::Point<float> p) { return bodyPoint (p, s, nrm, f); };

        // halo (plus large et brumeux avec la reverb)
        g.setColour (base.withAlpha (0.16f + 0.1f * rev));
        g.strokePath (body, juce::PathStrokeType (R * (0.18f + 0.35f * rev)));
        g.setColour (base.withAlpha (0.22f));
        g.strokePath (body, juce::PathStrokeType (R * 0.08f));

        // corps : dégradé (lumière en haut à gauche), un peu translucide avec la reverb
        {
            juce::ColourGradient gr (light, f.c.x - 0.35f * R, f.c.y - 0.45f * R, dark, f.c.x + 0.4f * R, f.c.y + 1.1f * R, true);
            gr.addColour (0.45, base);
            gr.multiplyOpacity (1.0f - 0.28f * rev);
            g.setGradientFill (gr);
            g.fillPath (body);
        }

        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (body);
            // cellules qui tournent (env mod)
            const auto tf = juce::AffineTransform::scale (R * 1.7f / 256.0f)
                                .rotated (sim.texRot + s.twist * 0.6f)
                                .translated (f.c.x + sim.texDrift.x * R, f.c.y + sim.texDrift.y * R);
            juce::FillType dt (texture (false), tf); dt.setOpacity (0.20f * (1.0f - 0.4f * rev));
            g.setFillType (dt); g.fillPath (body);
            const auto tf2 = juce::AffineTransform::scale (R * 1.1f / 256.0f).rotated (-sim.texRot * 0.6f + 1.0f)
                                 .translated (f.c.x + 37.0f, f.c.y - 11.0f);
            juce::FillType lt (texture (true), tf2); lt.setOpacity (0.32f);
            g.setFillType (lt); g.fillPath (body);

            // cœur qui chauffe (drive)
            if (sim.drv > 0.01f)
            {
                const auto hot = juce::Colour::fromHSV (0.04f, 0.9f, 1.0f, 1.0f);
                juce::ColourGradient hg (hot.withAlpha (0.65f * sim.drv), f.c.x, f.c.y + 0.25f * R, hot.withAlpha (0.0f), f.c.x, f.c.y + 1.0f * R, true);
                g.setGradientFill (hg); g.fillPath (body);
            }
            // bulles (drive)
            for (auto& b : sim.bubbles)
                if (b.life > 0.0f)
                {
                    const auto bp = BP ({ b.x, b.y });
                    const float br = b.r * R;
                    g.setColour (light.withAlpha (0.45f));
                    g.drawEllipse (juce::Rectangle<float> (br * 2.0f, br * 2.0f).withCentre (bp), 1.2f);
                    g.setColour (juce::Colours::white.withAlpha (0.7f));
                    g.fillEllipse (juce::Rectangle<float> (br * 0.6f, br * 0.6f).withCentre (bp.translated (-br * 0.35f, -br * 0.35f)));
                }
            // ombre interne du bord + reflet irisé (résonance)
            g.setColour (dark.withAlpha (0.45f));
            g.strokePath (body, juce::PathStrokeType (R * 0.22f));
            if (reso > 0.05f)
            {
                const auto iri = juce::Colour::fromHSV (std::fmod (hue + 0.5f + 0.15f * std::sin (sim.t * 2.0f), 1.0f), 0.7f, 1.0f, 1.0f);
                g.setColour (iri.withAlpha (0.55f * reso));
                g.strokePath (body, juce::PathStrokeType (R * 0.07f), juce::AffineTransform::translation (-0.04f * R, -0.05f * R));
            }
        }

        g.setColour (kInk.withAlpha (0.9f - 0.35f * rev));
        g.strokePath (body, juce::PathStrokeType (std::max (1.6f, R * 0.055f)));

        // reflet spéculaire
        {
            const auto hc = BP ({ -0.42f, -0.5f });
            juce::ColourGradient sg (juce::Colours::white.withAlpha (0.75f), hc.x, hc.y, juce::Colours::white.withAlpha (0.0f), hc.x + 0.3f * R, hc.y, true);
            g.setGradientFill (sg);
            g.fillEllipse (juce::Rectangle<float> (0.6f * R, 0.36f * R).withCentre (hc));
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillEllipse (juce::Rectangle<float> (0.09f * R, 0.09f * R).withCentre (hc.translated (0.12f * R, 0.08f * R)));
        }

        // yeux (pupilles : cutoff)
        const float pk = juce::jlimit (0.0f, 1.0f, sim.pupil);
        for (int e = 0; e < 2; ++e)
        {
            const juce::Point<float> local (e == 0 ? -0.34f : 0.3f, e == 0 ? -0.22f : -0.3f);
            const auto ec = BP (local);
            const float er = R * (e == 0 ? 0.27f : 0.3f);
            auto eye = juce::Rectangle<float> (er * 2.0f, er * 2.0f).withCentre (ec);
            g.setColour (kInk); g.fillEllipse (eye.expanded (R * 0.04f));
            g.setColour (juce::Colour (0xfffbf8ee)); g.fillEllipse (eye);
            const float ir = er * (0.55f + 0.42f * pk);
            const auto ic = ec + juce::Point<float> (sim.gazeX, sim.gazeY) * (er - ir) * 0.9f;
            const auto iris = juce::Colour::fromHSV (std::fmod (hue + 0.55f, 1.0f), 0.8f, 0.75f, 1.0f);
            g.setColour (iris); g.fillEllipse (juce::Rectangle<float> (ir * 2.0f, ir * 2.0f).withCentre (ic));
            const float pr = ir * (0.2f + 0.78f * pk * pk);
            g.setColour (kInk); g.fillEllipse (juce::Rectangle<float> (pr * 2.0f, pr * 2.0f).withCentre (ic));
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.fillEllipse (juce::Rectangle<float> (er * 0.36f, er * 0.36f).withCentre (ic.translated (-er * 0.3f, -er * 0.32f)));
            if (sim.blink > 0.01f)
            {
                juce::Graphics::ScopedSaveState ss (g);
                g.reduceClipRegion (eye.toNearestInt());
                g.setColour (base.darker (0.25f));
                g.fillRect (eye.withHeight (eye.getHeight() * sim.blink));
                g.setColour (kInk);
                g.drawHorizontalLine ((int) (eye.getY() + eye.getHeight() * sim.blink), eye.getX(), eye.getRight());
            }
        }

        // bouche (s'ouvre sur les notes, grande sur les accents)
        {
            const auto mc = BP ({ -0.02f, 0.18f });
            const float mw = R * 0.36f, mo = sim.mouth;
            if (mo > 0.08f)
            {
                auto m = juce::Rectangle<float> (mw * (0.8f + 0.3f * mo), R * 0.28f * mo + 2.0f).withCentre (mc.translated (0.0f, R * 0.06f * mo));
                g.setColour (kInk); g.fillEllipse (m);
                g.setColour (juce::Colour (0xffff5f8f));
                g.fillEllipse (m.withTrimmedTop (m.getHeight() * 0.55f).reduced (m.getWidth() * 0.2f, 0.0f));
            }
            else
            {
                juce::Path sm;
                sm.startNewSubPath (mc.x - mw * 0.5f, mc.y);
                sm.quadraticTo (mc.x, mc.y + R * 0.12f, mc.x + mw * 0.5f, mc.y - R * 0.02f);
                g.setColour (kInk);
                g.strokePath (sm, juce::PathStrokeType (std::max (1.5f, R * 0.05f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
    }
}
#include "BlobGross.h"

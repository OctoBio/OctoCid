#pragma once

#include "DspUtils.h"
#include "Filters.h"
#include "Oscillator.h"
#include "DiodeLadder.h"
#include "NoteStack.h"

namespace octocid
{
    //==========================================================================
    // Paramètres du panneau principal, normalisés (sauf tune en demi-tons)
    //==========================================================================
    struct VoiceParams
    {
        float tune      = 0.0f;   // -12..+12 demi-tons
        float cutoff    = 0.5f;   // 0..1
        float resonance = 0.5f;   // 0..1
        float envMod    = 0.5f;   // 0..1
        float decay     = 0.5f;   // 0..1
        float accent    = 0.5f;   // 0..1
        float squareMix = 0.0f;   // 0 = saw, 1 = square

        // Mods Devil Fish (tiroir Slime Mods)
        float slideTimeMs = 60.0f;   // 60 (303 d'origine) .. 360 ms
        int   sweepSpeed  = 1;       // 0 = Slow, 1 = Normal, 2 = Fast
        float slidePunch  = 0.0f;    // 0 = fidèle (glide sans relance) .. 1 = relance complète
    };

    //==========================================================================
    // Voix acid monophonique.
    //
    // Calibrage (plages des potards, enveloppes, accent, courbe de résonance,
    // filtres d'entrée/sortie) repris des mesures du projet Open303
    // (c) 2009 Robin Schmidt, licence MIT — voir THIRD_PARTY.md.
    //
    //  - MEG (enveloppe de filtre) : attaque ~3 ms, constante de décroissance
    //    200..2000 ms (200 ms sur les notes accentuées).
    //  - Cutoff : 314..2394 Hz, profondeur et décalage de l'enveloppe selon la
    //    loi mesurée sur l'original (l'Env Mod abaisse aussi la base).
    //  - VCA : décroissance lente (1,23 s) tant que la note est tenue, et le MEG
    //    s'ajoute au volume : c'est le « punch » de la 303, énorme sur l'accent.
    //  - Accent : circuit « accent sweep » à condensateur. Il se charge plus
    //    lentement que le MEG ne retombe et se vide lentement : des accents
    //    rapprochés s'accumulent (le « wow » qui monte). Sweep Speed règle la
    //    vitesse du condensateur, comme le mod Devil Fish.
    //  - Slide : notes chevauchées -> glissement RC en fréquence, 60..360 ms.
    //    Slide Punch (moderne) relance enveloppe de filtre et volume sur chaque
    //    note glissée ; à 0 c'est le comportement de l'original.
    //  - Comme l'original, oscillateur et filtre ne sont jamais remis à zéro
    //    entre les notes (une remise à zéro rend l'attaque 4-5 dB plus faible).
    //
    // Aucune allocation ; process() tourne à la fréquence suréchantillonnée.
    //==========================================================================
    class AcidVoice
    {
    public:
        static constexpr float kOutputGain     = 0.16f;   // calibré au test : le pire cas (accent max) reste ~0 dBFS
        static constexpr float kAccentDecayMs  = 200.0f;
        static constexpr float kEnvAttackS     = 0.003f;  // attaque du MEG vers le cutoff
        static constexpr float kAccentOctaves  = 2.4f;    // ouverture max du filtre par l'accent sweep
        static constexpr float kAmpDecayS      = 1.23f;
        static constexpr float kAmpReleaseS    = 0.0005f; // note normale
        static constexpr float kAmpReleaseAccS = 0.05f;   // note accentuée
        static constexpr float kMaxK           = 16.6f;   // juste sous l'auto-oscillation

        void prepare (double sampleRate) noexcept
        {
            fs = sampleRate;
            invFs = 1.0f / (float) sampleRate;
            osc.prepare (sampleRate);
            filter.prepare (sampleRate);
            preHighpass.prepare (sampleRate, 44.486f);
            ampDeclicker.prepare (sampleRate, 200.0f);

            smoothCoef   = onePoleCoef (0.008f, sampleRate);
            envAttack    = onePoleCoef (kEnvAttackS, sampleRate);

            // Condensateur d'accent : charge / décharge pour Slow, Normal, Fast
            const float chargeS[3]  = { 0.10f, 0.05f, 0.025f };
            const float releaseS[3] = { 1.20f, 0.45f, 0.12f };
            for (int i = 0; i < 3; ++i)
            {
                sweepCharge[i]  = onePoleCoef (chargeS[i], sampleRate);
                sweepRelease[i] = onePoleCoef (releaseS[i], sampleRate);
            }
            cachedSlideMs = -1.0f;
            updateSlide();
            ampDecay     = std::exp (-1.0f / (kAmpDecayS * (float) sampleRate));
            ampRelNormal = std::exp (-1.0f / (kAmpReleaseS * (float) sampleRate));
            ampRelAccent = std::exp (-1.0f / (kAmpReleaseAccS * (float) sampleRate));
            accentMegCoef = std::exp (-1000.0f / (kAccentDecayMs * (float) sampleRate));
        }

        // Changement de fréquence (Normal <-> High) sans couper le son
        void setSampleRate (double sampleRate) noexcept
        {
            const VoiceParams keep = sm;
            prepare (sampleRate);
            sm = keep;
        }

        void reset() noexcept
        {
            osc.reset();
            filter.reset();
            preHighpass.reset();
            ampDeclicker.reset();
            notes.clear();
            gate = false;
            accentNote = false;
            meg = envSmoothed = sweep = ampEnv = 0.0f;
            sm = target;
        }

        void setParams (const VoiceParams& p) noexcept
        {
            target = p;
            updateSlide();
        }
        void setAccentThreshold (int velocity) noexcept { accentThreshold = velocity; }

        //======================================================================
        void noteOn (int note, int velocity) noexcept
        {
            const bool legato = ! notes.empty() && gate;
            notes.push (note);
            accentNote = velocity >= accentThreshold;
            targetPitch = (float) note;

            if (legato)
            {
                // Slide : par défaut ni MEG ni VCA redéclenchés (comme l'original).
                // Slide Punch relance une partie de l'enveloppe et du volume.
                const float punch = target.slidePunch;
                meg    += (1.0f - meg)    * punch;
                ampEnv += (1.0f - ampEnv) * punch;
                return;
            }

            freq = midiToHz (targetPitch);   // hauteur directe (pas de slide)
            pitch = targetPitch;
            meg = 1.0f;
            ampEnv = 1.0f;
            gate = true;
        }

        void noteOff (int note) noexcept
        {
            const bool wasTop = notes.top() == note;
            notes.remove (note);

            if (notes.empty())
                gate = false;
            else if (wasTop)
                targetPitch = (float) notes.top();   // retour legato vers la note encore tenue
        }

        void allNotesOff() noexcept
        {
            notes.clear();
            gate = false;
        }

        //======================================================================
        float process() noexcept
        {
            // Lissage des paramètres (zéro zipper noise)
            sm.tune      += (target.tune      - sm.tune)      * smoothCoef;
            sm.cutoff    += (target.cutoff    - sm.cutoff)    * smoothCoef;
            sm.resonance += (target.resonance - sm.resonance) * smoothCoef;
            sm.envMod    += (target.envMod    - sm.envMod)    * smoothCoef;
            sm.decay     += (target.decay     - sm.decay)     * smoothCoef;
            sm.accent    += (target.accent    - sm.accent)    * smoothCoef;
            sm.squareMix += (target.squareMix - sm.squareMix) * smoothCoef;

            // --- Hauteur + slide RC (en fréquence, comme le circuit de l'original)
            freq += (midiToHz (targetPitch) - freq) * slideCoef;
            const float inc = std::min (freq * tuneFactor (sm.tune) * invFs, 0.45f);
            const float raw = preHighpass.process (osc.process (inc, sm.squareMix));

            // --- MEG : constante de temps 200..2000 ms (200 ms sur accent)
            const float megCoef = accentNote ? accentMegCoef
                                             : std::exp (-invFs / (0.2f * std::exp2 (sm.decay * 3.3219281f)));
            meg *= megCoef;
            envSmoothed += (meg - envSmoothed) * envAttack;

            // --- Accent sweep : le condensateur se charge vers le MEG sur les notes
            //     accentuées et se vide lentement -> accumulation des accents
            const float accentAmount = accentNote ? sm.accent : 0.0f;
            const int sp = target.sweepSpeed;
            const float sweepTarget = accentNote ? meg : 0.0f;
            if (sweepTarget > sweep) sweep += (sweepTarget - sweep) * sweepCharge[sp];
            else                     sweep += (sweepTarget - sweep) * sweepRelease[sp];

            // --- Cutoff : loi mesurée (Open303) entre 314 et 2394 Hz
            const float c = sm.cutoff, e = sm.envMod;
            const float envScaler = (1.0f - c) * (3.773996f * e + 0.736966f) + c * (4.194549f * e + 0.864345f);
            const float envOffset = 0.048293f * c + 0.294391f;
            const float baseHz = 313.815f * std::exp2 (c * 2.931652f);
            const float octaves = envScaler * (envSmoothed - envOffset) + kAccentOctaves * sm.accent * sweep;
            const float cutoffHz = std::max (200.0f, baseHz * std::exp2 (octaves));

            // --- Résonance : courbe de la 303 (déjà forte à mi-course)
            const float rSkew = (1.0f - std::exp (-3.0f * sm.resonance)) * 1.0524193f;   // / (1 - e^-3)
            const float k = kMaxK * rSkew;
            const float filtered = filter.process (raw, cutoffHz, k, 2.0f * (1.0f + rSkew));

            // --- VCA : enveloppe lente + MEG ajouté (punch), lissé par le déclicker
            ampEnv *= gate ? ampDecay : (accentNote ? ampRelAccent : ampRelNormal);
            float amp = ampEnv;
            if (gate)
                amp += (0.45f + 4.0f * accentAmount) * meg;
            amp = ampDeclicker.process (amp);

            return filtered * amp * kOutputGain;
        }

        // Pour le test / l'interface
        float getAccentSweep() const noexcept { return sweep; }
        float getPitch() const noexcept { return 69.0f + 12.0f * std::log2 (freq / 440.0f); }
        bool isGateOn() const noexcept { return gate; }
        bool isAccent() const noexcept { return accentNote; }

    private:
        void updateSlide() noexcept
        {
            const float ms = std::clamp (target.slideTimeMs, 20.0f, 400.0f);
            if (std::abs (ms - cachedSlideMs) > 0.01f)
            {
                cachedSlideMs = ms;
                slideCoef = onePoleCoef (0.2f * ms * 0.001f, fs);   // constante = 0,2 x durée
            }
        }

        float tuneFactor (float tune) noexcept
        {
            if (std::abs (tune - cachedTune) > 1.0e-6f)
            {
                cachedTune = tune;
                cachedTuneFactor = std::exp2 (tune * (1.0f / 12.0f));
            }
            return cachedTuneFactor;
        }

        double fs = 44100.0;
        float invFs = 1.0f / 44100.0f;

        AcidOscillator osc;
        DiodeLadder filter;
        OnePoleHighpass preHighpass;
        SvfLowpass ampDeclicker;
        NoteStack notes;

        VoiceParams target, sm;
        int accentThreshold = 100;

        bool gate = false, accentNote = false;
        float pitch = 60.0f, targetPitch = 60.0f, freq = 261.63f;
        float cachedTune = 0.0f, cachedTuneFactor = 1.0f;
        float meg = 0.0f, envSmoothed = 0.0f, sweep = 0.0f, ampEnv = 0.0f;

        float smoothCoef = 0.0f, slideCoef = 0.0f, envAttack = 0.0f;
        float sweepCharge[3] {}, sweepRelease[3] {};
        float cachedSlideMs = -1.0f;
        float ampDecay = 0.0f, ampRelNormal = 0.0f, ampRelAccent = 0.0f, accentMegCoef = 0.0f;
    };
}

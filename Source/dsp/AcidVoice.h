#pragma once

#include "DspUtils.h"
#include "Filters.h"
#include "Oscillator.h"
#include "DiodeLadder.h"
#include "NoteStack.h"
#include "Lfo.h"
#include "Drive.h"

namespace octocid
{
    //==========================================================================
    // Paramètres de la voix (normalisés sauf mention contraire)
    //==========================================================================
    struct VoiceParams
    {
        // Panneau principal
        float tune      = 0.0f;   // -12..+12 demi-tons
        float cutoff    = 0.5f;   // 0..1
        float resonance = 0.5f;   // 0..1
        float envMod    = 0.5f;   // 0..1
        float decay     = 0.5f;   // 0..1
        float accent    = 0.5f;   // 0..1
        float squareMix = 0.0f;   // 0 = saw, 1 = square

        // Slime Mods (inspirés des mods Devil Fish) — à leur défaut : son d'origine
        float slideTimeMs   = 60.0f;   // 60 .. 360 ms
        int   sweepSpeed    = 1;       // 0 = Slow, 1 = Normal, 2 = Fast
        float slidePunch    = 0.0f;    // relance sur les notes glissées
        float accentDecayMs = 200.0f;  // decay du MEG sur les notes accentuées (30..3000)
        float softAttackMs  = 3.0f;    // attaque des notes non accentuées (0,3..30)
        float filterTrack   = 0.0f;    // le cutoff suit la note (1 = 1 octave par octave)
        float filterFm      = 0.0f;    // la sortie module le cutoff
        float filterDrive   = 0.0f;    // gain dans le filtre (saturation de la résonance)
        int   subMode       = 0;       // 0 Off, 1 = -1 octave, 2 = -2 octaves
        float subLevel      = 0.5f;
        int   muffler       = 0;       // 0 Off, 1, 2
        float analog        = 0.0f;    // dérive lente + rampe imparfaite (0 = son numérique propre)

        // LFO
        float lfoRateHz = 2.0f;
        int   lfoShape  = 0;           // Sine, Triangle, Square, S&H
        int   lfoDest   = 0;           // Cutoff, Resonance, Pitch
        float lfoDepth  = 0.0f;
    };

    //==========================================================================
    // Voix acid monophonique.
    //
    // Calibrage du cœur (plages des potards, enveloppes, accent, courbe de
    // résonance, filtres d'entrée/sortie) repris des mesures du projet Open303
    // (c) 2009 Robin Schmidt, licence MIT — voir THIRD_PARTY.md.
    //
    //  - MEG (enveloppe de filtre) : attaque ~3 ms, constante de décroissance
    //    200..2000 ms ; Accent Decay séparé pour les notes accentuées.
    //  - Cutoff : 314..2394 Hz, loi d'enveloppe mesurée sur l'original.
    //  - VCA : décroissance lente (1,23 s) + MEG ajouté au volume (punch).
    //  - Accent sweep à condensateur, cumulatif ; il pousse aussi un peu la
    //    résonance (le « squelch » des accents).
    //  - Slide RC en fréquence 60..360 ms ; Slide Punch relance les glides.
    //  - Mods : Soft Attack, Filter Tracking, Filter FM (sortie du VCA réinjectée
    //    dans le cutoff), Filter Drive (saturation de l'entrée du filtre),
    //    Sub Osc (après le filtre, rond et propre), Muffler (écrêtage doux qui
    //    garde le grave, après le VCA).
    //  - Oscillateur et filtre ne sont jamais remis à zéro entre les notes.
    //
    // Aucune allocation ; process() tourne à la fréquence suréchantillonnée.
    //==========================================================================
    class AcidVoice
    {
    public:
        static constexpr float kOutputGain    = 0.16f;   // calibré au test : le pire cas reste sous 0 dBFS
        static constexpr float kAccentAttackS = 0.003f;  // attaque du MEG des notes accentuées
        static constexpr float kAccentOctaves = 2.4f;    // ouverture max du filtre par l'accent sweep
        static constexpr float kAmpDecayS      = 1.23f;
        static constexpr float kAmpReleaseS    = 0.0005f;
        static constexpr float kAmpReleaseAccS = 0.05f;
        static constexpr float kMaxK           = 16.6f;  // juste sous l'auto-oscillation
        static constexpr float kMaxKAccent     = 16.9f;
        static constexpr float kPreVcaLevel    = 0.3f;

        void prepare (double sampleRate) noexcept
        {
            fs = sampleRate;
            invFs = 1.0f / (float) sampleRate;
            osc.prepare (sampleRate);
            filter.prepare (sampleRate);
            preHighpass.prepare (sampleRate, 44.486f);
            ampDeclicker.prepare (sampleRate, 200.0f);
            mufflerLow.prepare (sampleRate, 160.0f);
            lfo.prepare (sampleRate);

            smoothCoef      = onePoleCoef (0.008f, sampleRate);
            accentAttackCoef = onePoleCoef (kAccentAttackS, sampleRate);
            ampDecay     = std::exp (-1.0f / (kAmpDecayS * (float) sampleRate));
            ampRelNormal = std::exp (-1.0f / (kAmpReleaseS * (float) sampleRate));
            ampRelAccent = std::exp (-1.0f / (kAmpReleaseAccS * (float) sampleRate));

            const float chargeS[3]  = { 0.10f, 0.05f, 0.025f };
            const float releaseS[3] = { 1.20f, 0.45f, 0.12f };
            for (int i = 0; i < 3; ++i)
            {
                sweepCharge[i]  = onePoleCoef (chargeS[i], sampleRate);
                sweepRelease[i] = onePoleCoef (releaseS[i], sampleRate);
            }

            cachedSlideMs = cachedAccDecMs = cachedSoftMs = -1.0f;
            driftPeriod = std::max (1, (int) (sampleRate / 3.0));       // nouvelle cible ~3 fois par seconde
            driftCoef = onePoleCoef (0.35f, sampleRate);
            updateCoefs();
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
            mufflerLow.reset();
            lfo.reset();
            notes.clear();
            gate = false;
            accentNote = false;
            meg = envSmoothed = sweep = ampEnv = 0.0f;
            vcaAttack = 1.0f;
            subPhase = 0.0f;
            lastOut = 0.0f;
            driftP = driftC = driftPT = driftCT = 0.0f;
            driftCount = 0;
            sm = target;
        }

        void setParams (const VoiceParams& p) noexcept
        {
            target = p;
            lfo.setRate (p.lfoRateHz);
            lfo.setShape (p.lfoShape);
            updateCoefs();
        }

        void setAccentThreshold (int velocity) noexcept { accentThreshold = velocity; }

        // Drive placé avant le VCA (nullptr = Drive après la voix, géré par le processeur).
        // Avant le VCA, l'enveloppe de volume sculpte le son distordu : il reste percutant.
        void setPreVcaDrive (Drive* d) noexcept { preVcaDrive = d; }
        void syncLfo (float phase01) noexcept { lfo.syncPhase (phase01); }

        //======================================================================
        void noteOn (int note, int velocity) noexcept
        {
            noteOnAccent (note, velocity >= accentThreshold);
        }

        // Note avec accent explicite (séquenceur)
        void noteOnAccent (int note, bool accent) noexcept
        {
            const bool legato = ! notes.empty() && gate;
            notes.push (note);
            accentNote = accent;
            targetPitch = (float) note;

            if (legato)
            {
                // Slide : par défaut ni MEG ni VCA redéclenchés (comme l'original).
                const float punch = target.slidePunch;
                meg    += (1.0f - meg)    * punch;
                ampEnv += (1.0f - ampEnv) * punch;
                return;
            }

            freq = midiToHz (targetPitch);   // hauteur directe (pas de slide)
            meg = 1.0f;
            ampEnv = 1.0f;
            vcaAttack = (! accentNote && vcaAttackCoef < 1.0f) ? 0.0f : 1.0f;
            gate = true;
        }

        void noteOff (int note) noexcept
        {
            const bool wasTop = notes.top() == note;
            notes.remove (note);

            if (notes.empty())
                gate = false;
            else if (wasTop)
                targetPitch = (float) notes.top();
        }

        void allNotesOff() noexcept
        {
            notes.clear();
            gate = false;
        }

        //======================================================================
        float process() noexcept
        {
            // Lissage des paramètres continus (zéro zipper noise)
            auto smoothTo = [this] (float& v, float t) { v += (t - v) * smoothCoef; };
            smoothTo (sm.tune,        target.tune);
            smoothTo (sm.cutoff,      target.cutoff);
            smoothTo (sm.resonance,   target.resonance);
            smoothTo (sm.envMod,      target.envMod);
            smoothTo (sm.decay,       target.decay);
            smoothTo (sm.accent,      target.accent);
            smoothTo (sm.squareMix,   target.squareMix);
            smoothTo (sm.filterTrack, target.filterTrack);
            smoothTo (sm.filterFm,    target.filterFm);
            smoothTo (sm.filterDrive, target.filterDrive);
            smoothTo (sm.subLevel,    target.subMode > 0 ? target.subLevel : 0.0f);
            smoothTo (sm.lfoDepth,    target.lfoDepth);
            smoothTo (sm.analog,      target.analog);

            // --- Analog : deux dérives lentes et indépendantes (hauteur, cutoff)
            float driftCents = 0.0f, driftOct = 0.0f, curve = 0.0f;
            if (sm.analog > 1.0e-4f)
            {
                if (--driftCount <= 0)
                {
                    driftCount = driftPeriod;
                    driftPT = rand01() * 2.0f - 1.0f;
                    driftCT = rand01() * 2.0f - 1.0f;
                }
                driftP += (driftPT - driftP) * driftCoef;
                driftC += (driftCT - driftC) * driftCoef;
                driftCents = sm.analog * 7.0f * driftP;
                driftOct = sm.analog * 0.12f * driftC;
                curve = sm.analog * 1.6f;
            }

            // --- LFO
            const float lfoRaw = lfo.process();                       // tourne toujours (phase continue)
            lastLfo = lfoRaw;
            const bool lfoOn = sm.lfoDepth > 1.0e-4f;
            const float lfoOut = lfoOn ? lfoRaw * sm.lfoDepth : 0.0f;
            const int lfoDest = target.lfoDest;

            // --- Hauteur + slide RC (en fréquence, comme le circuit de l'original)
            freq += (midiToHz (targetPitch) - freq) * slideCoef;
            float f = freq * tuneFactor (sm.tune);
            if (sm.analog > 1.0e-4f)
                f *= 1.0f + driftCents * 0.000577623f;               // cents -> rapport (approx. linéaire)
            if (lfoDest == 2 && lfoOn)
                f *= std::exp2 (lfoOut * (2.0f / 12.0f));            // vibrato ±2 demi-tons
            const float inc = std::min (f * invFs, 0.45f);

            float raw = preHighpass.process (osc.process (inc, sm.squareMix, curve));

            // --- MEG
            meg *= accentNote ? accentMegCoef : std::exp (-invFs / (0.2f * std::exp2 (sm.decay * 3.3219281f)));
            envSmoothed += (meg - envSmoothed) * (accentNote ? accentAttackCoef : softAttackCoef);

            // --- Accent sweep (condensateur cumulatif)
            const float accentAmount = accentNote ? sm.accent : 0.0f;
            const int sp = target.sweepSpeed;
            const float sweepTarget = accentNote ? meg : 0.0f;
            sweep += (sweepTarget - sweep) * (sweepTarget > sweep ? sweepCharge[sp] : sweepRelease[sp]);

            // --- Cutoff (loi mesurée Open303) + mods
            const float e = sm.envMod;
            const float cutOct = cutoffOctaves (sm.cutoff);                    // plage élargie aux extrémités
            const float c = std::clamp (cutOct * (1.0f / 2.931652f), 0.0f, 1.0f); // équivalent du potard d'origine
            const float envScaler = (1.0f - c) * (3.773996f * e + 0.736966f) + c * (4.194549f * e + 0.864345f);
            const float envOffset = 0.048293f * c + 0.294391f;
            const float baseHz = 313.815f * std::exp2 (cutOct);
            float octaves = envScaler * (envSmoothed - envOffset) + kAccentOctaves * sm.accent * sweep + driftOct;
            if (sm.filterTrack > 1.0e-4f)
                octaves += sm.filterTrack * std::log2 (f * (1.0f / 130.8128f));   // référence : Do3
            if (sm.filterFm > 1.0e-4f)
                octaves += sm.filterFm * 3.0f * lastOut;
            if (lfoDest == 0 && lfoOn)
                octaves += 2.5f * lfoOut;
            const float cutoffHz = std::clamp (baseHz * std::exp2 (octaves), std::min (200.0f, baseHz * 0.64f), 0.45f * (float) fs);

            // --- Résonance : courbe 303 + LFO + petit coup de l'accent
            float res = sm.resonance;
            if (lfoDest == 1 && lfoOn)
                res = std::clamp (res + 0.5f * lfoOut, 0.0f, 1.0f);
            const float rSkew = (1.0f - std::exp (-3.0f * res)) * 1.0524193f;   // / (1 - e^-3)
            const float k = std::min (kMaxK * rSkew * (1.0f + 0.12f * sm.accent * sweep), kMaxKAccent);

            // --- Filtre (Filter Drive = gain d'entrée, sature la boucle)
            const float driveGain = 1.0f + 7.0f * sm.filterDrive * sm.filterDrive;
            raw *= driveGain;
            float filtered = filter.process (raw, cutoffHz, k, 2.0f * (1.0f + rSkew) / std::sqrt (driveGain));

            // --- Sub-oscillateur (après le filtre : rond et propre)
            if (sm.subLevel > 1.0e-4f)
            {
                const float div = target.subMode == 2 ? 0.25f : 0.5f;
                subPhase += inc * div;
                if (subPhase >= 1.0f) subPhase -= 1.0f;
                const float sub = std::tanh (2.5f * std::sin (2.0f * kPi * subPhase)) * 1.0136f;
                filtered += sm.subLevel * 1.4f * sub;
            }

            // --- Drive avant le VCA (niveau ramené vers celui qu'il voit après le VCA)
            if (preVcaDrive != nullptr)
                filtered = preVcaDrive->process (filtered * kPreVcaLevel) * (1.0f / kPreVcaLevel);

            // --- VCA
            ampEnv *= gate ? ampDecay : (accentNote ? ampRelAccent : ampRelNormal);
            float amp = ampEnv;
            if (gate)
                amp += (0.45f + 4.0f * accentAmount) * meg;
            vcaAttack += (1.0f - vcaAttack) * vcaAttackCoef;
            amp = ampDeclicker.process (amp * vcaAttack);

            float out = filtered * amp;

            // --- Muffler : écrêtage doux du haut du spectre, le grave passe intact
            if (target.muffler > 0)
            {
                const float low = mufflerLow.process (out);
                const float T = target.muffler == 1 ? 1.2f : 0.5f;
                out = low + T * fastTanh ((out - low) / T);
            }

            lastOut = out * 0.25f;
            return out * kOutputGain;
        }

        // Pour le test / l'interface
        float getAccentSweep() const noexcept { return sweep; }
        float getPitch() const noexcept { return 69.0f + 12.0f * std::log2 (freq / 440.0f); }
        bool isGateOn() const noexcept { return gate; }

        // Potard Cutoff -> octaves au-dessus de 313,8 Hz.
        // Entre 25 et 75 % : loi mesurée de l'original (314 Hz .. 2,4 kHz sur toute la course).
        // En dessous : jusqu'à 2,6 octaves de plus vers le grave (filtre vraiment fermé, ~52 Hz) ;
        // au-dessus : jusqu'à 1 octave de plus vers l'aigu (~4,8 kHz).
        static float cutoffOctaves (float knob) noexcept
        {
            const float c = std::clamp (knob, 0.0f, 1.0f);
            float oct = 2.931652f * c;
            if (c < 0.25f) oct -= 2.6f * std::pow ((0.25f - c) * 4.0f, 1.3f);
            if (c > 0.75f) oct += 1.0f * std::pow ((c - 0.75f) * 4.0f, 1.3f);
            return oct;
        }
        float getLfoValue() const noexcept { return lastLfo; }   // pour l'animation de l'escargot
        bool isAccent() const noexcept { return accentNote; }

    private:
        void updateCoefs() noexcept
        {
            const float slideMs = std::clamp (target.slideTimeMs, 20.0f, 400.0f);
            if (std::abs (slideMs - cachedSlideMs) > 0.01f)
            {
                cachedSlideMs = slideMs;
                slideCoef = onePoleCoef (0.2f * slideMs * 0.001f, fs);    // constante = 0,2 x durée
            }

            const float accDec = std::clamp (target.accentDecayMs, 30.0f, 3000.0f);
            if (std::abs (accDec - cachedAccDecMs) > 0.01f)
            {
                cachedAccDecMs = accDec;
                accentMegCoef = std::exp (-1000.0f / (accDec * (float) fs));
            }

            const float soft = std::clamp (target.softAttackMs, 0.3f, 30.0f);
            if (std::abs (soft - cachedSoftMs) > 0.001f)
            {
                cachedSoftMs = soft;
                softAttackCoef = onePoleCoef (soft * 0.001f, fs);
                // Au-delà de 3 ms (réglage d'origine), le volume reçoit aussi une attaque
                const float vcaMs = soft - 3.0f;
                vcaAttackCoef = vcaMs > 0.05f ? onePoleCoef (vcaMs * 0.001f, fs) : 1.0f;
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
        SvfLowpass ampDeclicker, mufflerLow;
        NoteStack notes;
        Lfo lfo;
        float lastLfo = 0.0f;
        Drive* preVcaDrive = nullptr;

        VoiceParams target, sm;
        int accentThreshold = 100;

        bool gate = false, accentNote = false;
        float targetPitch = 60.0f, freq = 261.63f;
        float cachedTune = 0.0f, cachedTuneFactor = 1.0f;
        float meg = 0.0f, envSmoothed = 0.0f, sweep = 0.0f, ampEnv = 0.0f, vcaAttack = 1.0f;
        float subPhase = 0.0f, lastOut = 0.0f;
        float driftP = 0.0f, driftC = 0.0f, driftPT = 0.0f, driftCT = 0.0f, driftCoef = 0.0f;
        int driftCount = 0, driftPeriod = 1;
        uint32_t rngState = 0x2545F491u;
        float rand01() noexcept { rngState = rngState * 1664525u + 1013904223u; return (float) (rngState >> 8) * (1.0f / 16777216.0f); }

        float smoothCoef = 0.0f, slideCoef = 0.0f, accentAttackCoef = 0.0f, softAttackCoef = 0.0f, vcaAttackCoef = 1.0f;
        float sweepCharge[3] {}, sweepRelease[3] {};
        float ampDecay = 0.0f, ampRelNormal = 0.0f, ampRelAccent = 0.0f, accentMegCoef = 0.0f;
        float cachedSlideMs = -1.0f, cachedAccDecMs = -1.0f, cachedSoftMs = -1.0f;
    };
}

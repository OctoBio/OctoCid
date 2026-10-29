#pragma once

#include "DspUtils.h"
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
    };

    //==========================================================================
    // Voix acid monophonique, comportement fidèle à l'original :
    //  - MEG (enveloppe de filtre) : attaque instantanée, decay 200 ms..2,5 s,
    //    forcé au minimum sur les notes accentuées.
    //  - VCA : attaque très courte, décroissance lente tant que la note est
    //    tenue, relâchement rapide.
    //  - Accent : volume plus fort + circuit « accent sweep » à condensateur :
    //    il se charge plus lentement que le MEG ne retombe, donc une seule note
    //    n'atteint pas le maximum, mais des accents rapprochés s'accumulent.
    //  - Slide : deux notes qui se chevauchent -> glissement exponentiel (RC)
    //    sans redéclencher les enveloppes.
    //
    // Aucune allocation, tout est en float, process() est appelé à la
    // fréquence suréchantillonnée.
    //==========================================================================
    class AcidVoice
    {
    public:
        static constexpr float kOutputGain    = 0.5f;    // calibré au test (~-6 dBFS crête)
        static constexpr float kAccentDecayS  = 0.2f;    // decay du MEG sur accent
        static constexpr float kSlideTauS     = 0.03f;   // ~60 ms pour arriver sur la note
        static constexpr float kSweepChargeS  = 0.06f;   // charge du condensateur d'accent
        static constexpr float kSweepReleaseS = 0.25f;   // décharge (lente -> accumulation)
        static constexpr float kAccentOctaves = 4.0f;    // ouverture max du filtre par l'accent

        void prepare (double sampleRate) noexcept
        {
            fs = sampleRate;
            invFs = 1.0f / (float) sampleRate;
            osc.prepare (sampleRate);
            filter.prepare (sampleRate);

            smoothCoef   = onePoleCoef (0.008f, sampleRate);
            slideCoef    = onePoleCoef (kSlideTauS, sampleRate);
            sweepCharge  = onePoleCoef (kSweepChargeS, sampleRate);
            sweepRelease = onePoleCoef (kSweepReleaseS, sampleRate);
            ampAttack    = onePoleCoef (0.0012f, sampleRate);
            ampRelease   = onePoleCoef (0.004f, sampleRate);
            accGainCoef  = onePoleCoef (0.002f, sampleRate);
            vcaDecay     = std::exp (-1.0f / (2.5f * (float) sampleRate));
            maxCutoff    = 0.42f * (float) sampleRate;
        }

        // Changement de fréquence d'échantillonnage (Normal <-> High) sans couper le son
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
            notes.clear();
            gate = false;
            accentNote = false;
            meg = sweep = amp = decayLevel = 0.0f;
            accGain = 1.0f;
            sm = target;
        }

        void setParams (const VoiceParams& p) noexcept { target = p; }
        void setAccentThreshold (int velocity) noexcept { accentThreshold = velocity; }

        //======================================================================
        void noteOn (int note, int velocity) noexcept
        {
            const bool legato = ! notes.empty() && gate;
            notes.push (note);
            accentNote = velocity >= accentThreshold;
            targetPitch = (float) note;

            if (legato)
                return;                        // slide : pas de redéclenchement

            pitch = targetPitch;               // nouvelle note : hauteur directe
            meg = 1.0f;
            decayLevel = 1.0f;
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

            // Hauteur + slide RC
            pitch += (targetPitch - pitch) * slideCoef;
            const float inc = std::min (midiToHz (pitch + sm.tune) * invFs, 0.45f);
            const float raw = osc.process (inc, sm.squareMix);

            // MEG : decay 0,2 s .. 2,5 s (temps pour tomber à ~10 %)
            const float decayS = accentNote ? kAccentDecayS
                                            : 0.2f * std::exp2 (sm.decay * 3.6439f);
            meg *= std::exp (-2.3f * invFs / decayS);

            // Accent sweep (condensateur qui se charge / se décharge)
            const float sweepTarget = accentNote ? meg * sm.accent : 0.0f;
            sweep += (sweepTarget - sweep) * (sweepTarget > sweep ? sweepCharge : sweepRelease);

            // Cutoff : l'Env Mod élargit la modulation ET abaisse la base (comme l'original)
            const float baseHz = 260.0f * std::exp2 (sm.cutoff * 3.6f);
            const float octaves = sm.envMod * (4.5f * meg - 1.0f) + kAccentOctaves * sweep;
            const float cutoffHz = std::min (baseHz * std::exp2 (octaves), maxCutoff);

            const float k = 0.97f * DiodeLadder::kSelfOsc * sm.resonance;
            const float filtered = filter.process (raw, cutoffHz, k);

            // VCA
            decayLevel *= vcaDecay;
            const float ampTarget = gate ? decayLevel : 0.0f;
            amp += (ampTarget - amp) * (ampTarget > amp ? ampAttack : ampRelease);

            const float accTarget = accentNote ? 1.0f + sm.accent : 1.0f;
            accGain += (accTarget - accGain) * accGainCoef;

            return filtered * amp * accGain * kOutputGain;
        }

        // Pour le test / l'interface
        float getAccentSweep() const noexcept { return sweep; }
        float getPitch() const noexcept { return pitch; }
        bool isGateOn() const noexcept { return gate; }
        bool isAccent() const noexcept { return accentNote; }

    private:
        double fs = 44100.0;
        float invFs = 1.0f / 44100.0f;

        AcidOscillator osc;
        DiodeLadder filter;
        NoteStack notes;

        VoiceParams target, sm;
        int accentThreshold = 100;

        bool gate = false, accentNote = false;
        float pitch = 60.0f, targetPitch = 60.0f;
        float meg = 0.0f, sweep = 0.0f, amp = 0.0f, decayLevel = 0.0f, accGain = 1.0f;

        float smoothCoef = 0.0f, slideCoef = 0.0f, sweepCharge = 0.0f, sweepRelease = 0.0f;
        float ampAttack = 0.0f, ampRelease = 0.0f, accGainCoef = 0.0f, vcaDecay = 0.0f;
        float maxCutoff = 18000.0f;
    };
}

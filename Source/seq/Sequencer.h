#pragma once

#include "Pattern.h"
#include <cmath>
#include <array>
#include <algorithm>
#include <cstdint>
#include <climits>

namespace octocid
{
    //==========================================================================
    // Évènement de note pour la voix (séquenceur ou clavier), au sample près.
    //==========================================================================
    struct NoteEvent
    {
        enum Type : uint8_t { On = 0, Off = 1, AllOff = 2 };
        int sample = 0;
        Type type = On;
        int note = 0;
        bool accent = false;
    };

    // Liste d'évènements à capacité fixe (aucune allocation dans le thread audio)
    class NoteEventList
    {
    public:
        static constexpr int kCapacity = 2048;

        void clear() noexcept { n = 0; }
        int size() const noexcept { return n; }
        const NoteEvent& operator[] (int i) const noexcept { return e[(size_t) i]; }

        // Insertion triée par sample ; à sample égal, l'ordre d'ajout est conservé
        void add (const NoteEvent& ev) noexcept
        {
            if (n >= kCapacity) return;
            int i = n;
            while (i > 0 && e[(size_t) i - 1].sample > ev.sample)
            {
                e[(size_t) i] = e[(size_t) i - 1];
                --i;
            }
            e[(size_t) i] = ev;
            ++n;
        }

    private:
        std::array<NoteEvent, kCapacity> e {};
        int n = 0;
    };

    //==========================================================================
    // Séquenceur pas à pas, calé sur la position du DAW (en noires, « ppq »).
    //
    //  - Note : joue la note ; si le pas précédent avait Slide, la nouvelle note
    //    est enfoncée avant que l'ancienne soit relâchée -> glissement legato.
    //  - Slide vers la même hauteur = liaison (pas de redéclenchement).
    //  - Tie : prolonge la note précédente.  Rest : silence.
    //  - Gate des notes non liées : 50 % du pas par défaut (comme l'original), réglable.
    //  - Swing : les pas impairs sont retardés (jusqu'à 37,5 % d'un pas à 75 %).
    //  - Changement de pattern : appliqué au début du cycle suivant.
    //  - Saut de position (boucle, déplacement du curseur) : on coupe proprement.
    //==========================================================================
    class StepSequencer
    {
    public:
        // Longueur de gate des notes non liées (0,5 = 50 % du pas, comme l'original)
        void setGate (double g) noexcept { gateLen = std::clamp (g, 0.05, 0.95); }

        void reset() noexcept
        {
            expectedPpq = -1.0e9;
            haveTiming = false;
            heldNote = -1;
            pendingOff = -1.0;
            prevSlide = false;
            stepIdx = -1;
            lastK = INT64_MIN;
        }

        void setBank (const PatternBank& b) noexcept { bank = b; }
        void setPattern (int index, const Pattern& p) noexcept
        {
            if (index >= 0 && index < PatternBank::kNumPatterns)
                bank.patterns[index] = p;
        }

        void requestPattern (int index) noexcept { requested = std::clamp (index, 0, PatternBank::kNumPatterns - 1); }
        void setTranspose (int semis) noexcept { transpose = std::clamp (semis, -36, 36); }
        int getTranspose() const noexcept { return transpose; }
        int getActivePattern() const noexcept { return active; }
        int getRequestedPattern() const noexcept { return requested; }
        int getCurrentStep() const noexcept { return stepIdx; }

        // Coupe toute note en cours (transport arrêté, séquenceur désactivé)
        void stop (NoteEventList& out, int sample) noexcept
        {
            out.add ({ sample, NoteEvent::AllOff, 0, false });
            const int keepActive = active;
            reset();
            active = keepActive;
        }

        //======================================================================
        // Génère les évènements du bloc [ppqStart, ppqStart + numSamples).
        // stepBeats : durée d'un pas en noires ; swing : 0..0,75 ; baseNote : Do de référence.
        //======================================================================
        void process (double ppqStart, double bpm, int numSamples, double sampleRate,
                      double stepBeats, float swing, int baseNote, NoteEventList& out) noexcept
        {
            if (numSamples <= 0 || bpm <= 0.0 || stepBeats <= 0.0)
                return;

            const double spb = sampleRate * 60.0 / bpm;              // samples par noire
            const double ppqEnd = ppqStart + numSamples / spb;

            // Saut de position : on repart proprement
            if (haveTiming && std::abs (ppqStart - expectedPpq) > 1.0e-3)
            {
                out.add ({ 0, NoteEvent::AllOff, 0, false });
                heldNote = -1; pendingOff = -1.0; prevSlide = false; lastK = INT64_MIN;
            }
            haveTiming = true;
            expectedPpq = ppqEnd;

            auto toSample = [&] (double t)
            {
                const double s = (t - ppqStart) * spb;
                return std::clamp ((int) std::llround (s), 0, numSamples - 1);
            };
            const double swingDelay = std::clamp ((double) swing, 0.0, 0.75) * 0.5 * stepBeats;
            auto stepTime = [&] (int64_t k) { return (double) k * stepBeats + ((k & 1) != 0 ? swingDelay : 0.0); };

            const int64_t kFirst = (int64_t) std::floor (ppqStart / stepBeats) - 1;
            const int64_t kLast  = (int64_t) std::floor (ppqEnd / stepBeats) + 1;

            for (int64_t k = kFirst; k <= kLast; ++k)
            {
                const double t = stepTime (k);
                if (t < ppqStart || t >= ppqEnd)
                    continue;

                flushPendingOff (t, ppqStart, toSample, out);
                handleStep (k, t, stepBeats, baseNote, toSample (t), out);
            }

            flushPendingOff (ppqEnd, ppqStart, toSample, out);
        }

    private:
        template <typename ToSample>
        void flushPendingOff (double upTo, double ppqStart, ToSample toSample, NoteEventList& out) noexcept
        {
            if (pendingOff >= 0.0 && pendingOff < upTo && heldNote >= 0)
            {
                out.add ({ toSample (std::max (pendingOff, ppqStart)), NoteEvent::Off, heldNote, false });
                heldNote = -1;
                pendingOff = -1.0;
            }
        }

        void handleStep (int64_t k, double t, double stepBeats, int baseNote, int sample, NoteEventList& out) noexcept
        {
            // Position dans le pattern : compteur continu, recalé si la suite des pas est rompue
            const Pattern* pat = &bank.patterns[active];
            if (lastK != INT64_MIN && k == lastK + 1 && stepIdx >= 0)
                stepIdx = (stepIdx + 1) % std::max (1, pat->length);
            else
                stepIdx = (int) (((k % pat->length) + pat->length) % pat->length);
            lastK = k;

            // Début de cycle : on applique le pattern demandé
            if (stepIdx == 0 && requested != active)
            {
                active = requested;
                pat = &bank.patterns[active];
            }

            const int len = std::max (1, pat->length);
            const Step& st = pat->steps[stepIdx];
            const Step& next = pat->steps[(stepIdx + 1) % len];
            const int pitch = std::clamp (baseNote + st.octave * 12 + (int) st.note + transpose, 0, 127);

            switch (st.gate)
            {
                case Gate::Note:
                {
                    if (heldNote >= 0 && prevSlide)
                    {
                        if (pitch != heldNote)
                        {
                            out.add ({ sample, NoteEvent::On, pitch, st.accent });    // nouvelle note d'abord...
                            out.add ({ sample, NoteEvent::Off, heldNote, false });    // ...puis l'ancienne : slide
                        }
                    }
                    else
                    {
                        if (heldNote >= 0)
                            out.add ({ sample, NoteEvent::Off, heldNote, false });
                        out.add ({ sample, NoteEvent::On, pitch, st.accent });
                    }
                    heldNote = pitch;
                    pendingOff = -1.0;
                    if (! st.slide && next.gate != Gate::Tie)
                        pendingOff = t + gateLen * stepBeats;
                    prevSlide = st.slide;
                    break;
                }

                case Gate::Tie:
                {
                    if (heldNote < 0) { prevSlide = false; break; }           // rien à prolonger
                    pendingOff = -1.0;
                    if (! st.slide && next.gate != Gate::Tie)
                        pendingOff = t + gateLen * stepBeats;
                    prevSlide = st.slide;
                    break;
                }

                case Gate::Rest:
                default:
                {
                    if (heldNote >= 0)
                    {
                        out.add ({ sample, NoteEvent::Off, heldNote, false });
                        heldNote = -1;
                    }
                    pendingOff = -1.0;
                    prevSlide = false;
                    break;
                }
            }
        }

        PatternBank bank {};
        double gateLen = 0.5;
        int active = 0, requested = 0, transpose = 0;

        double expectedPpq = -1.0e9;
        bool haveTiming = false;
        int heldNote = -1;
        double pendingOff = -1.0;
        bool prevSlide = false;
        int stepIdx = -1;
        int64_t lastK = INT64_MIN;
    };
}

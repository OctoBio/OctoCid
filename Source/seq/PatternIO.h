#pragma once

#include <JuceHeader.h>
#include "Pattern.h"

//==============================================================================
// Sauvegarde des patterns dans l'état du plugin et export MIDI.
//==============================================================================
namespace octocid::PatternIO
{
    // Un pas = 5 caractères : note (hex), octave (d/0/u), accent (a/.), slide (s/.), gate (n/-/t)
    inline juce::String encode (const Pattern& p)
    {
        juce::String s;
        s.preallocateBytes (Pattern::kMaxSteps * 5 + 8);
        for (const auto& st : p.steps)
        {
            s << juce::String::toHexString ((int) st.note).getLastCharacters (1);
            s << (st.octave < 0 ? 'd' : (st.octave > 0 ? 'u' : '0'));
            s << (st.accent ? 'a' : '.');
            s << (st.slide ? 's' : '.');
            s << (st.gate == Gate::Rest ? '-' : (st.gate == Gate::Tie ? 't' : 'n'));
        }
        return s;
    }

    inline bool decode (const juce::String& s, int length, Pattern& out)
    {
        if (s.length() < Pattern::kMaxSteps * 5)
            return false;
        Pattern p;
        p.length = juce::jlimit (1, Pattern::kMaxSteps, length);
        for (int i = 0; i < Pattern::kMaxSteps; ++i)
        {
            const auto c = s.substring (i * 5, i * 5 + 5);
            Step st;
            st.note = (uint8_t) juce::jlimit (0, 11, c.substring (0, 1).getHexValue32());
            st.octave = (int8_t) (c[1] == 'd' ? -1 : (c[1] == 'u' ? 1 : 0));
            st.accent = c[2] == 'a';
            st.slide = c[3] == 's';
            st.gate = c[4] == '-' ? Gate::Rest : (c[4] == 't' ? Gate::Tie : Gate::Note);
            p.steps[i] = st;
        }
        out = p;
        return true;
    }

    inline juce::ValueTree toTree (const PatternBank& bank)
    {
        juce::ValueTree t ("Patterns");
        for (int i = 0; i < PatternBank::kNumPatterns; ++i)
        {
            juce::ValueTree pt ("Pattern");
            pt.setProperty ("index", i, nullptr);
            pt.setProperty ("length", bank.patterns[i].length, nullptr);
            pt.setProperty ("steps", encode (bank.patterns[i]), nullptr);
            t.appendChild (pt, nullptr);
        }
        return t;
    }

    inline bool fromTree (const juce::ValueTree& t, PatternBank& bank)
    {
        if (! t.hasType ("Patterns"))
            return false;
        PatternBank b = bank;
        for (const auto& pt : t)
        {
            const int i = pt.getProperty ("index", -1);
            if (i >= 0 && i < PatternBank::kNumPatterns)
                decode (pt.getProperty ("steps").toString(), pt.getProperty ("length", 16), b.patterns[i]);
        }
        bank = b;
        return true;
    }

    //==========================================================================
    // Export MIDI d'un pattern : accents = vélocité haute, slides = notes qui
    // se chevauchent (comme le jeu au clavier), ties = note prolongée.
    // stepBeats : durée d'un pas en noires (0,25 pour des doubles-croches).
    //==========================================================================
    inline juce::MidiMessageSequence toSequence (const Pattern& p, double stepBeats, int baseNote, int ppqRes, double gate = 0.5)
    {
        juce::MidiMessageSequence seq;
        const double stepTicks = stepBeats * ppqRes;
        const int n = p.length;

        int i = 0;
        while (i < n)
        {
            const Step& st = p.steps[i];
            if (st.gate != Gate::Note) { ++i; continue; }

            const int note = juce::jlimit (0, 127, baseNote + st.octave * 12 + st.note);
            const double on = i * stepTicks;

            // La note dure jusqu'à la fin du dernier pas lié (ties). Un slide vers la même
            // hauteur se comporte comme une liaison (deux notes identiques qui se chevauchent
            // se couperaient l'une l'autre dans le DAW).
            auto pitchOf = [&] (const Step& s) { return baseNote + s.octave * 12 + s.note; };
            int last = i;
            while (last + 1 < n)
            {
                const Step& nx = p.steps[last + 1];
                const bool tie = nx.gate == Gate::Tie;
                const bool sameSlide = p.steps[last].slide && nx.gate == Gate::Note && pitchOf (nx) == note;
                if (! tie && ! sameSlide) break;
                ++last;
            }
            const Step& lastStep = p.steps[last];
            const bool slideToNext = lastStep.slide && last + 1 < n && p.steps[last + 1].gate == Gate::Note;

            const double off = slideToNext ? (last + 1) * stepTicks + stepTicks * 0.25   // chevauche la suivante
                                           : (last + gate) * stepTicks;                   // longueur de gate

            seq.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) (st.accent ? 127 : 80)), on);
            seq.addEvent (juce::MidiMessage::noteOff (1, note), off);
            i = last + 1;
        }
        seq.updateMatchedPairs();
        return seq;
    }

    inline juce::File writeMidiFile (const Pattern& p, double stepBeats, int baseNote, const juce::String& name, double gate = 0.5)
    {
        constexpr int ppqRes = 960;
        juce::MidiFile mf;
        mf.setTicksPerQuarterNote (ppqRes);

        auto seq = toSequence (p, stepBeats, baseNote, ppqRes, gate);
        // Fin de piste à la fin du pattern, pour que la boucle tombe juste
        seq.addEvent (juce::MidiMessage::endOfTrack(), p.length * stepBeats * ppqRes);
        mf.addTrack (seq);

        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("OctoCid");
        dir.createDirectory();
        auto file = dir.getChildFile (name + ".mid");
        file.deleteFile();
        juce::FileOutputStream os (file);
        if (os.openedOk())
            mf.writeTo (os);
        return file;
    }
}

#pragma once

#include <utility>

//==============================================================================
// Presets d'usine (son seulement : les patterns du séquenceur ne sont pas touchés).
// Chaque preset part des valeurs par défaut, puis applique ses réglages, en
// unités réelles (%, ms, dB, index de choix).
//==============================================================================
namespace FactoryPresets
{
    struct Value  { const char* id; float v; };
    struct Preset { const char* name; const char* tag; Value values[32]; };

    // Les listes se terminent par { nullptr, 0 }.
    inline const Preset list[] =
    {
        { "Init 303", "Le son d'origine, tous les mods au repos", { { nullptr, 0 } } },

        { "Acid House 88", "Ligne house classique, ouverte et chantante, sans effet",
          { { "cutoff", 42 }, { "resonance", 70 }, { "envmod", 48 }, { "decay", 68 }, { "accent", 60 },
            { "slidetime", 90 }, { "punch", 30 }, { "volume", 0.1 }, { nullptr, 0 } } },

        { "Squelch Machine", "Résonance au bord du sifflement, accents qui montent",
          { { "cutoff", 6 }, { "resonance", 97 }, { "envmod", 96 }, { "decay", 18 }, { "accent", 100 },
            { "sweepspeed", 0 }, { "slidepunch", 30 }, { "slidetime", 110 },
            { "drivetype", 1 }, { "drive", 40 }, { "drivetone", 65 }, { "drivemix", 60 }, { "punch", 35 }, { "revmix", 8 }, { "revdecay", 30 }, { "volume", -1.4 }, { nullptr, 0 } } },

        { "Rat Techno", "Acid techno qui crache, disto rat",
          { { "cutoff", 30 }, { "resonance", 82 }, { "envmod", 70 }, { "decay", 35 }, { "accent", 85 },
            { "slidepunch", 40 }, { "slidetime", 120 },
            { "drivetype", 2 }, { "drive", 70 }, { "drivetone", 55 }, { "drivemix", 100 }, { "punch", 50 }, { "punchattack", 15 }, { "dlymix", 12 }, { "dlytime", 3 }, { "dlyfb", 30 }, { "volume", -2.4 }, { nullptr, 0 } } },

        { "Screamer Lead", "Lead acid percutant, une octave au-dessus",
          { { "tune", 12 }, { "waveform", 1 }, { "cutoff", 40 }, { "resonance", 86 }, { "envmod", 80 },
            { "decay", 10 }, { "accent", 100 }, { "accentdecay", 100 },
            { "slidetime", 170 }, { "slidepunch", 70 }, { "sweepspeed", 0 }, { "filterfm", 15 },
            { "drivetype", 2 }, { "drive", 75 }, { "drivetone", 72 }, { "drivemix", 100 }, { "drivepos", 1 }, { "punch", 45 }, { "punchattack", 12 }, { "dlymix", 18 }, { "dlytime", 3 }, { "dlyfb", 38 }, { "revmix", 10 }, { "revdecay", 40 }, { "volume", -4.5 }, { nullptr, 0 } } },

        { "Rave Wobble", "Basse carrée qui balance en croches, filtre au LFO",
          { { "waveform", 1 }, { "cutoff", 30 }, { "resonance", 82 }, { "envmod", 22 }, { "decay", 75 },
            { "accent", 60 },
            { "lfosync", 4 }, { "lfoshape", 1 }, { "lfodest", 0 }, { "lfodepth", 65 },
            { "drivetype", 1 }, { "drive", 50 }, { "drivetone", 60 }, { "drivemix", 100 }, { "punch", 40 }, { "volume", -2 }, { nullptr, 0 } } },

        { "Rubber Sub", "Basse ronde rap / trap, sub et glides lents",
          { { "waveform", 1 }, { "cutoff", 8 }, { "resonance", 20 }, { "envmod", 22 }, { "decay", 65 },
            { "accent", 40 }, { "slidetime", 220 }, { "subosc", 1 }, { "sublevel", 70 }, { "muffler", 1 },
            { "volume", -3.9 }, { nullptr, 0 } } },

        { "Deep Dub Acid", "Long decay, filtre qui respire lentement",
          { { "cutoff", 18 }, { "resonance", 62 }, { "envmod", 38 }, { "decay", 85 }, { "accent", 50 },
            { "muffler", 2 }, { "slidetime", 100 },
            { "lfosync", 2 }, { "lfoshape", 0 }, { "lfodest", 0 }, { "lfodepth", 28 }, { "volume", 6.2 }, { "dlymix", 26 }, { "dlytime", 6 }, { "dlyfb", 58 }, { "revmix", 12 }, { "revdecay", 55 }, { "revtone", 35 }, { nullptr, 0 } } },

        { "Liquid Glide", "Glides longs et chantants, résonance liquide",
          { { "cutoff", 14 }, { "resonance", 93 }, { "envmod", 40 }, { "decay", 82 }, { "accent", 55 },
            { "slidetime", 320 }, { "slidepunch", 50 }, { "sweepspeed", 0 }, { "volume", 2.9 }, { "revmix", 18 }, { "revdecay", 70 }, { "revtone", 60 }, { nullptr, 0 } } },

        { "Pluck Stab", "Stab sec et court, disto avant le VCA, suit le clavier",
          { { "tune", 12 }, { "waveform", 1 }, { "cutoff", 40 }, { "resonance", 40 }, { "envmod", 96 }, { "decay", 0 },
            { "accent", 80 }, { "accentdecay", 40 }, { "softattack", 0.3f }, { "filtertrack", 70 },
            { "drivetype", 1 }, { "drive", 65 }, { "drivetone", 80 }, { "drivemix", 100 }, { "drivepos", 1 }, { "punch", 50 }, { "punchattack", 8 }, { "dlymix", 15 }, { "dlytime", 0 }, { "dlyfb", 30 }, { "volume", -2.1 }, { nullptr, 0 } } },

        { "FM Monster", "Filter FM et disto : chaotique et mordant",
          { { "cutoff", 30 }, { "resonance", 85 }, { "envmod", 65 }, { "decay", 35 }, { "accent", 85 },
            { "filterfm", 75 }, { "filterdrive", 35 },
            { "drivetype", 2 }, { "drive", 50 }, { "drivetone", 40 }, { "drivemix", 75 }, { "punch", 45 }, { "volume", -1.7 }, { nullptr, 0 } } },

        { "Trance Lead", "Lead saw qui chante, vibrato léger",
          { { "tune", 12 }, { "cutoff", 44 }, { "resonance", 70 }, { "envmod", 55 }, { "decay", 55 },
            { "accent", 70 }, { "slidetime", 180 }, { "slidepunch", 50 }, { "softattack", 8 },
            { "lfosync", 0 }, { "lforate", 5.5f }, { "lfoshape", 0 }, { "lfodest", 2 }, { "lfodepth", 8 },
            { "drivetype", 1 }, { "drive", 35 }, { "drivetone", 75 }, { "drivemix", 70 }, { "volume", 1.4 }, { "dlymix", 22 }, { "dlytime", 3 }, { "dlyfb", 45 }, { "revmix", 15 }, { "revdecay", 60 }, { "revtone", 65 }, { nullptr, 0 } } },

        { "Broken Speaker", "Lo-fi saturé, sombre et étouffé",
          { { "waveform", 1 }, { "cutoff", 25 }, { "resonance", 65 }, { "envmod", 70 }, { "decay", 40 },
            { "accent", 80 }, { "muffler", 2 },
            { "drivetype", 2 }, { "drive", 100 }, { "drivetone", 15 }, { "drivemix", 100 }, { "volume", 2.9 }, { nullptr, 0 } } },

        { "S&H Madness", "Cutoff aléatoire en doubles-croches",
          { { "cutoff", 36 }, { "resonance", 90 }, { "envmod", 30 }, { "decay", 55 }, { "accent", 75 },
            { "lfosync", 5 }, { "lfoshape", 3 }, { "lfodest", 0 }, { "lfodepth", 75 }, { "volume", 2 }, { "dlymix", 14 }, { "dlytime", 2 }, { "dlyfb", 35 }, { nullptr, 0 } } },

        { "Siren Track", "La résonance siffle et suit les notes",
          { { "tune", 12 }, { "cutoff", 20 }, { "resonance", 97 }, { "envmod", 18 },
            { "decay", 95 }, { "accent", 60 }, { "filtertrack", 100 }, { "filterdrive", 70 },
            { "slidetime", 260 }, { "slidepunch", 60 }, { "volume", -2.2 }, { "dlymix", 15 }, { "dlytime", 6 }, { "dlyfb", 40 }, { "revmix", 20 }, { "revdecay", 75 }, { nullptr, 0 } } },
        { "Hardfloor Spiral", "Acid dur et carré, accents qui claquent, écho pointé",
          { { "waveform", 1 }, { "cutoff", 26 }, { "resonance", 92 }, { "envmod", 85 }, { "decay", 22 }, { "accent", 100 },
            { "sweepspeed", 2 }, { "slidetime", 80 },
            { "drivetype", 1 }, { "drive", 60 }, { "drivetone", 60 }, { "drivemix", 100 }, { "punch", 70 }, { "punchattack", 6 },
            { "dlymix", 20 }, { "dlytime", 3 }, { "dlyfb", 42 }, { "volume", -0.8 }, { nullptr, 0 } } },

        { "Minimal Click", "Très court et sec, juste un clic de filtre",
          { { "cutoff", 22 }, { "resonance", 55 }, { "envmod", 30 }, { "decay", 0 }, { "accent", 70 },
            { "accentdecay", 60 }, { "softattack", 0.3f }, { "punch", 60 }, { "punchattack", 4 }, { "volume", 1.7 }, { nullptr, 0 } } },

        { "Goa Roller", "Psytrance qui roule, suit le clavier, écho de croches",
          { { "tune", 12 }, { "cutoff", 40 }, { "resonance", 76 }, { "envmod", 55 }, { "decay", 2 }, { "accent", 90 }, { "sweepspeed", 2 }, { "filtertrack", 100 }, { "slidetime", 60 }, { "drivetype", 1 }, { "drive", 30 }, { "drivetone", 70 }, { "drivemix", 60 }, { "dlymix", 26 }, { "dlytime", 2 }, { "dlyfb", 60 }, { "revmix", 10 }, { "revdecay", 45 }, { "volume", 1.7 }, { nullptr, 0 } } },

        { "Ghetto Acid", "Sale et lourd : sub, disto avant le VCA, filtre fermé",
          { { "cutoff", 12 }, { "resonance", 80 }, { "envmod", 95 }, { "decay", 25 }, { "accent", 100 }, { "sweepspeed", 2 }, { "subosc", 1 }, { "sublevel", 55 }, { "drivetype", 1 }, { "drive", 85 }, { "drivetone", 50 }, { "drivemix", 100 }, { "drivepos", 1 }, { "punch", 40 }, { "volume", -0.2 }, { nullptr, 0 } } },

        { "Bubble Bath", "La résonance fait des bulles au rythme du LFO",
          { { "waveform", 1 }, { "cutoff", 36 }, { "resonance", 72 }, { "envmod", 35 }, { "decay", 60 }, { "accent", 55 }, { "lfosync", 6 }, { "lfoshape", 1 }, { "lfodest", 1 }, { "lfodepth", 100 }, { "revmix", 16 }, { "revdecay", 50 }, { "revtone", 70 }, { "volume", -1.0 }, { nullptr, 0 } } },

        { "Laser Zap", "Balayage éclair sur chaque note, très brillant",
          { { "tune", 12 }, { "cutoff", 4 }, { "resonance", 88 }, { "envmod", 100 }, { "decay", 4 }, { "accent", 100 },
            { "sweepspeed", 2 }, { "filterfm", 30 }, { "drivetype", 2 }, { "drive", 40 }, { "drivetone", 80 }, { "drivemix", 50 },
            { "dlymix", 20 }, { "dlytime", 0 }, { "dlyfb", 50 }, { "volume", -0.4 }, { nullptr, 0 } } },

        { "Analog Drift", "Chaud et vivant : dérive analogique, étouffé, glides doux",
          { { "tune", 12 }, { "cutoff", 30 }, { "resonance", 62 }, { "envmod", 40 }, { "decay", 72 }, { "accent", 50 }, { "analog", 100 }, { "muffler", 1 }, { "slidetime", 220 }, { "softattack", 15 }, { "revmix", 14 }, { "revdecay", 45 }, { "revtone", 40 }, { "volume", 4.8 }, { nullptr, 0 } } },

        { "Industrial Grind", "Carré broyé : drive de filtre, FM et disto rat sombre",
          { { "waveform", 1 }, { "cutoff", 45 }, { "resonance", 74 }, { "envmod", 82 }, { "decay", 20 }, { "accent", 90 }, { "filterdrive", 100 }, { "filterfm", 60 }, { "lfosync", 4 }, { "lfoshape", 2 }, { "lfodest", 0 }, { "lfodepth", 45 }, { "drivetype", 2 }, { "drive", 85 }, { "drivetone", 85 }, { "drivemix", 100 }, { "drivepos", 1 }, { "punch", 70 }, { "volume", -2.1 }, { nullptr, 0 } } },

        { "Space Echo Arp", "Notes courtes perdues dans l'écho et la réverbe",
          { { "waveform", 1 }, { "cutoff", 50 }, { "resonance", 66 }, { "envmod", 50 }, { "decay", 3 }, { "accent", 70 }, { "dlymix", 40 }, { "dlytime", 3 }, { "dlyfb", 72 }, { "revmix", 30 }, { "revdecay", 85 }, { "revtone", 55 }, { "volume", -1.4 }, { nullptr, 0 } } },
    };

    inline constexpr int count = (int) (sizeof (list) / sizeof (list[0]));
}

# OctoCid

Synthé acid VST3 par **OctoBio** — émulation fidèle d'une bassline acid classique, séquenceur intégré, tiroir de mods modernes et une mascotte escargot.

> Statut : étape 4 (séquenceur). Jouable au clavier, au piano roll ou avec le séquenceur intégré, avec une interface provisoire.

## Jeu au clavier

- Notes qui se chevauchent = **slide** (glissement sans redéclencher les enveloppes)
- Vélocité ≥ seuil (100 par défaut, réglable) = **accent**
- Qualité : Normal (suréchantillonnage 2x) ou High (4x)

## Séquenceur (onglet SÉQUENCEUR)

- **SEQ ON** : le séquenceur joue en rythme avec le DAW (tempo et transport). Une note MIDI transpose
  le pattern (Do3 = 0). Les notes Do0 à Sol0 choisissent les patterns A à H, au cycle suivant.
- **SEQ OFF** : le synthé se joue au clavier ou au piano roll.
- 8 patterns (A–H) de 1 à 32 pas, résolution 1/16, 1/8, 1/32, 1/16T, 1/8T, swing 0 à 75 %.
- Par pas : note, octave (Down / 0 / Up), accent, slide, gate (Note / Silence / Liaison).
- Outils : Randomize avec gamme et densités d'accents / slides, décalage, transposition, copier / coller / effacer.
- **Glisser-déposer MIDI** : faire glisser la zone « MIDI » vers une piste de Bitwig crée le clip du pattern
  (accents = vélocité haute, slides = notes qui se chevauchent).
- Les patterns sont sauvegardés avec le projet.

## Sections

- **303** : Tune, Cutoff, Resonance, Env Mod, Decay, Accent, Waveform, Volume
- **Drive** : Off / Overdrive / Distortion (modèle de circuit d'une pédale « rat »), Drive, Tone, Mix,
  Position *Après* le VCA (comme une pédale en sortie) ou *Avant* (le volume sculpte la disto : reste percutant)
- **LFO** : libre ou synchronisé au tempo (1/1 à 1/32, pointé, triolet), Sine / Triangle / Square / S&H,
  vers Cutoff, Resonance ou Pitch
- **Punch + FX** : Punch (compresseur pensé pour l'acid : les accents claquent, la ligne devient dense),
  Attaque ; Delay ping-pong synchronisé (1/16 à 1/2, pointé, triolet) avec Feedback ; Reverb plate
  (Mix, Decay, Tone). Delay et reverb coupent le grave : la basse reste sèche et serrée
- **Slime Mods** : Slide Time, Slide Punch, Sweep Speed, Accent Decay, Soft Attack, Filter Tracking,
  Filter FM, Filter Drive, Sub Osc, Muffler — à leur réglage par défaut, le son est celui de l'original

## Presets d'usine

15 presets, tous au même niveau perçu (-23 LUFS sur une ligne de démo, sauf Init) et vérifiés
comme distincts par analyse (brillance, grave, dynamique, saturation, mouvement du filtre) :

Init 303 · Acid House 88 · Squelch Machine · Rat Techno · Screamer Lead · Rave Wobble · Rubber Sub ·
Deep Dub Acid · Liquid Glide · Pluck Stab · FM Monster · Trance Lead · Broken Speaker · S&H Madness · Siren Track

## Build local (Windows)

```bash
cmake -S . -B build -A x64
cmake --build build --config Release
```

Le VST3 se trouve dans `build/OctoCid_artefacts/Release/VST3/OctoCid.vst3`.
Copie-le dans `C:\Program Files\Common Files\VST3\`.

## CI

Chaque push sur `main` lance GitHub Actions (Windows) et publie deux artefacts : le `.vst3` et le Standalone `.exe`.

## Feuille de route

1. Squelette ✅
2. Moteur sonore (osc, filtre diode ladder, enveloppes, accent, slide) ✅
3. Drive, LFO, Slime Mods ✅ (+ Punch, Delay, Reverb, presets d'usine son)
4. Séquenceur + sync DAW + export MIDI ✅
5. Interface
6. Escargot animé
7. Presets avec patterns, finitions, pluginval

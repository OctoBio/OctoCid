# OctoCid

Synthé acid VST3 par **OctoBio** — émulation fidèle d'une bassline acid classique, séquenceur intégré, tiroir de mods modernes et une mascotte escargot.

> Version 1.0. Jouable au clavier, au piano roll ou avec le séquenceur intégré.

## Interface

Fenêtre redimensionnable (tirer le coin en bas à droite, ou menu « ... » > Taille, de 60 à 200 %), taille gardée avec le projet :

1. **Barre du haut** : logo, presets, SEQ ON/OFF, animation de l'escargot, qualité Normal / High, menu (presets, apparence, taille, patterns d'usine).
2. **Drive** à gauche, **mascotte** au centre, **LFO** à droite.
3. **Panneau 303** (Tune, Cutoff, Resonance, Env Mod, Decay, Accent, Waveform, Volume) et le bouton du tiroir
   **SLIME MODS**, qui se déplie au-dessus (réglages avancés + Punch / Delay / Reverb) sans masquer le séquenceur.
4. **Séquenceur** sur toute la largeur.

Potards : valeur affichée au survol et pendant le réglage, double-clic = valeur par défaut,
Ctrl + glisser = réglage fin, molette. Fond « portail » illustré, panneaux en verre dépoli, style glitch.

## L'escargot

Il réagit au son (30 images/s, seulement sa zone est redessinée, le corps est mis en cache) :
pupilles qui se dilatent avec le **Cutoff**, yeux qui tremblent avec la **Resonance**, flaque qui grandit avec
l'**Env Mod**, bave qui s'allonge avec le **Decay**, saut sur les **accents**, glissade et traînée lumineuse sur les
**slides**, coquille qui chauffe et transpire avec le **Drive**, pédoncules qui ondulent avec le **LFO** et penchent
avec le **Tune**. Il cligne des yeux, et s'endort après 20 s sans note. Bouton « Escargot » pour le figer (sauvegardé).

## Jeu au clavier

- Notes qui se chevauchent = **slide** (glissement sans redéclencher les enveloppes)
- Vélocité ≥ seuil (100 par défaut, réglable) = **accent**
- Qualité : Normal (suréchantillonnage 2x) ou High (4x)

## Séquenceur

- **SEQ ON** : le séquenceur joue en rythme avec le DAW (tempo et transport). Une note MIDI transpose
  le pattern (Do3 = 0). Les notes Do0 à Sol0 choisissent les patterns A à H, au cycle suivant.
- **SEQ OFF** : le synthé se joue au clavier ou au piano roll.
- 8 patterns (A–H) de 1 à 32 pas, résolution 1/16, 1/8, 1/32, 1/16T, 1/8T, swing 0 à 75 %.
- Par pas : note, octave (Down / 0 / Up), accent, slide, gate (Note / Silence / Liaison).
- **Générer** : nouvelle ligne acid dans la gamme, densité de -7 (tonique martelée) à +7 (toute la gamme, ligne pleine),
  dosage des accents et des slides ; règles « 303 » (tonique dominante, slides vers une note différente…).
- **Varier** : fait évoluer la ligne actuelle (décale accents ou slides, inverse, échange les moitiés, change quelques notes).
- **Gamme** : ramène toutes les notes dans la gamme choisie.
- Décalage d'une seule ligne (notes, accents, slides ou gates), décalage global, transposition, copier / coller / effacer,
  annuler / rétablir.
- **REC** : enregistrement pas à pas au clavier MIDI (vélocité haute = accent, notes liées = slide).
- **Raccourcis clavier** (comme le Transistor Bass) : flèches, Maj + haut/bas (octave), A, S, G, N, R, T, Ctrl+Z / Ctrl+Y.
- **Gate** : longueur des notes (10 à 95 %, 50 % comme l'original).
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
  Filter FM, Filter Drive, Sub Osc, Muffler, Analog (dérive lente + rampe d'oscillateur imparfaite) —
  à leur réglage par défaut, le son est celui de l'original

## Mes presets

Menu « ... » > **Enregistrer le preset…** : le son actuel est sauvegardé dans `Documents\OctoBio\OctoCid\Presets`
(fichiers `.ocpreset`, faciles à partager). Ils apparaissent sous « MES PRESETS » dans la liste et dans le DAW ;
« Supprimer » et « Ouvrir le dossier des presets » sont dans le même menu.

## Apparence

Menu « ... » > **Apparence** : 16 fonds (portails violet, vert, feu, bleu, orage, acide…), 16 styles d'interface
(glitch, épurés, BD), 16 potards, 9 escargots. Choix gardé pour toutes les fenêtres ; « Revenir à l'apparence d'origine ».

## Presets d'usine

24 presets de son, tous au même niveau perçu (-23 LUFS sur une ligne de démo, sauf Init) et vérifiés
comme distincts par analyse (brillance, grave, dynamique, saturation, mouvement du filtre).
Ils ne touchent jamais aux patterns du séquenceur :

Init 303 · Acid House 88 · Squelch Machine · Rat Techno · Screamer Lead · Rave Wobble · Rubber Sub ·
Deep Dub Acid · Liquid Glide · Pluck Stab · FM Monster · Trance Lead · Broken Speaker · S&H Madness · Siren Track ·
Hardfloor Spiral · Minimal Click · Goa Roller · Ghetto Acid · Bubble Bath · Laser Zap · Analog Drift ·
Industrial Grind · Space Echo Arp

## Banques de patterns d'usine

Menu « ... » > **Patterns d'usine** : 7 banques de 8 patterns (Classiques, Acid House, Acid Techno, Trance / Goa,
Glides, Hard Acid, Rap / Trap). Charger une banque remplace A à H ; « Revenir à mes patterns d'avant » annule.

## Validation

Tests internes à chaque version : automatisation aléatoire, toutes tailles de buffer (32 à 2048) et fréquences
(44,1 à 192 kHz), sauvegarde / rappel d'état, presets, séquenceur synchronisé, export MIDI, coût de l'interface.
Pour une validation pluginval (Tracktion) sous Windows :

```
pluginval.exe --strictness-level 5 --validate "C:\Program Files\Common Files\VST3\OctoCid.vst3"
```

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
5. Interface ✅ (fond portail, style glitch, potards néon)
6. Escargot animé ✅
7. Presets, banques de patterns, finitions ✅ (pluginval : voir ci-dessous)

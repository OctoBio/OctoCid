# OctoCid

Synthé acid VST3 par **OctoBio** — émulation fidèle d'une bassline acid classique, séquenceur intégré, tiroir de mods modernes et une mascotte animée (blob gluant ou escargot).

> Version 1.6.1. Jouable au clavier, au piano roll ou avec le séquenceur intégré.

## ⬇️ Télécharger

👉 **[Dernière version (VST3 + Standalone, Windows 64 bits)](https://github.com/OctoBio/OctoCid/releases/latest)** — aucun compte GitHub nécessaire.

1. Télécharger `OctoCid-…-VST3-Windows.zip` et le dézipper
2. Copier le dossier `OctoCid.vst3` dans `C:\Program Files\Common Files\VST3`
3. Relancer le DAW (ou relancer le scan des plugins)

Si Windows affiche « Windows a protégé votre ordinateur » (plugin non signé) : *Informations complémentaires* > *Exécuter quand même*.

## Interface

Fenêtre redimensionnable (tirer le coin en bas à droite, ou menu « ... » > Taille, de 60 à 200 %), taille gardée avec le projet :

1. **Barre du haut** : logo, presets, SEQ ON/OFF, animation de la mascotte, qualité Normal / High, menu (presets, apparence, taille, patterns d'usine).
2. **Drive** à gauche, **mascotte** au centre, **3 LFO** à droite (onglets 1 / 2 / 3).
3. **Panneau 303** (Tune, Cutoff, Resonance, Env Mod, Decay, Accent, Waveform, Volume) et le bouton du tiroir
   les boutons du tiroir **SLIME MODS** (réglages avancés + Punch / Delay / Reverb), **EFFETS** et **MOD**
   (les 8 modulations), qui se déplie au-dessus sans masquer le séquenceur.
4. **Séquenceur** sur toute la largeur.

Potards : valeur affichée au survol et pendant le réglage, double-clic = valeur par défaut,
Ctrl + glisser = réglage fin, molette. Fond « portail » illustré, panneaux en verre dépoli, style glitch.

## La mascotte

**Blob gluant** (par défaut) : peau calculée pixel par pixel (relief, couche de mucus luisante, lumière qui
traverse les bords). Il se balade dans tous les sens, éclate comme une goutte d'eau sur les 4 axes à chaque note (avec des gouttelettes projetées), tremble
comme de la gelée et saute sur les accents, sans jamais changer de taille. **Accent** : il bave (filets, gouttes, flaque) ; **Resonance** : couleur ;
**Cutoff** : fermé il fond et s'étale, ouvert il devient nerveux, et ses pupilles se dilatent ; **Reverb** : il se
tord en spirale, laisse des échos fantômes et des ondes ; **Drive** : bulles et cœur qui chauffe ; **Env Mod** : ses
cellules tournent ; **LFO** : il se balance. Menu « ... » > Apparence > Mascotte pour revenir à l'escargot.

### L'escargot

Il réagit au son (30 images/s, seulement sa zone est redessinée, le corps est mis en cache) :
pupilles qui se dilatent avec le **Cutoff**, yeux qui tremblent avec la **Resonance**, flaque qui grandit avec
l'**Env Mod**, bave qui s'allonge avec le **Decay**, saut sur les **accents**, glissade et traînée lumineuse sur les
**slides**, coquille qui chauffe et transpire avec le **Drive**, pédoncules qui ondulent avec le **LFO** et penchent
avec le **Tune**. Il cligne des yeux, et s'endort après 20 s sans note. Bouton « Mascotte » pour la figer (sauvegardé).

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
- **3 LFO assignables** à n'importe quel potard (36 destinations, y compris les effets et la vitesse des autres LFO) :
  - 7 formes : Sinus, Triangle, Scie +, Scie -, Carré, S&H, Aléatoire lissé ;
  - Vitesse libre (0,02 à 30 Hz) ou calée sur le tempo (4 mesures à 1/32, pointé, triolet) ;
  - 5 modes : *Libre*, *Note* (repart à chaque note détachée, pas sur un slide), *Synchro* (calé sur la position
    du DAW : identique à chaque lecture), *Un coup* (un seul cycle, comme une enveloppe), *Aléatoire* (phase au hasard) ;
  - Phase de départ, Délai puis Montée (fondu d'entrée), Lissage (sans saut quand le LFO repart), bipolaire / unipolaire.
  - **Formes dessinées à la souris** (façon Serum / Vital) : double-clic pour ajouter ou supprimer un point, glisser un
    point pour le déplacer (aimanté à la grille, Ctrl : sans aimant, Maj : réglage fin), glisser une courbe pour la bomber.
    **Grand éditeur** (bouton du panneau LFO) : 16 formes prêtes (pompe, rebond, escaliers, porte trance, wobble…),
    grille X / Y, aimant, pinceau *Marches* / *Rampes* (comme un step séquenceur), inverser, miroir, annuler, copier / coller.
    Les formes sont gardées avec le projet et dans les presets utilisateur.
  - **Assigner** : attraper un onglet 1 / 2 / 3 (ou « ✥ LFO n ») et le lâcher sur un potard, ou clic droit sur un potard > *Moduler par LFO*.
    Un arc coloré montre la plage balayée, un point blanc suit la valeur réelle. **Alt + glisser** sur le potard
    règle la quantité. La page **MOD** liste les 8 emplacements (source, destination, quantité, ×).
  - Les presets faits avec l'ancien LFO sont convertis automatiquement.
- **Punch + FX** : Punch (compresseur pensé pour l'acid : les accents claquent, la ligne devient dense),
  Attaque ; Delay ping-pong synchronisé (1/16 à 1/2, pointé, triolet) avec Feedback ; Reverb plate
  (Mix, Decay, Tone). Delay et reverb coupent le grave : la basse reste sèche et serrée
- **Slime Mods** : Slide Time, Slide Punch, Sweep Speed, Accent Decay, Soft Attack, Filter Tracking,
  Filter FM, Filter Drive, Sub Osc, Muffler, Analog (dérive lente + rampe d'oscillateur imparfaite) —
  à leur réglage par défaut, le son est celui de l'original

## Page EFFETS

Bouton **EFFETS** (sous SLIME MODS) : effets maison pensés pour rendre l'acid plus percutant ou plus planant,
tous à zéro par défaut (son sec), environ 2 % d'un cœur CPU tous activés :

- **FRAPPE** : **Impact** (façonneur de transitoires : l'attaque de chaque note claque, jusqu'à +10 dB, sans gonfler tout le son) et
  **Smash** (compression 3 bandes vers le haut et vers le bas, esprit « OTT » : détails remontés, ligne dense et brillante)
- **JET** : flanger ou phaser stéréo synchronisé au tempo (4 mesures à 1/8), avec feedback : le mouvement qui « serpente »
- **PUMP** : pompe rythmique (effet sidechain) au 1/4, 1/8, 1/2 ou 1/4 pointé
- **STEREO** : **Width**, élargisseur compatible mono (le grave reste au centre)

Ordre : Impact → Punch → Smash → Jet → Delay → Reverb → Pump → Width.

## Mes presets

Menu « ... » > **Enregistrer le preset…** : le son actuel est sauvegardé dans `Documents\OctoBio\OctoCid\Presets`
(fichiers `.ocpreset`, faciles à partager). Ils apparaissent sous « MES PRESETS » dans la liste et dans le DAW ;
« Supprimer » et « Ouvrir le dossier des presets » sont dans le même menu.

## Apparence

Menu « ... » > **Apparence** : 17 fonds (portails violet, vert, feu, bleu, orage, acide, cosmos…) + **ta propre image** (*Fond > Choisir une image sur mon ordi…* : elle reste sur ton ordinateur, jamais intégrée au plugin), 16 styles d'interface
(glitch, épurés, BD), 16 potards, mascotte blob ou 9 escargots. Choix gardé pour toutes les fenêtres ; « Revenir à l'apparence d'origine ».

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
8. 3 LFO assignables + page MOD ✅ (v1.4.0)
9. LFO dessinés à la souris + grand éditeur ✅ (v1.5.0)
10. Mascotte blob gluant (peau ombrée en relief, bave avec l'Accent) ✅ (v1.6.0)

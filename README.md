# OctoCid

Synthé acid VST3 par **OctoBio** — émulation fidèle d'une bassline acid classique, séquenceur intégré, tiroir de mods modernes et une mascotte escargot.

> Statut : étape 2 (moteur sonore). Jouable au clavier ou au piano roll, avec des potards provisoires.

## Jeu au clavier

- Notes qui se chevauchent = **slide** (glissement sans redéclencher les enveloppes)
- Vélocité ≥ seuil (100 par défaut, réglable) = **accent**
- Qualité : Normal (suréchantillonnage 2x) ou High (4x)

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
3. Drive, LFO, Slime Mods
4. Séquenceur + sync DAW + export MIDI
5. Interface
6. Escargot animé
7. Presets, finitions, pluginval

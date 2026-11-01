# Code et données tiers

## Open303 — Robin Schmidt

Le calibrage du moteur sonore d'OctoCid (plages des potards, loi de modulation du cutoff,
enveloppes, accent, courbe de résonance, filtres d'entrée/sortie) reprend les valeurs
mesurées par le projet Open303 : https://github.com/RobinSchmidt/Open303

Le filtre d'OctoCid a la même fonction de transfert que le modèle « TB_303 »
(mystran & kunn) d'Open303, mais il est discrétisé différemment (ZDF/TPT).

```
The MIT License

Copyright (c) 2009 Robin Schmidt (www.rs-met.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

## Reverb — référence Mutable Instruments (Émilie Gillet)

La reverb plate d'OctoCid suit la topologie de Griesinger décrite par Jon Dattorro
(« Effect Design Part 1 », JAES 1997), telle qu'utilisée dans les modules Mutable Instruments
(https://github.com/pichenettes/eurorack, code DSP sous licence MIT, Copyright (c) 2014 Emilie Gillet).
Le code d'OctoCid est une réécriture ; aucun fichier n'a été copié.

## Compresseur Punch

Conception d'après D. Giannoulis, M. Massberg, J. D. Reiss, « Digital Dynamic Range Compressor
Design — A Tutorial and Analysis », JAES 2012 (algorithme publié, code original).

## Drive « Distortion »

Modèle de circuit écrit à partir des valeurs de composants publiées d'une pédale ProCo RAT
(analyse ElectroSmash). Aucun code tiers.

## JUCE

OctoCid est construit avec JUCE 8 (https://juce.com), sous les termes de la licence JUCE / AGPLv3.


## Police du logo

Le logo est une image rendue avec la police **Bungee** (David Jonathan Ross), licence SIL Open Font License 1.1.
Aucun fichier de police n'est distribué avec le plugin.

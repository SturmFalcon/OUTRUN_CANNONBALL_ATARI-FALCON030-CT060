# Conversion couleur RGB565 et écriture écran

Routine : `_atari_truecolor_diff` (src/main/atari/truecolor_asm.S), une rangée de 320 pixels par appel,
via `convert_rows_diff` (video_falcon.cpp). Part du temps [H] ≈ 22 % (+ 2,5 % de comparaison de lignes).

## Fonctionnement
- Le moteur produit une image d'index (13 bits, palette arcade de 8192 entrées, moitié ombrée).
- Pour chaque tampon d'écran (triple tampon), une copie des index qu'il affiche est gardée en Fast-RAM
  (« shadow »). Une paire de pixels identique à la copie n'est ni convertie ni écrite.
- Une paire changée : deux lectures de la palette RGB565 (Fast-RAM), une écriture longue en ST-RAM.
- Une ligne identique à l'image précédente et déjà à jour dans ce tampon est sautée entièrement
  (`_atari_rowcmp` + numéro de version de ligne).
- Volume [H] : ≈ 72 Ko écrits par image sur 143 Ko ; 95 % des mots écrits changent vraiment.

## Pistes
| Piste | Statut | Résultat |
|---|---|---|
| Deux paires par tour, comparées avant toute écriture (040/060) | [H-VALIDÉ] 7f86484 | 39,7 → 39,0 ms (−2 %), écran identique (ROWCHECK 0/2500) |
| Blocs de 16 octets changés convertis en Fast-RAM puis MOVE16 (`-DMOVE16_DIFF`) | [EXPÉRIMENTAL] 1e999aa | 35,6 ms contre 36,5 sous Hatari, écran exact (ROWCHECK 0/4140) ; écrit plus d'octets : à départager sur CT60 |
| MOVE16 de lignes entières (`-DMOVE16`, existant) | [EXPÉRIMENTAL] | écrit 143 Ko par image ; Fallout [RCT60] : MOVE16 vers ST sans gain |
| Table par paire d'index | écartée | 2 × 13 bits = 64 M entrées |
| Conversion précalculée par ligne | écartée | les index changent à chaque image (défilement) |

Les 65 536 couleurs et la palette arcade sont inchangées.

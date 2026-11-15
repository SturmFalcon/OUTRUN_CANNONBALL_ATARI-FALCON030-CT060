# Cadence adaptative (images par seconde selon la machine réelle)

Code : `main_loop_vsync()` dans `src/main/main_atari.cpp`, `Outrun::tick_part()` dans
`src/main/engine/outrun.cpp`, VBL dans `src/main/atari/vsync.cpp`. Commits bd28b75, fc2dad7, 812fd0a.

## Principe
L'arcade fait tourner sa logique à 30 Hz et prend deux interruptions verticales (vint) par pas, à 60 Hz.
Le pas est coupé en deux moitiés : `tick_part(f,0)` = logique + CPU route + 1re vint ; `tick_part(f,1)` =
2e vint + crédits. Les deux à la suite font exactement `tick(f)` en mode 30 fps.

Une moitié est due par VBL : **la vitesse du jeu ne dépend jamais du nombre d'images** (30 pas/s à 60 Hz).
Une image est dessinée toutes les n moitiés ; elle est présentée à un VBL fixé (`vsync_queue`) et reste
donc exactement n VBL à l'écran.

| n | images/s (VGA 60 Hz) | régularité |
|---|---|---|
| 1 | 60 | chaque vint montrée, comme l'arcade |
| 2 | 30 | une image par pas |
| 3 | 20 | régulière (3 VBL) |
| 4 | 15 | régulière |
| 5 | 12 | régulière |
| 6 | 10 | régulière (plancher) |

40 et 50 i/s ne divisent pas 60 Hz : images de 1 puis 2 VBL (saccade). 25 i/s n'est régulier qu'en
RVB 50 Hz. Ils ne sont pas proposés. (Règle permanente : VGA = 60 Hz seulement.)

## Choix automatique (AUTO, défaut)
Mesures à chaque image avec l'horloge fine Timer C (26 µs) : coût du rendu R et coût moyen d'une moitié Lh
(moyenne glissante et crête décroissante).
- **Descente** d'un niveau : 6 images de suite où R + n·Lh > n VBL, ou 2 images en retard sur les 16 dernières.
- **Montée** d'un niveau : seulement après ~3 s (180 VBL) où la crête récente tiendrait dans n−1 VBL avec 20 % de marge.
- Groupe entier en retard : pas d'image cette fois, le jeu garde son rythme (compté `missed`).
- Niveau de départ : `hw_start_level()` d'après le profil détecté ([HYPOTHÈSE], voir PERFORMANCE_PROFILES.md).

Aucune réduction de qualité (véhicules, décor, ombres) n'est faite automatiquement.

## Réglages
`outrun.ini` : `vsync = 2` (défaut : actif si l'écran mesure 55 à 65 Hz et CPU 68040 ou 68060), `vrate = 0` (AUTO) ou
60/30/20/15, `vsync_lat = 1` (VBL entre la fin d'un groupe et son image). Menu OPTIONS : ligne
PICTURES PER SEC = AUTO nn / 60 / 30 / 20 / 15 (enregistrée par SAVE SETTINGS). Menu principal : `CPU 68060 nn MHZ`.
Sur 030 la boucle d'origine (cadence/fps) est inchangée.

## Non-régression du jeu
Option de test `-DSTATE_CRC` : somme de contrôle de l'état (road_pos, car_increment, car_x, score, chrono,
game_state) après chaque pas, écrite aux pas 300/600/900/1200. [H] 15/11 : **identique** pour v0.39,
l'ancienne boucle, AUTO, 60, 20 et 15 fixes (`mesures/2026-11-15_adaptatif_H060/statecrc.txt`).

## Écrans 75 / 90 Hz : étude (rien d'annoncé, rien de validé)
- La logique du jeu est à 30 Hz (vint 60 Hz). À 75 Hz, 30 pas/s = 2,5 VBL par pas : aucune cadence régulière
  ne suit la logique (75/37,5/25 i/s montrent des pas inégaux). **Pire que 60 Hz.**
- À 90 Hz, 30 pas/s = 3 VBL : 30 i/s régulières (comme à 60 Hz), 45 i/s = 1,5 VBL par moitié de pas : saccade.
  **Aucun gain** sur 60 Hz pour ce jeu.
- Côté Videl : en VGA l'horloge pixel est 25,175 MHz (ou 32 MHz) ; à fréquence ligne VGA (31,5 kHz), 75 Hz ne laisse
  que ~419 lignes totales, trop peu pour 224 lignes doublées (448). Il faudrait une fréquence ligne hors norme VGA,
  donc un moniteur multisync et des registres Videl non documentés dans falcon-dev-docs. Hatari ne valide pas ces modes.
- Conclusion : 75/90 Hz non retenus. Ils ne seraient étudiés que sur un vrai Falcon avec un moniteur qui les accepte.

# Comparaison v0.39-beta / version CT60 [ANALYSE]

Exécutables : CB060 v0.39 (md5 4168938a…, tag v0.39-beta) et CB060 CT60 (md5 78d03e87…, commit c66b827).
Deux méthodes indépendantes : Ghidra MCP 12.1.4 (PC du banc) et objdump (`tools/ct60/funcmap.py`, cloud).
Une différence de code n'est **pas** un gain : les gains sont mesurés à part (CT60_60MHZ_OPTIMIZATION.md).

## Vue globale
| Méthode | identiques | modifiées | nouvelles | supprimées | remarque |
|---|---|---|---|---|---|
| Ghidra (fonctions reconnues et nommées, hachage d'opcodes) | 1202 | 76 | 25 | 11 | correspondance floue : 1352/1562, dont 1134 à 1,0 |
| objdump (tous les symboles, séquence d'instructions) | 2561 (+325 opérandes seules) | 46 | 51 | 16 | « opérandes seules » = adresses déplacées |

## Routines critiques
| Routine | Rôle | Ghidra : similarité, taille v0.39 → CT60 | Pourquoi elle change |
|---|---|---|---|
| `_atari_sprite_rows` | sprites zoomés | 0,49 ; 2248 → 2400 o | boucle par pixel 060, rangées réduites aux pixels opaques |
| `_atari_truecolor_diff` | conversion RGB565 + écriture écran | 0,53 ; 74 → 146 o | deux paires par tour (040/060) |
| `hwtiles::render_tile_layer` | tuiles | 0,69 | recherche de tuile en ligne |
| `_atari_tile8d`, `_atari_tile8d_mask` | tracé 8×8 | 1,0 (identiques) | — |
| `_atari_road_copy1/2`, `_atari_fill16` | route | 0,73 / 0,53 / 0,40 | écritures longues, deux index par mot |
| `HWRoad::render_foreground_lores` | route | 0,29 | appels de la pompe ST ; Ghidra coupe la fonction v0.39 autrement (646 o) qu'objdump (1952 o) |
| `Render::draw_frame` | assemblage de l'image | 0,76 | pompe, captures de test |
| `Render::flip_screen_base` | échange d'écran | 0,24 ; 50 → 140 o | échange au VBL (registres Videl seuls, $45E = 0) |
| `_atari_pcm_channel`, `Audio::tick_now` | son | 1,0 (identiques) | — |
| `_atari_copy16` | MOVE16 | 1,0 | — |
| `vsync_vbl`, `_atari_st_pump`, `_atari_truecolor_stage` | VBL, pompe | nouvelles | cadence adaptative, pompe ST |

Aucune instruction non décodée dans ces routines (décodage 68040). Sorties brutes :
`/mnt/project-files/cannonball-ct60/ghidra/resultats/` (V039_*.json, CT60_*.json, comparaison_routines.json).

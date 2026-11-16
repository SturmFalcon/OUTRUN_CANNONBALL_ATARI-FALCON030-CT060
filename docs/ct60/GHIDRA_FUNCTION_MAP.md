# Carte des fonctions critiques (CB060, version CT60)

Exécutable CB060 CT60 md5 78d03e87… (commit c66b827). Adresses = décalage dans le segment texte (Ghidra : +$10000).
Taille et instructions : objdump + symboles (`tools/ct60/funcmap.py map`), recoupés par Ghidra MCP [ANALYSE].
« Profil » : part du temps CPU [H] (Hatari 060/32 MHz, vrate 60 pour saturer, build B060PR3) ; sous Hatari,
l'écriture ST-RAM est trop rapide (×3,7), donc la conversion y est sous-estimée par rapport à la vraie CT60.
Les rôles viennent des sources (aucun nom deviné).

## Par sous-système (profil [H])
| Sous-système | Part | Fonctions principales |
|---|---|---|
| Sprites zoomés | ≈ 29 % | `_atari_sprite_rows` 25,9 %, `hwsprites::render` 1,7 %, `OSprites::do_sprite` 0,9 % |
| Conversion RGB565 + écriture écran | ≈ 25 % | `_atari_truecolor_diff` 21,8 %, `_atari_rowcmp` 2,5 %, `memcmp` 0,6 % |
| Route | ≈ 17 % | `_atari_road_copy2` 4,8 %, `_atari_road_copy1` 3,8 %, `_atari_fill16` 2,8 %, `render_foreground_lores` 2,3 %, `_atari_road_lut` 1,4 %, `_atari_road_offset` 0,9 % |
| Tuiles et texte | ≈ 15 % | `render_tile_layer` 6,1 %, `_atari_tile8d` 5,5 %, `render_text_layer` 1,6 %, `_atari_tile8_clip` 1,4 %, `_atari_tile8d_mask` 0,8 % |
| Son (68060) | ≈ 7 % | `_atari_pcm_channel` 3,7 %, `Audio::tick_now` 1,6 %, `FmDsp::step` 0,7 %, `YM2151::write_reg` 0,3 % |
| Logique du jeu (route, sprites, niveau) | ≈ 3 % | `ORoad::do_road_data`, `setup_x_data`, `OSprites::*` |
| TOS | 0,3 % | |

## Fonctions
| Rôle | Fonction | Adresse | Octets | Instr. | div / mul | Remarques 68060 |
|---|---|---|---|---|---|---|
| Boucle principale | `_main` | $3133E | 4160 | 1056 | 5 / 33 | contient la boucle adaptative (inline) |
| Pas de jeu | `tick` (statique) | $30E78 | 1014 | 264 | 0 / 0 | |
| Demi-pas moteur | `Outrun::tick_part` | $26CA4 | 350 | 97 | 0 / 0 | logique 30 Hz en deux moitiés |
| VBL | `vsync_vbl` | $9432 | 76 | 22 | 0 / 0 | échange d'écran : registres Videl seuls |
| File d'échange | `vsync_queue` | $9526 | 56 | 14 | | |
| Route, données | `ORoad::do_road_data` | $1DA3E | 322 | 103 | | logique arcade |
| Route, segments | `ORoad::setup_x_data` | $1C906 | 538 | 187 | 5 / 6 | 5 divisions par appel |
| Route, tracé | `HWRoad::render_foreground_lores` | $2E396 | 1978 | 573 | 0 / 2 | appelle copy1/copy2/fill16 |
| Route, fond | `HWRoad::render_background_lores` | $2E28A | 268 | 87 | 0 / 3 | |
| Route, copies | `_atari_road_copy1` / `_atari_road_copy2` | $395CE / $39626 | 88 / 112 | 31 / 42 | | écritures longues alignées |
| Route, remplissage | `_atari_fill16` | $3957C | 82 | 33 | | déroulé ×4 |
| Sprites, préparation | `hwsprites::render` | $2F38E | 1144 | 329 | 1 / 3 | une entrée par sprite |
| Sprites, tables | `hwsprites::init` | $2EE2C | 1158 | 377 | | tables lead/last (pixels opaques) |
| Sprites, tracé zoomé | `_atari_sprite_rows` | $398C0 | 2400 | 835 | 3 / 2 | span_row : 3 `divu.l` par rangée ; span_fr : boucle par pixel |
| Sprites, logique | `OSprites::do_sprite` | $1F2BE | 908 | 284 | 1 / 3 | |
| Conversion RGB565 | `_atari_truecolor_diff` | $3A782 | 146 | 54 | | deux paires par tour, écrit en ST-RAM |
| Conversion, lignes | `convert_rows_diff` | $874E | 316 | 105 | 0 / 1 | choisit diff / pompe / MOVE16 |
| Cache de lignes | `_atari_rowcmp` | $3A74C | 30 | 11 | | compare à l'image précédente |
| Image | `Render::draw_frame` | $8E36 | 1096 | 307 | 0 / 12 | |
| Échange d'écran | `Render::flip_screen_base` | $8D90 | 166 | 48 | | triple tampon |
| Pompe ST | `_atari_st_pump` / `_atari_st_flush` / `_atari_truecolor_stage` | $3A220 / $3A24E / $3A270 | 46 / 34 / 72 | | | option `stpump`, coupée |
| MOVE16 | `_atari_copy16` | $3A76A | 24 | 7 | | option de compilation |
| Tuiles, couche | `hwtiles::render_tile_layer` | $305BC | 1066 | 327 | 0 / 1 | recherche de tuile en ligne |
| Tuiles 8×8 | `_atari_tile8d` / `_mask` / `_clip` | $3A3EA / $3A4A6 / $3A56E | 188 / 200 / 146 | | | tuiles prédécodées |
| Texte | `hwtiles::render_text_layer` | $309E6 | 972 | 283 | | |
| Son PCM | `_atari_pcm_channel` | $394D8 | 162 | 55 | | mixage 68060 |
| Son, pas | `Audio::tick_now` | $6EE | 998 | 291 | | |
| FM sur DSP | `FmDsp::step` / `FmDsp::flush` | $25EC / $22DC | 240 / 610 | | | le 68060 envoie les registres |
| .mod sur DSP | `DspReplay::frame` | $12FE | 690 | 207 | 2 / 2 | |
| Détection | `hw_detect` / `_atari_hw_spin` | $3642 / $393B8 | 636 / 54 | | | cookies, boucle chronométrée |

Liste complète (toutes les fonctions, classes d'instructions) : `tools/ct60/funcmap.py map CB060.TOS`.
Fiches Ghidra (désassemblage, C, xrefs) : `/mnt/project-files/cannonball-ct60/ghidra/resultats/`.

## Incertitudes
- Les tailles d'objdump s'arrêtent au dernier symbole ; Ghidra découpe parfois autrement (render_foreground_lores v0.39 : 646 o pour Ghidra, 1952 pour objdump).
- Le profil est celui d'une course, 1re étape ; les menus et les autres étapes ne sont pas mesurés.
- Sous Hatari, l'ordre des coûts n'est pas celui de la vraie CT60 : écriture ST trop rapide, calcul ≈ 2,2 fois trop lent.

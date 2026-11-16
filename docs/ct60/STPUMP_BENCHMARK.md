# Pompe ST-RAM (`stpump`) : mesures

Principe et code : ST_WRITE_OVERLAP.md. Option outrun.ini `stpump` (0 = coupée, défaut ; 1 à 16 = mots
longs écrits par appel), 040/060 seulement. Statut : **[EXPÉRIMENTAL]**.

## Organisation (code machine relu, Ghidra [ANALYSE])
- `_atari_truecolor_stage` range chaque écriture changée en paire (adresse ST, valeur) dans une liste en
  Fast-RAM (8 octets par mot long écrit, ≈ 145 Ko pour une image).
- `_atari_st_pump` (12 instructions, appelée par les rangées de sprites, les lignes de route et les tuiles)
  écrit K paires puis rend la main ; `_atari_st_flush` vide le reste avant l'échange d'écran.
- Les écritures restent dans l'ordre de l'image, une par mot long : la file d'écriture du 68060 les absorbe
  si le calcul qui suit ne touche pas le bus [HYPOTHÈSE à mesurer].

## Mesures
| Où | Résultat | Valeur |
|---|---|---|
| Jeu, Hatari [H] | +5 ms par image (gestion de la liste ; Hatari n'émule pas le chevauchement) | sans valeur pour la CT60 |
| PUMPCHECK [H] | écran = conversion | 0 erreur sur 450 images |
| STOVL, Hatari [H] | D (pompe K=1..8) plus lent que A+B | sans valeur pour la CT60 |
| STOVL, vraie CT60 | **non mesuré** | campagne finale (groupe 2) |
| Jeu, vraie CT60, K = 2/4/8 | **non mesuré** | campagne finale (groupe 3) |

Décision : reste coupée tant que la vraie CT60 n'a pas montré D nettement sous A + B.

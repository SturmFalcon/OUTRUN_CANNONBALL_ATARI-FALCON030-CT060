# Sprites zoomés : optimisation

Routine : `_atari_sprite_rows` (src/main/atari/sprite_asm030.S), appelée une fois par sprite par
`hwsprites::render`. Part du temps [H] : ≈ 26 % (≈ 29 % avec la préparation C++).

## Fonctionnement (code machine relu, recoupé par Ghidra [ANALYSE])
- Les champs du sprite (zoom, couleur, pas, table de transparence…) sont chargés une fois ; la boucle
  `rows_loop` traite chaque rangée visible (clip vertical, rangées non tracées sautées).
- Zoom différent de 1:1 : `span_row` calcule, pour la rangée, le nombre K de pixels écran
  (K = ⌈N·$200/hzoom⌉), le borne au clip, puis trace pixel écran par pixel écran (`span_fr` vers la droite,
  `span_fwd`/`span_bloop` sinon). Chaque pixel lit le quartet source, passe par la table de couleur
  (0 = transparent, bit 15 = ombre) et écrit un mot dans le tampon d'index (Fast-RAM).
- La rangée suivante avance par l'accumulateur de zoom vertical ; la pompe ST peut écrire quelques mots
  longs entre deux rangées.

## Pistes mesurées [H] (15 i/s fixe, images identiques à v0.39)
| Piste | Statut | Rendu | Gain |
|---|---|---|---|
| Virgule fixe permutée + `(a2)+` dans la boucle par pixel | [H-VALIDÉ] 96851c0 | 45 → 44,2 ms | −3 % |
| Rangées réduites à leurs pixels opaques (tables lead/last précalculées au démarrage) | [H-VALIDÉ] 44189bd | 44,2 → 40,1 ms | −10 % |
| Division 16 bits dans la préparation de rangée (`-DSPR_DIVW`) | [EXPÉRIMENTAL] 582233f | 36,5 → 38,1 ms | +4 % sous Hatari |
| Sortie avant toute division pour les rangées sans pixel opaque (`-DSPR_EARLYNONE`) | [EXPÉRIMENTAL] 4a7d700 | 36,5 → 36,6 ms | neutre |

Les deux variantes expérimentales donnent les mêmes images ; elles seront départagées sur la vraie CT60
(sur le papier, `divu.w` coûte environ la moitié de `divu.l` sur 68060 [HYPOTHÈSE] ; Hatari dit le contraire
ou bien c'est un effet d'alignement du code).

## Pistes étudiées, non retenues
- Réciproque de hzoom par sprite pour supprimer les divisions : demanderait une multiplication 32×32→64,
  que le 68060 n'exécute pas en matériel (exception), ou une correction ±1 ; gain estimé < 2 %.
- Déroulage de la boucle par pixel : elle contient déjà un seul branchement par pixel (transparence),
  la sortie du dbra étant prédite ; le gain dépend du prédicteur réel, non mesurable sous Hatari.

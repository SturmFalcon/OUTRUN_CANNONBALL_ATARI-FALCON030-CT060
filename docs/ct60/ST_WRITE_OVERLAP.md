# Chevauchement des écritures ST-RAM (« pompe »)

## Le problème
L'écran du Falcon est en ST-RAM, sur le bus 16 bits à 16 MHz. Sur la vraie CT60, une écriture Fast-RAM → ST
plafonne vers **5,6 Mo/s [RCT60, ISOBENCH]**, quelle que soit la fréquence du 68060. Cannonball écrit
≈ 72 Ko par image (seules les paires de pixels changées), soit **≈ 13 ms par image [HYPOTHÈSE]** : presque la
moitié du budget de 30 i/s.

Le 68060 a une file d'écriture : une écriture ST le bloque seulement quand la file est pleine. Le travail Fallout
(MKBENCH, mesuré sur la vraie CT60) donne **au moins 64 instructions libres par mot long écrit en ST**. Si on
glisse les écritures ST au milieu du calcul de l'image suivante, une partie des 13 ms peut disparaître.

## Ce qui est fait (commit 3545057, option `stpump` dans outrun.ini, **0 = coupée par défaut**)
- La conversion couleur ne touche plus l'écran : elle range les écritures dans une liste (adresse, valeur) en
  Fast-RAM.
- Pendant le rendu de l'image suivante, les sprites (à chaque rangée), la route (à chaque ligne) et les tuiles
  (à chaque tuile) appellent `st_pump()`, qui écrit K mots longs de la liste (K = `stpump`, 1 à 16).
- Avant de montrer l'image, le reste de la liste est vidé, puis l'échange d'écran est demandé au VBL.
- La pompe ne s'active que si le processeur est un 040 ou un 060 (`_CPU` ≥ 40) ; jamais sur 030.
- Vérification [H] : PUMPCHECK compare l'écran à la conversion, **0 erreur** sur 450 images ; images du moteur
  identiques à v0.39.

## Pourquoi elle est coupée
Hatari n'émule pas la file d'écriture ni la lenteur du bus ST : sous Hatari la pompe **coûte** ≈ 5 ms par image
(la gestion de la liste) et ne peut rien gagner. Son intérêt ne peut se mesurer que sur la vraie CT60.

## Outil de mesure pour la vraie CT60 : STOVL.TOS (tools/ct60, commit 0193c4b)
Quatre essais chronométrés, journal `JOURNAUX\STOVL.LOG`, refuse de tourner si `_CPU` < 40 :
- A : écritures ST seules ; B : calcul seul ;
- C : les deux à la suite ; D : les deux mêlés, K = 1, 2, 4, 8 mots par appel.
Si D est nettement sous A + B, la pompe vaut la peine d'être réglée et mise par défaut ; sinon elle reste coupée.
Résultats [H] (sans valeur pour le chevauchement) : A 4,7 ms, B 104 ms.

À faire passer avec la campagne CT60 finale (après accord de STurm), avec les tâches CB-HW-RCT60-01 et
CB-CAL-RCT60-01.

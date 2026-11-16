# Optimisation CT60 (68060 à 60 MHz) : journal

Branche `ct60-60mhz-optimization` du fork de STurm. Référence immuable : **v0.39-beta** (07b04bd, CB060.TOS md5
4168938aba86a5d6cbce23f1dda30a53, reconstruit bit à bit).

Tags : **[H]** mesuré sous Hatari (060 à 32 MHz, cpu-exact, VGA, EmuTOS 1.3) ; **[RCT60]** mesuré sur la vraie
CT60 (aucune mesure à ce jour) ; **[HYPOTHÈSE]** estimation.

## Classement
- [H-VALIDÉ] : #1, #2, #4 à #8 (actifs par défaut).
- [EXPÉRIMENTAL] : #3 pompe (`stpump`), #9, #10, #11, `-DMOVE16` (coupés ; départagés sur la vraie CT60).
- [RCT60-VALIDÉ] : aucun à ce jour.
- Règle du 16/11 (STurm) : une variante plus lente sous Hatari n'est plus supprimée ; elle reste en option
  jusqu'à la campagne matérielle.

## Règles suivies
- Affichage jamais au-dessus de 60 Hz ; cadences 60/30/20/15 (diviseurs du VBL) ; RVB 50 Hz reste compatible.
- Logique du jeu indépendante du rendu : sommes d'état (`-DSTATE_CRC`) identiques à v0.39 à toutes les cadences.
- Jamais de baisse de qualité automatique : 320×224, 65 536 couleurs, tous les véhicules, décors et ombres.
- La fréquence de la CT60 n'est jamais modifiée par le programme ; elle est seulement mesurée.
- Une piste = un commit, avec mesure avant/après et comparaison d'images.

## Méthode de mesure [H]
- Build de mesure : `-DAUTOPLAY -DBENCH_N=400` (pièce, départ, accélérateur, 400 images après 100 de mise en route).
  **Jamais** de `-DSTWRITE_COUNT` dans un build chronométré (rendu gonflé d'environ 40 %).
- Cadence 15 fixe pour comparer le coût du rendu (`render_ms_x10`), puis AUTO pour la cadence obtenue.
- Non-régression de l'image : `-DSHOTHALVES -DDUMP_INDEX` écrit l'image du moteur aux pas 400, 450 … 2400 ;
  chaque version doit donner des fichiers **identiques octet pour octet** à ceux de v0.39 (`cmp`).
- `-DROWCHECK` : l'écran ST correspond à la conversion, ligne par ligne. `-DPUMPCHECK` : idem avec la pompe.

## Journal

| # | Commit | Piste | Rendu 15 fixe [H] | Gain | Images |
|---|---|---|---|---|---|
| 0 | 653ed94 | référence de la boucle adaptative | 44,5 à 45,6 ms | — | réf. |
| 1 | 96851c0 | sprites : boucle par pixel 060 (virgule fixe permutée, `(a2)+`) | 44,2 ms | −3 % | identiques |
| 2 | 6e12bf0 | correctif VBL : `$45E` laissé à 0, registres Videl seuls | sans effet sur le coût | — | **corrige** 220/500 images fausses (ROWCHECK) |
| 3 | 3545057 | pompe d'écriture ST (`stpump`, coupée par défaut) | +5 ms sous Hatari | voir ST_WRITE_OVERLAP.md | identiques, PUMPCHECK 0 erreur |
| 4 | 44189bd | sprites : chaque rangée réduite à ses pixels opaques | **40,1 ms** | **−10 %** | identiques |
| 5 | c402bd2 | route : écritures longues alignées, remplissage déroulé ×4 (040/060) | 39,7 ms | −1 % | identiques |
| 6 | 7f86484 | conversion couleur : deux paires par tour, comparées avant toute écriture (040/060) | 39,0 ms | −2 % | écran identique (ROWCHECK 0/2500) |
| 7 | c34a268 | tuiles : recherche de la tuile en ligne, sans appel par colonne | 37,0 ms | −5 % | identiques |
| 8 | 62db2f7 | route : les deux index de la table tirés d'un mot de chaque source | **36,5 ms** | −1,4 % | identiques |
| 9 | 582233f | sprites : division 16 bits (`-DSPR_DIVW`) | 38,1 ms | +4 % | identiques ; [EXPÉRIMENTAL], coupé |
| 10 | 1e999aa | conversion : blocs de 16 octets changés en MOVE16 (`-DMOVE16_DIFF`) | 35,6 ms | −2,5 % sous Hatari | écran exact ; [EXPÉRIMENTAL], coupé (Hatari n'émule pas le bus) |
| 11 | 4a7d700 | sprites : rangées vides quittées avant les divisions (`-DSPR_EARLYNONE`) | 36,6 ms | neutre | identiques ; [EXPÉRIMENTAL], coupé |

Total depuis la boucle adaptative : **≈ 45 → 36,5 ms par image [H] (−19 %)**, sans perte de qualité.

Cadence AUTO pleine qualité [H] :
- avant (6f00ae3) : 14,4 i/s, presque tout à 15 i/s ;
- après la route (c402bd2) : 16,1 i/s, 183 images à 20 i/s, 169 à 15 ;
- après les tuiles (c34a268) : 18,2 i/s, 323 images à 20 i/s ;
- après la route (62db2f7) : **18,3 i/s**, 331 images à 20 i/s, 45 à 15, 0 image sautée, 0 en retard.

## Profil restant [H] (après #5, avant #6 et #7)
- conversion couleur + écriture ST (`tv_loop`) ≈ 21 %
- route ≈ 11 %, tuiles ≈ 10 %
- préparation de chaque rangée de sprite (3 divisions `divu.l`) ≈ 5 %
- mixage du son PCM ≈ 3 %

## Ce que ça donne sur CT60 [HYPOTHÈSE]
Voir HATARI_CT60_COMPARISON.md : ≈ 28,3 ms par image à 60 MHz (plage 24 à 35), ≈ 25 ms à 75, ≈ 23 ms à 90.
Le but A (30 i/s stables à 60 MHz, budget 33,3 ms) est plausible mais pas acquis tant que la vraie CT60 n'a pas
mesuré. 60 i/s reste hors de portée (voir la même page).

## Pistes suivantes
1. Préparation des rangées de sprites : moins de divisions.
2. Pompe : liste par plages plutôt que par paires (adresse, valeur), pour le test sur vraie CT60.

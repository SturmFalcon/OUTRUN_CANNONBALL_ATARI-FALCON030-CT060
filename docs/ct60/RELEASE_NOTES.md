# Notes de version, branche CT60 (depuis v0.39-beta)

Version de travail, non publiée. Mesures [H] = Hatari 060/32 MHz ; rien de mesuré sur vraie CT60.

## Cadence
- Images calées sur le VBL : une image reste exactement 1, 2, 3 ou 4 VBL à l'écran (60, 30, 20, 15 i/s ;
  12 et 10 en secours). Jamais d'affichage au-dessus de 60 Hz.
- **AUTO** (par défaut) : mesure chaque image, descend vite si le temps manque, remonte après ≈ 3 s avec de la
  marge. Ne baisse jamais la qualité.
- Choix manuel AUTO / 60 / 30 / 20 / 15 dans le menu OPTIONS et `vrate` dans outrun.ini.
- La vitesse du jeu ne dépend plus du nombre d'images : la logique avance d'un demi-pas par VBL (état du jeu
  identique à v0.39 à toutes les cadences).

## Détection
- Menu : « CPU 680x0 nn MHZ » (fréquence mesurée, jamais modifiée) ; cookies _CPU, _FPU, _MCH, CT60, MiNT,
  mémoire libre.

## Vitesse [H], images identiques à v0.39
- Sprites zoomés : boucle par pixel 060 (−3 %) puis rangées réduites à leurs pixels opaques (−10 %).
- Route : écritures longues alignées (−1 %).
- Rendu d'une image : ≈ 45 → 39,7 ms ; AUTO pleine qualité 14,4 → 16,1 i/s (la moitié du temps à 20 i/s).

## Corrections
- Pixels faux avec la boucle VBL (adresse écran TOS $45E).

## Options nouvelles (outrun.ini)
- `vrate` : 0 = AUTO (défaut), ou 60, 30, 20, 15.
- `stpump` : 0 (défaut) ou 1 à 16 ; écritures écran mêlées au calcul de l'image suivante, 040/060 seulement,
  à évaluer sur vraie CT60.

## Outils
- STOVL.TOS : mesure du chevauchement écritures ST / calcul sur 060.

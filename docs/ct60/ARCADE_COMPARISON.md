# Comparaison avec la borne d'arcade

| | Borne OutRun (Sega, 1986) | Cannonball Falcon CT60 (cette branche) |
|---|---|---|
| Image | 320×224 | 320×224 (identique, rien n'est réduit) |
| Couleurs | palette arcade | 65 536 couleurs (RGB565), palette convertie |
| Logique du jeu | 30 pas/s | 30 pas/s, mêmes pas (état identique à v0.39, elle-même fidèle au moteur Cannonball) |
| Images affichées | une image par pas logique (30/s), défilement du décor à 60/s | AUTO 60/30/20/15 ; [H] 15-20/s ; CT60 30/s visé [HYPOTHÈSE] |
| Rafraîchissement | ≈ 60 Hz | VGA ≈ 60 Hz (59,94 mesuré sur CT60) ; jamais au-dessus |
| Vitesse du jeu | référence | logique calée sur le VBL du Falcon : écart avec la borne = écart entre les deux fréquences d'écran, non mesuré précisément ici |
| Son | YM2151 + PCM Sega | YM2151 sur le DSP (même son, échantillon par échantillon), PCM mixé par le 68060 |

Ce qui n'est pas reproduit : 60 images/s pleine qualité (hors de portée selon l'estimation, voir
HATARI_CT60_COMPARISON.md). Ce qui ne peut pas se comparer ici : la latence des commandes et le ressenti
(**non mesuré**).

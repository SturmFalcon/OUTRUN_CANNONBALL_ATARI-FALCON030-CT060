# Historique des mesures

Même scène pour toutes les lignes : autoplay, 1re étape, 400 images après 100 de mise en route, pleine qualité.
Tout est **[H]** (Hatari 060 à 32 MHz, cpu-exact, VGA). Colonne [RCT60] : vide tant que la vraie CT60 n'a pas tourné.
Données brutes et MD5 : `/mnt/project-files/cannonball-ct60/mesures/`.

| Date | Commit | Changement | Rendu / image [H] | AUTO [H] | [RCT60] |
|---|---|---|---|---|---|
| 15/11 | 07b04bd (v0.39) | référence, boucle 200 Hz | non séparé (≈ 67 ms d'intervalle) | 14,8 i/s, 342/401 régulières | non mesuré |
| 15/11 | 653ed94 | boucle VBL adaptative | 45-46 ms | 11,8 i/s (trop prudent) | non mesuré |
| 15/11 | 6f00ae3 | règle de montée d'AUTO | 45-46 ms | 14,4 i/s, 0 retard | non mesuré |
| 15/11 | 96851c0 | sprites, boucle par pixel | 44,2 ms | — | non mesuré |
| 16/11 | 6e12bf0 | correctif $45E (exactitude) | — | — | non mesuré |
| 16/11 | 44189bd | sprites, rangées réduites | 40,1 ms | — | non mesuré |
| 16/11 | c402bd2 | route, écritures longues | 39,7 ms | 16,1 i/s, 0 retard | non mesuré |
| 16/11 | 7f86484 | conversion couleur, deux paires par tour | 39,0 ms | 16,7 i/s, 0 retard | non mesuré |
| 16/11 | c34a268 | tuiles, recherche en ligne | 37,0 ms | 18,2 i/s, 0 retard | non mesuré |
| 16/11 | 62db2f7 | route, deux index par mot | **36,5 ms** | **18,3 i/s**, 0 retard | non mesuré |
| 16/11 | (abandonné) | sprites, division 16 bits | 38,1 ms | 16,9 i/s | non mesuré |

Détail par piste : CT60_60MHZ_OPTIMIZATION.md. Cadences détaillées : FPS_BENCHMARKS.md.

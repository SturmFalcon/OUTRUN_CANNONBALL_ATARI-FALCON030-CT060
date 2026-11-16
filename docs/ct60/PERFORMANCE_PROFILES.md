# Profils de performance

| Profil (`hwdetect`) | Condition | Boucle | Niveau de départ AUTO | Statut |
|---|---|---|---|---|
| 030 | `_CPU` < 40 | boucle d'origine v0.39 (cadence/fps) | — | inchangé |
| 040 | 40 ≤ `_CPU` < 60 | boucle d'origine | — | non testé |
| 060-LENT | 060, < 45 MHz mesurés | VBL adaptative | 20 i/s | [H] seulement |
| 060-60MHZ | 060, 45 à 75 MHz | VBL adaptative | 30 i/s | [HYPOTHÈSE] |
| 060-RAPIDE | 060, > 75 MHz | VBL adaptative | 30 i/s | [HYPOTHÈSE] |

Le niveau de départ ne décide que des ~3 premières secondes : AUTO se règle ensuite sur les temps mesurés.

## Table de la mission (hypothèse de départ, jamais présentée comme mesurée)
60 MHz → 30 ; 66 → 30-40 ; 75 → 40 ; 80 → 40-50 ; 90 → 50-60. Décision validée par STurm le 15/11 :
seules 60/30/20/15 i/s (VGA 60 Hz) ; 40 et 50 ne sont pas proposés.

## Estimation actuelle [HYPOTHÈSE] (rendu [H] 36,5 ms, calcul ÷ 2,2, écriture ST ≈ 13 ms)
| CT60 | par image | AUTO tiendrait |
|---|---|---|
| 60 MHz | ≈ 28 ms (24 à 35) | 30 i/s, sauf au pire de la plage (alors 20) |
| 75 MHz | ≈ 25 ms | 30 i/s |
| 90 MHz | ≈ 23 ms | 30 i/s, ≈ 10 ms de marge ; 60 i/s hors de portée |
Aucune mesure CT60 réelle de cette version à ce jour (campagne : CAMPAGNE_CT60_FINALE.md).

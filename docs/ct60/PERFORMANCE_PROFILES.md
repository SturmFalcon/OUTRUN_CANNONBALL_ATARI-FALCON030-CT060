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
60 MHz → 30 ; 66 → 30-40 ; 75 → 40 ; 80 → 40-50 ; 90 → 50-60.
Sur écran VGA 60 Hz, 40 et 50 ne sont pas réguliers : AUTO choisira 30 ou 60. Le passage à 60 i/s demande
qu'une image + une moitié de pas tiennent dans 16,7 ms (avec 20 % de marge pour monter).

## Estimation CT60 réelle [HYPOTHÈSE]
Voir HATARI_CT60_COMPARISON.md et FPS_BENCHMARKS.md. Aucune mesure CT60 réelle de cette version à ce jour.

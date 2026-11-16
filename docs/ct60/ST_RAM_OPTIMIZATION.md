# ST-RAM : ce qui coûte et ce qui est essayé

## Constat
- L'écran 320×224×16 bits est en ST-RAM (exigence du Videl) ; triple tampon, échange au VBL par les
  registres Videl seuls ($45E laissé à 0).
- Le bus ST du Falcon (16 bits, ≈ 16 MHz, partagé avec le Videl) accepte ≈ 5,6 Mo/s en écriture depuis la
  CT60 [RCT60, ISOBENCH], **quelle que soit la fréquence du 68060**.
- Cannonball écrit ≈ 72 Ko par image → ≈ 13 ms par image [HYPOTHÈSE], soit 40 % du budget de 30 i/s à 60 MHz.
- Contention Videl : mesurée sur 030 (×1,24 à 320 octets/ligne) ; non mesurée sur CT60.

## Leviers
| Levier | Statut | Document |
|---|---|---|
| N'écrire que les paires changées (copie Fast par tampon) | en place depuis v0.31 | RGB565_OPTIMIZATION.md |
| Écritures longues alignées | en place | — |
| Chevauchement écriture / calcul (pompe) | [EXPÉRIMENTAL], coupé | STPUMP_BENCHMARK.md |
| MOVE16 (lignes entières ou blocs changés) | [EXPÉRIMENTAL], coupé | MOVE16_BENCHMARK.md |
| Mesure du coût ST réel de la boucle de conversion | prêt (STOVL G − H) | CAMPAGNE_CT60_FINALE.md |
| Cache de données / mode d'écriture de la ST-RAM | **non touché** : pas de changement global de cache sans preuve | — |

## Ce que Hatari ne peut pas dire
La vitesse du bus ST, la file d'écriture du 68060, la contention Videl et MOVE16 : toute décision sur ces
points attend la campagne CT60.

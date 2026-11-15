# Hatari 060 / vrai CT60

Calage complet : `/mnt/project-files/cannonball-ct60/mesures/CALAGE_HATARI_CT60.md` (ISOBENCH du fil CyberR,
relancé avec la configuration Hatari de Cannonball). Les coefficients du travail Fallout (configuration Hatari
différente) ne sont **pas** repris tels quels.

| Nature | temps Hatari / temps CT60 réel |
|---|---|
| calcul entier | 1,5 à 3,0 (médiane 2,2) |
| copie Fast → ST | 0,27 (Hatari 3,7 × trop rapide ; réel 5,6 Mo/s) |
| copie Fast → Fast | 0,15 |
| c2p vers ST | 0,85 à 0,91 |

## Estimation Cannonball sur CT60 60 MHz [HYPOTHÈSE]
Rendu [H] 68 ms = environ 65 ms de calcul + environ 3,4 ms d'écriture ST (72 Ko).
- calcul : 65 / 3,0 à 65 / 1,5 → 22 à 43 ms (médiane 30 ms)
- écriture ST : 72 Ko à 5,6 Mo/s → 13 ms
- soit **35 à 56 ms par image (médiane ≈ 43 ms)**, plus environ 0,5 ms par moitié de pas.

Donc, sur la version actuelle : 30 i/s (33 ms) **hors de portée** ; AUTO choisirait probablement 20 i/s
(50 ms), voire 15. La correction par rapport à l'estimation précédente (20 à 30 ms) vient d'un profil v0.39 pris
par erreur en RVB 50 Hz, où 27 % du temps était de l'attente : en VGA le rendu coûte bien ≈ 65-68 ms [H].

À vérifier par la tâche CB-CAL-RCT60-01 (v0.39 instrumentée, en file pour le prochain montage CT60).
Prochain levier : le rendu (sprites, conversion, écriture ST), pas la logique.

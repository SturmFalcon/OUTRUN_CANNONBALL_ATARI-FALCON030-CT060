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

## Estimation Cannonball sur CT60 [HYPOTHÈSE]
Rendu [H] ≈ 46 ms = ≈ 43 ms de calcul + ≈ 3,4 ms d'écriture ST (72 Ko). Le calcul suit les MHz, pas l'écriture ST
(bus du Falcon).

| CT60 | calcul | écriture ST | total par image | AUTO choisirait |
|---|---|---|---|---|
| 60 MHz | ≈ 19 ms (14-28) | ≈ 13 ms | ≈ 32 ms (27-41) | 30 i/s à la limite, sinon 20 |
| 75 MHz | ≈ 15 ms | ≈ 13 ms | ≈ 28 ms | 30 i/s |
| 90 MHz | ≈ 13 ms | ≈ 13 ms | ≈ 26 ms | 30 i/s |

60 i/s (16,7 ms tout compris) est hors de portée en pleine qualité, même à 90 MHz. L'affichage ne dépasse
jamais 60 Hz (règle STurm).

Historique : une première estimation (20-30 ms) venait d'un profil pris en RVB 50 Hz, une deuxième (35-56 ms)
d'une version de mesure ralentie par le compteur STWRITE ; celle-ci est faite sans l'un ni l'autre.
À vérifier par la tâche CB-CAL-RCT60-01 (en file pour le prochain montage CT60).

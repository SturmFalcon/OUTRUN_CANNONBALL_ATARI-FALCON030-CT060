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
Rendu [H] après les optimisations (bcd0565) ≈ 35,1 ms = ≈ 31,7 ms de calcul + ≈ 3,4 ms d'écriture ST (72 Ko).
Le calcul suit les MHz du 68060 (rapport 2,2 mesuré à la fréquence de la CT60 de STurm, supposée 60 MHz),
pas l'écriture ST (bus du Falcon, ≈ 5,6 Mo/s mesurés).

| CT60 | calcul | écriture ST | total par image | budget 30 i/s (33,3 ms) |
|---|---|---|---|---|
| 60 MHz | ≈ 14,4 ms (10,6-21) | ≈ 13 ms | ≈ 27,4 ms (23,5-34) | tenu sauf au pire de la plage |
| 75 MHz | ≈ 11,5 ms | ≈ 13 ms | ≈ 24,5 ms | tenu |
| 90 MHz | ≈ 9,6 ms | ≈ 13 ms | ≈ 22,6 ms | tenu, ≈ 10 ms de marge |

Avant les optimisations de rendu (653ed94) : ≈ 32 ms (27-41) à 60 MHz.

**60 i/s** demande ≤ 16,7 ms tout compris. L'écriture ST seule en prend ≈ 13 : il faudrait un calcul de moins de
4 ms, soit près de 3 fois moins que l'estimation à 90 MHz (≈ 10 ms). Seul un fort chevauchement des écritures ST (pompe, à
mesurer avec STOVL) pourrait changer ce constat ; sans mesure réelle, 60 i/s pleine qualité n'est pas visé.
L'affichage ne dépasse jamais 60 Hz (règle STurm).

Historique : une première estimation (20-30 ms) venait d'un profil pris en RVB 50 Hz, une deuxième (35-56 ms)
d'une version de mesure ralentie par le compteur STWRITE.
À vérifier par la tâche CB-CAL-RCT60-01 (en file pour le prochain montage CT60).

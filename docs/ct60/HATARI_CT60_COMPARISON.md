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
Rendu [H] après les optimisations (c402bd2) ≈ 39,7 ms = ≈ 36,3 ms de calcul + ≈ 3,4 ms d'écriture ST (72 Ko).
Le calcul suit les MHz du 68060 (rapport 2,2 mesuré à la fréquence de la CT60 de STurm, supposée 60 MHz),
pas l'écriture ST (bus du Falcon, ≈ 5,6 Mo/s mesurés).

| CT60 | calcul | écriture ST | total par image | budget 30 i/s (33,3 ms) |
|---|---|---|---|---|
| 60 MHz | ≈ 16,5 ms (12-24) | ≈ 13 ms | ≈ 29,5 ms (25-37) | tenu au milieu de la plage, pas au pire |
| 75 MHz | ≈ 13 ms | ≈ 13 ms | ≈ 26 ms | tenu |
| 90 MHz | ≈ 11 ms | ≈ 13 ms | ≈ 24 ms | tenu, ≈ 9 ms de marge |

Avant les optimisations de rendu (653ed94) : ≈ 32 ms (27-41) à 60 MHz.

**60 i/s** demande ≤ 16,7 ms tout compris. L'écriture ST seule en prend ≈ 13 : il faudrait un calcul de moins de
4 ms, soit 4 fois moins qu'aujourd'hui, même à 90 MHz. Seul un fort chevauchement des écritures ST (pompe, à
mesurer avec STOVL) pourrait changer ce constat ; sans mesure réelle, 60 i/s pleine qualité n'est pas visé.
L'affichage ne dépasse jamais 60 Hz (règle STurm).

Historique : une première estimation (20-30 ms) venait d'un profil pris en RVB 50 Hz, une deuxième (35-56 ms)
d'une version de mesure ralentie par le compteur STWRITE.
À vérifier par la tâche CB-CAL-RCT60-01 (en file pour le prochain montage CT60).

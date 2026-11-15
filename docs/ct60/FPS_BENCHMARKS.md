# Mesures de cadence (images/s) par version

Toutes les lignes ci-dessous sont **[H]** : Hatari 2.6.1-devel, Falcon, 68060 réglé à 32 MHz (le maximum d'Hatari),
`--cpu-exact on`, VGA, EmuTOS 1.3, autoplay (pièce, départ, accélérateur), 400 images après 100 de mise en route.
Hatari n'est **pas** un CT60 : voir HATARI_CT60_COMPARISON.md pour la conversion. Aucune ligne [RCT60] à ce jour.
Données brutes : `/mnt/project-files/cannonball-ct60/mesures/` (dossiers datés, MD5 des programmes).

## Cadence et régularité (réglages par défaut : pleine qualité)

| Version | Boucle | i/s | images à durée irrégulière* | pas logiques/s | coût/image moyen / p95 / max |
|---|---|---|---|---|---|
| v0.39 (4168938a) | 200 Hz, K=2 | 14,8 | 59-89 / 400 | 29,8 | non mesuré (≈ 67 ms d'intervalle) |
| VBL `vsync_lat=1` (4fa33f8) | VBL, K=2 | 14,7 | 52 / 400 | 29,8 | — |
| VBL `vsync_lat=2` | VBL, K=2 | 14,7 | 16 / 400 | 29,8 | — |
| Adaptatif AUTO (fc2dad7) | VBL, n auto | 9,9 | **5 / 400** (395 × 6 VBL) | 29,8 | 74,9 / 98 / 110 ms |
| Adaptatif AUTO, lat 2 | VBL, n auto | 9,9 | **1 / 400** | 29,8 | 75,6 / 98 / 110 ms |
| Adaptatif 15 fixe | VBL, n=4 | 14,0 | 141 / 400 | 29,8 | 67,2 / 90 / 103 ms |
| Adaptatif 30 fixe (réglages légers) | VBL, n=2 | 18,8 | 321 / 400 | 29,9 | 50,8 / 63+ / 92 ms |
| Adaptatif 60 fixe | VBL, n=1 | 15,5 | 399 / 400 | 29,8 | 60,5 / 84 / 101 ms |

\* images restées à l'écran un nombre de VBL différent de la valeur visée (journal CADENCE).

Lecture : sous Hatari 32 MHz aucune cadence ≥ 15 i/s ne tient régulièrement ; AUTO choisit donc 10 i/s,
parfaitement régulières (choix « stabilité avant nombre »). Forcer 15/30/60 donne plus d'images mais irrégulières ;
la vitesse du jeu reste exacte dans tous les cas (29,8-29,9 pas/s, mêmes sommes d'état que v0.39).

## Répartition du coût d'une image [H] (B060Z, AUTO pleine qualité)
- rendu de l'image (sprites, tuiles, route, conversion RGB565, écriture ST) : **68,0 ms**
- une moitié de pas de jeu (logique + route CPU + vint + son) : **1,1 ms**
- écriture ST-RAM : 71,8 Ko par image, dont 95 % de mots réellement changés.

Conséquence : la cadence dépend presque entièrement du rendu ; la logique du jeu est négligeable.
Profil par fonction (v0.39) : sprites ≈ 31 % du travail, conversion+écriture ≈ 16 %, tuiles ≈ 11 %, route ≈ 10 %.

## Par scène
Seule la scène « course, 1re étape, ligne droite puis virages » est couverte par l'autoplay. Menus, attract mode,
étapes suivantes, arrivée : **non mesuré**.

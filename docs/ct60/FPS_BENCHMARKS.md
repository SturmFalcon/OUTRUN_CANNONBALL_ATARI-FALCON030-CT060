# Mesures de cadence (images/s) par version

Toutes les lignes ci-dessous sont **[H]** : Hatari 2.6.1-devel, Falcon, 68060 réglé à 32 MHz (le maximum d'Hatari),
`--cpu-exact on`, VGA, EmuTOS 1.3, autoplay (pièce, départ, accélérateur), 400 images après 100 de mise en route.
Hatari n'est **pas** un CT60 : voir HATARI_CT60_COMPARISON.md pour la conversion. Aucune ligne [RCT60] à ce jour.
Données brutes : `/mnt/project-files/cannonball-ct60/mesures/` (dossiers datés, MD5 des programmes).

## Cadence et régularité (réglages par défaut : pleine qualité)

| Version | Boucle | i/s | images à durée exacte | pas logiques/s | coût/image moyen / p95 / max |
|---|---|---|---|---|---|
| v0.39 (4168938a) | 200 Hz, K=2 | 14,8 | 342 / 401 | 29,8 | non mesuré (≈ 67 ms d'intervalle) |
| VBL `vsync_lat=1` (4fa33f8) | VBL, K=2 | 14,7 | 348 / 400 | 29,8 | — |
| VBL `vsync_lat=2` | VBL, K=2 | 14,7 | 384 / 400 | 29,8 | — |
| Adaptatif 15 fixe (653ed94) | VBL, n=4 | 14,8 | **399 / 400** | 30,0 | 50,1 / 64 / 74 ms |
| Adaptatif 60 fixe | VBL, n=1 | 21,0 | 1 / 400 (ne tient pas) | 29,8 | 45,1 / 54 / 70 ms |
| AUTO, 1re règle de montée (653ed94) | VBL, n auto | 11,8 | 392 / 400 à 12 i/s | 29,8 | 54,4 / 69 / 79 ms |
| **AUTO réglé (montée : moyenne +10 %, crête dans n−1+lat VBL)** | VBL, n auto | **14,4** | 357 × 15 i/s, 36 × 12, 7 × 10, **0 retard** | 29,7 | 50,4 / 63 / 73 ms |
| AUTO réglé, sans ombres | VBL, n auto | 14,4 | 358 × 15 i/s, 0 retard | 29,7 | 47,6 / 60 / 68 ms |
| AUTO réglé, réglages légers | VBL, n auto | 18,5 | 336 × 20 i/s, 45 × 15, 19 × 12 | 29,7 | 34,8 / 42 / 50 ms |
| 15 fixe, sprites réduits + route (c402bd2) | VBL, n=4 | 14,9 | 400 × 15 i/s, 1 en retard | 29,9 | 44,2 / 52 / 59 ms (rendu 39,7) |
| AUTO, sprites réduits + route (c402bd2) | VBL, n auto | 16,1 | 183 × 20 i/s, 169 × 15, 36 × 12, 12 × 10, **0 retard, 0 sautée** | 29,9 | 44,6 / 53 / 59 ms |
| 15 fixe, + conversion + tuiles (c34a268) | VBL, n=4 | 14,9 | 400 × 15 i/s, 3 en retard | 29,9 | 41,5 / 50 / 56 ms (rendu 37,0) |
| **AUTO, + conversion + tuiles (c34a268)** | VBL, n auto | **18,2** | 323 × 20 i/s, 52 × 15, 25 × 12, **0 retard, 0 sautée** | 29,9 | 40,5 / 49 / 55 ms |

« images à durée exacte » : images restées à l'écran exactement le nombre de VBL de leur niveau (journal CADENCE) ;
les autres sont les changements de niveau du début (AUTO part à 20 i/s sous Hatari puis descend).

Lecture : sous Hatari 32 MHz, AUTO tient 15 i/s pleine qualité sans aucune image en retard, et 20 i/s en réglages
légers. Forcer 60 donne plus d'images mais irrégulières ; la vitesse du jeu reste exacte dans tous les cas
(29,7-30 pas/s, mêmes sommes d'état que v0.39).

**Attention** : les versions de mesure compilées avec `-DSTWRITE_COUNT` (compteur d'octets écrits) sont ≈ 40 %
plus lentes au rendu ; aucun chiffre de cadence ne doit venir d'elles (B060W2, B060X/Y/Z écartés).

## Répartition du coût d'une image [H] (B060P, AUTO pleine qualité, avant les optimisations de rendu)
- rendu de l'image (sprites, tuiles, route, conversion RGB565, écriture ST) : **≈ 46 ms**
- une moitié de pas de jeu (logique + route CPU + vint + son) : **≈ 1,1 ms**
- écriture ST-RAM : ≈ 72 Ko par image, dont 95 % de mots réellement changés.

La cadence dépend presque entièrement du rendu. Profil par fonction (hors compteur) : sprites zoomés ≈ 33 %,
conversion couleur + écriture ≈ 16 %, tuiles ≈ 10 %, route ≈ 9 %.

Après les optimisations : rendu 39,7 ms (c402bd2) puis **37,0 ms** (c34a268) au lieu de 45 ; à c402bd2, profil : conversion couleur + écriture
≈ 21 %, route ≈ 11 %, tuiles ≈ 10 %, préparation des rangées de sprites ≈ 5 %. Détail : CT60_60MHZ_OPTIMIZATION.md.

## Par scène
Seule la scène « course, 1re étape, ligne droite puis virages » est couverte par l'autoplay. Menus, attract mode,
étapes suivantes, arrivée : **non mesuré**.

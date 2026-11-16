# Campagne finale sur la vraie CT60 (préparée, **non lancée**)

Lancement **uniquement avec l'accord de STurm**, au prochain montage de la CT60, par la file du banc
(projet `cannonball` à ajouter à `taches/SEULES` ce jour-là). Aucune action manuelle sur le Falcon : chaque
tâche est un TOS en mode AUTO qui écrit son journal dans `JOURNAUX\`. Données ROM : placées par le banc
depuis `CB_ROMS_83.zip` (jamais dans git). Ordre : du plus court au plus long, une hypothèse par groupe.

## Groupe 1 : machine et calage (déjà en file : CB-HW-RCT60-01, CB-CAL-RCT60-01)
| Tâche | Programme | Mesure | Critère |
|---|---|---|---|
| CB-HW | HWTEST.TOS | fréquence mesurée (boucle chronométrée), cookies, mémoire | MHz affiché = fréquence réelle de la carte ±2 % |
| CB-CAL | CBCAL.TOS (v0.39 instrumentée) | coût par image de v0.39 | sert de référence [RCT60] |

## Groupe 2 : écritures ST-RAM (STOVL.TOS, ≈ 1 min)
A ST seul, B calcul seul, C réparti, D pompe K=1/2/4/8, E MOVE16 seul, F MOVE16 réparti,
G conversion réelle vers ST, H même chose vers Fast. Réponses attendues :
- G − H = coût ST réel de la conversion (à comparer aux ≈ 13 ms estimés) ;
- D < A + B nettement → la pompe mérite d'être réglée ; sinon elle reste coupée ;
- E < A → MOVE16 plus rapide que move.l vers la ST-RAM (Fallout [RCT60] : non, +2,6 %).

## Groupe 3 : le jeu, à 15 i/s fixe puis en AUTO (400 images chacun, ≈ 1 min par passe)
| Variante | Build | Option outrun.ini | Statut |
|---|---|---|---|
| Référence v0.39 | CB060 v0.39 bench | — | référence |
| **Version CT60 par défaut** | CB060 actuel bench | — | [H-VALIDÉ] |
| Pompe K = 2, 4, 8 | CB060 actuel bench | `stpump = K` | [EXPÉRIMENTAL] |
| Division 16 bits | `-DSPR_DIVW` | — | [EXPÉRIMENTAL] (+4 % sous Hatari) |
| MOVE16 lignes entières | `-DMOVE16` | — | [EXPÉRIMENTAL] |
| MOVE16 blocs changés | `-DMOVE16_DIFF` | — | [EXPÉRIMENTAL] |
| Cache de lignes rétabli | `-DROWCACHE_ON` | — | comparaison (retiré par défaut sur 060 : −0,6 % [H]) |
Relevés : `render_ms`, `cost_ms` moyen / p95 / max, images en retard et sautées, répartition des niveaux AUTO.
Critère de choix : la variante la plus rapide **et** sans retard en AUTO devient la valeur par défaut ;
images identiques garanties par construction (mêmes routines vérifiées sous Hatari).

## Groupe 4 : contrôles
- Cadence et triple tampon : journal CADENCE (chaque image exactement n VBL), 0 image fausse (build ROWCHECK).
- Son : passe de 3 min en AUTO avec musique FM sur DSP ; journal des retards du son ; écoute par STurm s'il le souhaite.
- Longue durée : 20 min d'autoplay en AUTO (sommes d'état, pas de blocage, pas de dérive de cadence).

## Ce qui ne sera pas fait
- Aucun changement de fréquence de la CT60, aucun réglage de cache global.
- Aucun affichage au-dessus de 60 Hz.

# Détection du matériel au démarrage

Code : `src/main/atari/hwdetect.hpp/.cpp`, `hwdetect_asm.S`. Appelée une fois dans `main()`, avant le son.
Rien n'est modifié dans la machine : ni horloge, ni caches, ni réglage CT60.

| Donnée | Source | Remarque |
|---|---|---|
| Processeur | cookie `_CPU` (30, 40, 60) | 0 si absent (68000) |
| FPU | cookie `_FPU` | valeur brute |
| Machine | cookie `_MCH` | Falcon = 0x30000 |
| CT60/CT63 | cookie `CT60` (0x43543630) | présence + valeur |
| FreeMiNT | cookie `MiNT` | |
| ST-RAM libre | `Mxalloc(-1, 0)` | plus grand bloc |
| Fast-RAM libre | `Mxalloc(-1, 1)` | plus grand bloc |
| Fréquence | mesurée, voir CPU_FREQUENCY_DETECTION.md | jamais lue d'un réglage |

Profils (`HwProfile`) : `030`, `040`, `060-LENT` (< 45 MHz), `060-60MHZ` (45 à 75 MHz), `060-RAPIDE` (> 75 MHz).

Sorties :
- une ligne `HW ...` dans `JOURNAUX\CBBENCH.LOG` (versions de mesure) et dans `HWTEST.LOG` (outil `tools/ct60/hwtest.cpp`) ;
- le menu principal affiche `CPU 68060  nn MHZ` (fréquence mesurée, arrondie).

Le profil ne sert qu'à choisir le **premier** niveau de cadence (`hw_start_level()`). Le choix du
mode AUTO repose ensuite sur le temps réellement mesuré de chaque image (ADAPTIVE_FPS.md), si bien
qu'une erreur de mesure de la fréquence ne coûte que les 3 premières secondes.

## État de validation

| Mesure | [H] Hatari 060/32 MHz cycle-exact | [RCT60] |
|---|---|---|
| cookies, mémoire | corrects (cpu=60, mch=30000, ct60 absent, fast 131 Mo) | non mesuré (tâche CB-HW-RCT60-01 en file) |
| fréquence | 18,9 MHz affichés pour 32 MHz réglés : le coût du cycle d'Hatari 060 n'est pas celui du 68060 | non mesuré |

# Construire la version CT60

Chaîne : `m68k-atari-mint` GCC 4.6.4 (paquets .deb sur la branche `toolchain` du fork : `dpkg -i` fonctionne sur
Ubuntu 24.04). Détails généraux : README_ATARI.md, section « Building ».

```bash
# Jeu, 68060 (CT60/CT63)
CPU=68060 EXTRA="-msoft-float -DPLATFORM_FALCON" LINKCPU=68000 OUT=CB060.TOS bash build_atari.sh
# Jeu, 68030 (Falcon d'origine) : même code de jeu, sans les routines 040/060
CPU=68030 EXTRA="-msoft-float -DPLATFORM_FALCON" LINKCPU=68000 OUT=CB030.TOS bash build_atari.sh
# Outil STOVL (060 seulement)
bash tools/ct60/build_stovl.sh
```
Le script n'est pas exécutable dans le dépôt : toujours `bash build_atari.sh`. Le binaire 060 ne doit jamais tourner
sur 030, ni l'inverse.

## Builds de mesure et de vérification (jamais livrés)
| Drapeaux | Effet |
|---|---|
| `-DAUTOPLAY -DBENCH_N=400` | partie automatique, ligne VBENCH/VLEVELS en fin de course |
| `-DSTATE_CRC` | somme de l'état du jeu tous les 300 pas (doit égaler v0.39) |
| `-DSHOTHALVES -DDUMP_INDEX` | images du moteur aux pas 400…2400, à comparer à v0.39 avec `cmp` |
| `-DROWCHECK`, `-DPUMPCHECK` | écran ST contre conversion, sans et avec pompe |
| `-DMENUSHOT` | captures des deux pages du menu puis sortie |
| `-DSTWRITE_COUNT` | compte les octets écrits en ST ; **ralentit le rendu d'environ 40 % : jamais pour chronométrer** |
| `-DSPR_NOTRIM` | sprites sans réduction des rangées (comparaison) |
| `-DPUMP_NOCALLS` | retire les appels de la pompe (comparaison) |

## Vérifier qu'un build de référence v0.39 est intact
Tag v0.39-beta, même commande : CB030.TOS md5 `9ea0b6869f5e5fa12fb60689a626aa2d`,
CB060.TOS md5 `4168938aba86a5d6cbce23f1dda30a53`.

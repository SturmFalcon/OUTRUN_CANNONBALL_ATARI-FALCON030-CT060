# Ghidra MCP dans le travail Cannonball

## État vérifié le 16/11/2026 [ANALYSE]
| Point | Constat | Preuve |
|---|---|---|
| Ghidra 12.2 + Java 25 (PC du banc) | présent, **non utilisé** par ghidra-mcp | rapport du fil banc, listing avant/après identique |
| ghidra-mcp (bethington) 7.0.0 | installé sur le PC du banc, isolé : Ghidra **12.1.4** + JDK 21 dans `C:\Users\Heahu\Outils\ghidra-mcp\` | la release 7.0.0 cible 12.1.4 / Java 21 (extension.properties, pom) ; la version 12.2 n'existe que sur une branche amont non publiée |
| Connexion MCP | serveur headless sur 127.0.0.1:8089, scripts désactivés, accès limité à `essais\` | `/mnt/project-files/ghidra-mcp/RAPPORT_GHIDRA_MCP.md` |
| Ouvrir un programme, décompiler, désassembler, xrefs, renommer, commenter | **fonctionne** (vérifié sur des copies Rolling Thunder puis Cannonball) | preuves JSON du fil banc |
| Émulation P-code, flux de données | ne fonctionne pas en 68k (émulateur x86 seulement), flux partiel | idem |
| Ce fil (session cloud) | **aucun accès direct** au serveur MCP : il ne tourne que sur le PC du banc | aucun outil MCP Ghidra dans la session |

## Comment Cannonball s'en sert
- Le fil Cannonball prépare les exécutables (copies, aucune donnée ROM) et leurs symboles sur la branche
  `claude/project-thread-5gml39` du dépôt du banc (`ghidra-cannonball/`), avec la demande d'analyse.
- La session PC du banc les importe dans un projet **neuf** `CB_TEST`, en 68040 (Ghidra n'a pas de 68060),
  à la base $10000, applique les symboles (`nm`), puis exporte désassemblage, décompilation, xrefs et diff.
- Résultats : `ghidra-cannonball/resultats/` (commit a867978), copie dans
  `/mnt/project-files/cannonball-ct60/ghidra/resultats/`.
- Recoupement local sans Ghidra : `tools/ct60/funcmap.py` (objdump + symboles), même verdict sur les
  routines critiques.

## Règles
- Ghidra explique le code machine ; il ne mesure rien. Toute conclusion de vitesse vient d'une mesure.
- Les sorties sont étiquetées [ANALYSE]. Le décompilateur peut se tromper sur les registres matériels
  (non volatils) : le désassemblage fait foi.
- Les valeurs $FFFFFF / $FFFFFFF0 relevées dans les routines sont des masques, pas des registres.

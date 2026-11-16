# MOVE16 : mesures

MOVE16 copie 16 octets alignés en une rafale (68040/68060). Statut des variantes : **[EXPÉRIMENTAL]**.

## Variantes dans le jeu
| Build | Ce qui part en MOVE16 | Octets ST par image [H] | Vérification [H] |
|---|---|---|---|
| défaut | rien (move.l par paire changée) | ≈ 72 Ko | ROWCHECK 0 |
| `-DMOVE16` | chaque ligne convertie entière (Fast → ST) | ≈ 143 Ko | existant |
| `-DMOVE16_DIFF` | seulement les blocs de 16 octets qui contiennent un changement | > 72 Ko (blocs entiers) | ROWCHECK 0/4140 |
Code machine contrôlé : `move16 (a4)+,(a2)+`, tampon Fast aligné sur 16 au lancement, destination alignée
(`use_move16` exige ligne et décalage multiples de 16), 68060 seulement.

## Micro-mesures STOVL
| Test | Hatari [H] | Vraie CT60 |
|---|---|---|
| A move.l, 72 Ko vers ST | 4,8 ms | non mesuré |
| E MOVE16, 72 Ko vers ST | 2,7 ms | non mesuré |
| F MOVE16 réparti dans le calcul | sans valeur (pas de chevauchement émulé) | non mesuré |
Rappel [RCT60] Fallout : MOVE16 vers la ST-RAM sans gain (+2,6 %). Hatari est 3,7 fois trop rapide sur
l'écriture ST : ces chiffres [H] ne classent rien.

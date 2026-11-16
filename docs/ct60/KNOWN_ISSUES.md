# Problèmes connus et limites (branche ct60-60mhz-optimization)

- **Aucune mesure sur la vraie CT60.** Tous les chiffres sont [H] ou [HYPOTHÈSE]. Les seuils d'AUTO (marge 10 %,
  ≈ 3 s avant de monter) ne sont réglés que sous Hatari.
- **Fréquence affichée dans le menu** : mesurée par une boucle chronométrée ; l'hypothèse « 16 cycles par tour sur
  68060 » n'est pas vérifiée sur matériel (tâche CB-HW-RCT60-01 en file).
- **Pompe d'écriture ST (`stpump`)** : coupée par défaut. Sous Hatari elle coûte ≈ 5 ms par image ; son gain
  éventuel n'existe que sur la vraie CT60 (voir ST_WRITE_OVERLAP.md).
- **60 i/s pleine qualité** : hors de portée selon l'estimation (l'écriture ST seule prend ≈ 13 ms sur 16,7).
  Le réglage manuel 60 existe mais saccade si la machine ne suit pas ; AUTO ne le choisit que si les mesures
  le permettent.
- **40 et 50 i/s non proposés** : en VGA 60 Hz ils alterneraient des images de 1 et 2 VBL (saccade). 25 i/s
  n'est régulier qu'en RVB 50 Hz.
- **`vscale` < 100** ne réduit pas le nombre de lignes dans la boucle vsync (à vérifier).
- **Scènes non mesurées** : seule la 1re étape en course est couverte ; menus, attract mode, autres étapes et
  arrivée sont « non mesuré ».

- **Ghidra MCP** ne tourne que sur le PC du banc (Ghidra 12.1.4 isolé, la release 7.0.0 ne gère pas 12.2) ;
  pas de 68060 dans Ghidra (décodage 68040), pas d'émulation P-code en 68k.
- **Variantes expérimentales** (`-DSPR_DIVW`, `-DSPR_EARLYNONE`, `-DSPR_OUTEXIT`, `-DMOVE16`, `-DMOVE16_DIFF`, `stpump`) : non
  classées tant que la vraie CT60 ne les a pas mesurées.

- **Build 030** : compilé, non vérifié image par image sous Hatari 030 (trop lent pour atteindre les points de
  comparaison en 25 min) ; son code de jeu modifié est commun avec le 060, vérifié.

## Corrigé sur cette branche
- **Pixels faux avec la boucle VBL** (6e12bf0) : le tampon du jeu était écrit dans `screenpt` ($45E), et TOS
  rechargeait l'adresse au mauvais moment ; 220 images fausses sur 500 [H]. Désormais seuls les registres Videl
  sont écrits et $45E reste à 0 : 0 image fausse.

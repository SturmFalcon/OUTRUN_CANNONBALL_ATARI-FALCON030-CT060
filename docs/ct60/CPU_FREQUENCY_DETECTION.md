# Mesure de la fréquence réelle du 68060

## Méthode
- Boucle `atari_hw_spin` : 16 `add.l d1,d1` **dépendants** (chacun attend le résultat du précédent, donc
  aucun appariement superscalaire possible) puis `subq.l`/`bne`, alignée sur 16 octets.
- Coût supposé (manuels Motorola, code en cache) : 68060 16 cycles/tour, 68040 18, 68030 40.
- Comptage contre le compteur 200 Hz (`$4BA`) en superviseur, départ sur un front, 32 tops (160 ms).
- Deux mesures, la plus rapide est gardée (l'autre a pu être ralentie par une interruption).
- MHz × 10 = tours × cycles / (tops × 500), en 32 bits seulement : les aides 64 bits de libgcc
  sont du code 68020 qui peut piéger sur 68060 (exception 61 vue sur CT60).

## Limites connues
- Les interruptions restent actives : leur temps compte comme du temps de boucle (quelques dixièmes de %).
- La valeur dépend de l'hypothèse « 16 cycles/tour » : **non vérifiée sur CT60 réel**. La tâche
  `CB-HW-RCT60-01` (HWTEST.TOS, 3 passes) la vérifiera au prochain montage de la CT60.
- Sous Hatari 060 réglé à 32 MHz avec cycle-exact, la boucle donne 18,9 MHz [H] (4,7 sans cycle-exact) :
  Hatari ne modélise pas le coût par instruction du 68060. Aucune conclusion de fréquence ne se tire d'Hatari.

## Usage
La fréquence n'est **jamais** utilisée pour modifier la machine, ni pour fixer seule la cadence :
elle donne le niveau de départ du mode AUTO et s'affiche dans le menu. Table de départ ([HYPOTHÈSE],
à recaler sur mesures réelles) : 060 < 45 MHz → 20 i/s ; 060 ≥ 45 MHz → 30 i/s ; 030/040 : pas de mode VBL.

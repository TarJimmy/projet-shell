#ifndef __COMMANDE_H
#define __COMMANDE_H

#include "readcmd.h"
#include "csapp.h"

#define ERR_COMMANDE -1
/*
* Entrée : une commande sous la forme struct cmdline*
* Sortie : void
* Effet de bord : affiche chaque commande contenue dans l
*/
void afficherCmd(struct cmdline *l);

/*
* Entrée : une commande sous la forme struct cmdline*, un entier
* Sortie : void
* Effet de bord : affiche la commande i de l si elle existe, rien sinon
*/
void afficherOneCmd(struct cmdline *l, int i);

/*
* Entrée : une structure de commande
* Sortie : un entier n représentant le nombre de commandes contenues dans l->seq (l->seq[n-1] représente la dernière commande, l->seq[n] == 0)
*/
int tailleCmd(struct cmdline *l);

/*
* Entrée : un groupe de commandes sous la forme struct cmdline*
* Sortie :  
* Effet de bord : exécute les commandes, écrit l'erreur s'il y en a une
*/
void executeCmd(struct cmdline *l);

/*
* Entrée : un groupe de commandes sous la forme struct cmdline*, un entier
* Sortie : OK_EXEC si la commande s’est correctement exécutée, ERREUR_EXEC si l’exécution a causé une erreur (à améliorer)
* Effet de bord : exécute la commande i de la structure l si l->seq[i] existe
*/
int executeOneCmd(struct cmdline *l, char** cmd);

#endif
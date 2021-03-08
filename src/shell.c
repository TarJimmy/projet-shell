/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"
#include "commande.h"

#define CHAR_STOP_SHELL "quit"

/* Handler pour SIGCHLD */
void child_handler(int sig) {
	int status;
	// le père récupère un fils terminé (-1), mais ne se bloque pas en l’attendant (WNOHANG)
	// il récupère aussi les fils « perdus » en demandant les informations des fils qui ne sont pas suivis
	waitpid(-1, &status, WNOHANG | WUNTRACED);

	// hélas, le handler ne peut pas renvoyer d’informations d’erreurs à ExecuteCmd… Cela peut poser souci dans une chaîne de commandes…
	return;
}

/**
 * Mode Debug  : make DEBUG=0
 * Mode normal : make 
*/
int main() {
	#ifdef DEBUG
	printf("-----MODE DEBUG-----\n");
	#endif

	// installation du handler pour SIGCHLD
	Signal(SIGCHLD, child_handler);

	while (1) {
		struct cmdline *l;
		fflush(stdout);
		printf("shell> ");
		fflush(stdout);
		l = readcmd();

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}
		/* Syntax error, read another command */
		if (!l->seq || !l->seq[0] || l->err) {
			#ifdef DEBUG
			printf("error: %s\n", l->err);
			#endif
			continue;
		}

		/* User wants quit shell : command "quit" */
		if (strcmp(*l->seq[0], CHAR_STOP_SHELL) == 0) {
			// besoin de free la structure ?
			printf("exit\n");
			exit(0);
		}

		/* Exécute la ligne de commande */
		executeCmd(l);
		
		/* [DEBUG] Display in < and out > arguments and Display each command of the pipe */
		#ifdef DEBUG
		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);

		afficherCmd(l);
		#endif
	}
}
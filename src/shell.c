/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"
#include "commande.h"

#define CHAR_STOP_SHELL "quit"

/**
 * Mode Debug  : make DEBUG=0
 * Mode normal : make 
*/
int main() {
	#ifdef DEBUG
	printf("-----MODE DEBUG-----\n");
	#endif
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

		/* Exécute la commande (simple pour l’instant) */
		executeCmd(l);
		
		/* [DEBUG] Display in < and out > arguments and Display each command of the pipe */
		#ifdef DEBUG
		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);

		afficherCmd(l);
		#endif
	}
}
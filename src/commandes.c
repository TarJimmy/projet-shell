#include "commande.h"

// cet import crée une variable globale errno qui contient le code d'erreur de la dernière erreur rencontrée
#include <errno.h>

#define NOT_USE -2

/**
 * Affiche l'erreur sur la sortie d'erreur standard 
**/ 
void afficheError(int error, char* trigger) {
    fprintf(stderr, "\x1B[31mError : %s ", trigger);
	switch (error) {
    // erreur correspondant à une commande inexistante
	case ERR_COMMANDE:
		fprintf(stderr, "command not found");
		break;
    // toutes les autres erreurs
	default:
		fprintf(stderr, "%s", strerror(error));
		break;
	}
	fprintf(stderr, "\x1B[0m\n");
}


void afficherCmd(struct cmdline *l) {
    // appelle afficherOneCmd pour chaque commande de la séquence
    for (int i = 0; l->seq[i] != 0; i++) {
        printf("seq[%d]: ", i);
		afficherOneCmd(l, i);
        printf("\n");
    }
}


void afficherOneCmd(struct cmdline *l, int i) {

    #ifdef DEBUG
    //printf("tailleCmd: %d\n", tailleCmd(l));
    #endif
    // en cas d’indice incorrect, n’affiche rien (ne renvoie pas d’erreur)
    if (i >= 0 && i < tailleCmd(l)) {
        // cmd représente UNE commande de la séquence. C’est donc un char** (tableau de strings, chaque string est un argument de la commande)
        char **cmd = l->seq[i];

        // imprime chaque argument de la commande à la suite
        // on ne peut pas faire printf(cmd) tout court car c’est un tableau de chaînes, par une chaîne unique
	    for (int j = 0; cmd[j] != 0; j++) {
	    	printf("%s ", cmd[j]);
	    }
    }
}

/* Renvoie un entier représentant la taille de la ligne de commandes passée en paramètre */
int tailleCmd(struct cmdline *l) {
    int taille = 0;
    
    if (!l->err) {
        do {
            taille++;
        } while (l->seq[taille] != 0);
    }
    return taille;
}

// TODO : gestion des pipes et redirections
// cette première version n’exécute que les commandes simples
void executeCmd(struct cmdline *l) { 
    char* in = NULL;
    char* out = NULL;
    for (int i = 0; i < tailleCmd(l); i++) {
        #ifdef DEBUG
        printf("Number : i %d\n", i);
        printf("Number : i==0 %d\n", i==0);
        printf("Number : tailleCmd(l) -1 : %d\n", (tailleCmd(l) - 1));
        printf("l->in : %s\n", l->in);
        printf("l->out : %s\n", l->out);
        #endif
        if (i == 0) {
            in = l->in;
        } 
        if (i == tailleCmd(l) - 1) {
            out = l->out;
        }
        executeOneCmd(l, l->seq[i], in, out);
    }
    return;
}

/**
 * TODO : version basique ne gérant ni pipes, ni redirections, ni tâches en arrière-plan
 * cmd correspond à la séquence d’arguments, par exemple ["ls", "-l"]
 * cmd[0] est le nom de la commande, par exemple "ls"
 * fd est le descripteur de fichier où l'on doit travailler dessus
**/ 
void executeOneCmd(struct cmdline *l, char** cmd, char* in, char* out) {
    
    // vérification de la validité de l’indice
    pid_t pid_fils = Fork();

    // partie du fils
    if (pid_fils == 0) {
        int fd[2];
        if (in != NULL) {
            if ((fd[0] = open(in, O_RDONLY)) == -1) {
                afficheError(errno, in);
                return;
            };

            if (dup2(fd[0], STDIN_FILENO) == -1) {
                afficheError(errno, in);
                return;
            }
        }

        if (out != NULL) {
            if ((fd[1] = open(out, O_WRONLY | O_CREAT, S_IRWXU)) == -1) {
                //errno pour afficher celui ci au cas ou les fonctions suivantes déclenchent aussi une erreur
                afficheError(errno, out);
                return;
            }
            
            if (dup2(fd[1], STDOUT_FILENO) == -1) {
                //Sauvegarde errno pour afficher celui ci au cas ou les fonctions suivantes déclenchent aussi une erreur
                afficheError(errno, out);
                return;
            }
        }
        // env est un groupe de variables d’environnement, on n’y inclut que le PATH
        #ifdef DEBUG
        printf("Execution de EXECVE dans executeOneCmd, pid : %d\n", getpid());
        #endif

       //lie la sortie standard à travers le second pipe à l’entrée standard du processus fils.
        if(execvp(cmd[0], cmd) < 0) {
            #ifdef DEBUG
            printf("Return with error");
            #endif
            kill(getpid(), SIGSEGV);
        }

        // point théoriquement jamais atteint puisque execvp « remplace » le processus (utile pour supprimer le warning)
        exit(EXIT_SUCCESS);

    } else { // father
        int status;
        
        #ifdef DEBUG
        printf("Wait Pid dans executeCmdOne : %d\n", pid_fils);
        printf("Return father");
        #endif
        // le père attend la fin du fils qui exécute la commande
        Waitpid(-1, &status, 0);
        // code renvoyé dépend du code de terminaison du fils
        if (WIFEXITED(status)) {
            return;
        } else {
            afficheError(ERR_COMMANDE, out);
            return;
        }
    }
}
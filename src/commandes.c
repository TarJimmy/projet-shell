#include "commande.h"
#include <errno.h>

 #define NOT_USE -2

/**
 * Affiche l'erreur sur la sortie d'erreur standard 
**/ 
void afficheError(int error, char* trigger) {
    fprintf(stderr, "\x1B[31mError : %s ", trigger);
	switch (error) {
	case ERR_COMMANDE:
		fprintf(stderr, "command not found");
		break;
	default:
		fprintf(stderr, "%s", strerror(error));
		break;
	}
	fprintf(stderr, "\x1B[0m\n");
}

/**
 * Retourne l'erreur contenu dans errno si une a été généré, sinon renvoie EXIT_SUCCESS
**/
int restaureIO(int* fd, int out, int in) {
    uint8_t generateError = 1;
    if (fd[0] != NOT_USE) {
        //Restauration de l'entré standart de base
        if (close(fd[0]) == -1) generateError = 0;
        if (in != NOT_USE && dup2(in, STDIN_FILENO) == -1) generateError = 0;
        if (in != NOT_USE && close(in) == -1) generateError = 0;
    }
    if (fd[1] != NOT_USE) {
        //Restauration de l'entré standart de base
        if (close(fd[1]) == -1) generateError = 0;
        if (out != NOT_USE && dup2(out, STDOUT_FILENO) == -1) generateError = 0;
        if (out != NOT_USE && close(out) == -1) generateError = 0;
    }
    return generateError != 1 ? errno : EXIT_SUCCESS;
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
    int fd[2], in, out;
    fd[0] = NOT_USE;
    fd[1] = NOT_USE;
    in = NOT_USE;
    out = NOT_USE;
    //Si on l'utilisateur veut rediriger la sortie à un fichier spécifique
    if (l->in) {
        fd[0] = open(l->in, O_RDONLY);

        if ((fd[0] = open(l->in, O_RDONLY)) == -1) {
            afficheError(errno, l->in);
        };
    
        if((in = dup(STDIN_FILENO)) == -1) {
            afficheError(errno, l->in);
        }

        if (dup2(fd[0], in) == -1) {
            if (restaureIO(fd, out, in) != EXIT_SUCCESS) {
                afficheError(errno, "");
            }
            return;
        }
    }

    if (l->out) {

        if ((fd[1] = open(l->out, O_WRONLY | O_CREAT)) == -1) {
            //Sauvegarde errno pour afficher celui ci au cas ou les fonctions suivantes déclenchent aussi une erreur
            afficheError(errno, "");
            if (restaureIO(fd, out, in) != EXIT_SUCCESS) {
                afficheError(errno, "");
            }
            return;
        }
        if((out = dup(STDOUT_FILENO)) != -1) {
            //Sauvegarde errno pour afficher celui ci au cas ou les fonctions suivantes déclenchent aussi une erreur
            afficheError(errno, l->out);
            if (restaureIO(fd, out, in) != EXIT_SUCCESS) {
                afficheError(errno, "");
            }
            return;
        };
        
        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            //Sauvegarde errno pour afficher celui ci au cas ou les fonctions suivantes déclenchent aussi une erreur
            afficheError(errno, l->out);
            if (restaureIO(fd, out, in) != EXIT_SUCCESS) {
                afficheError(errno, "");
            }
            return;
        }
    }
    int err;
    for (int i = 0; i < tailleCmd(l); i++) {
        if ((err = executeOneCmd(l, l->seq[i])) != EXIT_SUCCESS) {
            afficheError(err, *l->seq[0]);
            if (restaureIO(fd, out, in) != EXIT_SUCCESS) {
                afficheError(errno, "");
            }
            return;
        }
    }
    
    if (restaureIO(fd, out, in) != EXIT_SUCCESS) {
        afficheError(errno, "");
    }
    return;
}

/**
 * TODO : version basique ne gérant ni pipes, ni redirections, ni tâches en arrière-plan
 * cmd correspond à la séquence d’arguments, par exemple ["ls", "-l"]
 * cmd[0] est le nom de la commande, par exemple "ls"
 * fd est le descripteur de fichier où l'on doit trvailler dessus
**/ 
int executeOneCmd(struct cmdline *l, char** cmd) {
    
    // vérification de la validité de l’indice
    pid_t pid_fils = Fork();

    // partie du fils
    if (pid_fils == 0) {
        // env est un groupe de variables d’environnement, on n’y inclut que le PATH
        #ifdef DEBUG
        printf("Execution de EXECVE dans executeOneCmd\n");
        #endif
        /**
        int sizePath = strlen(getenv("PATH")) + strlen("PATH=");
        char path[sizePath];
        strcpy(path, "PATH=");
        strcat(path, getenv("PATH"));

        char *const envList = { "HOME=/root", path, NULL };
        **/
       //lie la sortie standard à travers le second pipe à l’entrée standard du processus fils.
        if(execvp(cmd[0], cmd) < 0) {
            kill(getpid(), SIGSEGV);
        }
        // point théoriquement jamais atteint puisque Execve « remplace » le processus (utile pour supprimer le warning)
        printf("Coucou Success");
        exit(EXIT_SUCCESS);

    } else { // father
        int status;
        
        #ifdef DEBUG
        //printf("Wait Pid dans executeCmdOne : %d\n", pid_fils);
        #endif
        // le père attend la fin du fils qui exécute la commande
        Waitpid(pid_fils, &status, 0);  
        // code renvoyé dépend du code de terminaison du fils
        if (WIFEXITED(status)) {
            return EXIT_SUCCESS;
        } else {
            printf("coucou");
            return ERR_COMMANDE;
        }
    }
}
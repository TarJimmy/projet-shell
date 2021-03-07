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

// réalise les commandes, en gérant les redirections et partage de données entre processus
void executeCmd(struct cmdline *l) { 
    int pipefd[2];
    int save_dp[2];
    // On sauvegarde l'entrée et la sortie standart courante
    save_dp[0] = dup(STDIN_FILENO);
    save_dp[1] = dup(STDOUT_FILENO);
    for (int i = 0; i < tailleCmd(l); i++) {
        #ifdef DEBUG
        printf("Number : i %d\n", i);
        printf("Number : i==0 %d\n", i==0);
        printf("Number : tailleCmd(l) : %d\n", (tailleCmd(l)));
        printf("l->in : %s\n", l->in);
        printf("l->out : %s\n", l->out);
        #endif
        if (executeOneCmd(l,  pipefd, i) == EXIT_FAILURE) {
            break;
        }
    }
    // On replace l'entrée et le sortie d'origine
    dup2(save_dp[0], STDIN_FILENO);
    dup2(save_dp[1], STDOUT_FILENO);
    return;
}

/**
 * pipefd est le tableau que l'on utilise pour créer la pipe
 * index est le numéro de commande à réaliser
**/ 
int executeOneCmd(struct cmdline *l, int pipefd[], int index) {
    //Initialise la pipe entre le ère et le fils
    if (pipe(pipefd) == -1) {
        afficheError(errno, "pipe");
        return EXIT_FAILURE;
    }

    #ifdef DEBUG
    if (l->background[index] == 0) {
        printf("Commande à lancer au premier plan\n");
    } else {
        printf("Commande à lancer en arrière plan\n");
    }
    #endif
    
    pid_t pid_fils = Fork();

    // partie du fils
    if (pid_fils == 0) {
        int fd[2];
        //Si on est 1er on regarde s'il n'a pas un fichier en entrée
        if (index == 0 && l->in) {
            #ifdef DEBUG
            printf("-----index == 0 && l->in-----");
            #endif
            if ((fd[0] = open(l->in, O_RDONLY)) == -1) {
                afficheError(errno, l->in);
                kill(getpid(), SIGSEGV);
            }

            if (dup2(fd[0], STDIN_FILENO) == -1) {
                afficheError(errno, l->in);
                kill(getpid(), SIGSEGV);
            }
        }
        //On ferme la lecture du pipe car on ne l'utilise pas dans le fils
        if(close(pipefd[0]) < 0 ) { 
            afficheError(errno, *l->seq[index]);
            kill(getpid(), SIGSEGV);
        }

        /**
         * Si on est le dernier, on modifie la sortie de la commande si elle est précisé
         * Si on n'est pas le dernier, on écris dans la sortie de la pipe
         * Si aucune de ces conditions n'est réalisé, le programme écrira la réponse sur la sortie standard
        **/ 
        if ((index == tailleCmd(l) - 1) && l->out) {
            if ((fd[1] = open(l->out, O_WRONLY | O_CREAT, S_IRWXU)) == -1) {
                afficheError(errno, l->out);
                kill(getpid(), SIGSEGV);
            }
            
            if (dup2(fd[1], STDOUT_FILENO) == -1) {
                afficheError(errno, l->out);
                kill(getpid(), SIGSEGV);
            }
        } else if (index < tailleCmd(l) - 1) {
            //On place la sortie sur le write pipe
            if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
                afficheError(errno, *l->seq[index]);
                kill(getpid(), SIGSEGV);
            }
        } 

        #ifdef DEBUG
        printf("Execution de EXECVE dans executeOneCmd, pid : %d\n", getpid());
        #endif
        // On execute la commande
        if(execvp(l->seq[index][0], l->seq[index]) < 0) {
            #ifdef DEBUG
            printf("Return with error\n");
            #endif

            if (l->background[index] == 1) {
                afficheError(ERR_COMMANDE, *l->seq[index]);
            }
            kill(getpid(), SIGSEGV);
        }

        // point théoriquement jamais atteint puisque execvp « remplace » le processus (utile pour supprimer le warning)
        exit(EXIT_SUCCESS);

    } else { // père
        int status;
        
        #ifdef DEBUG
        printf("Wait Pid dans executeCmdOne : %d\n", pid_fils);
        printf("Return to father\n");
        #endif

        //On ferme l'entrée pour écrire et on replace la lecture de la pipe sur l'entrée standart standard
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);

        // si la commande courante est à lancer en premier plan
        if (l->background[index] == 0) {
            // le père attend la fin du fils qui exécute la commande
            Waitpid(-1, &status, 0);
            // code renvoyé dépend du code de terminaison du fils
            if (WIFEXITED(status)) {
                return EXIT_SUCCESS;
            } else {
                // Si la commande courante a eu un soucis, l'erreur est affiché et on précise que le programme a eu une erreur
                afficheError(ERR_COMMANDE, *l->seq[index]);
                return EXIT_FAILURE;
            }
        }
        // si la commande courante est à lancer en arrière-plan, le père n’attend pas sa terminaison
        // cependant, cela veut dire qu’il ne peut pas renvoyer s’il y a eu une erreur…
        return EXIT_SUCCESS;
        
    }
}

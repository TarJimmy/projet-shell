#include "commande.h"

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
int executeCmd(struct cmdline *l) {
    int fd[2], in, out;
    //Si on l'utilisateur veut rediriger la sortie à un fichier spécifique
    if (l->in) {
        fd[0] = open(l->in, O_RDONLY);

        if (fd[0] == -1) {
            perror("Erreur: Le fichier d'entrée est introuvable");
            return -4;
        }

        in = dup(STDIN_FILENO); // -> stdin

        if (dup2(fd[0], in) == -1) {
            perror("Erreur de redirection dans le fichier");
            return -5;
        }
    }

    if (l->out) {
        fd[1] = open(l->out, O_WRONLY | O_CREAT);

        if (fd[1] == -1) {
            printf("%s : permission denied", l->in);
            return -2;
        }

        out = dup(STDOUT_FILENO); //-> stdout

        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            perror("Erreur de redirection dans le fichier");
            return -3;
        }
    }
    
    for (int i = 0; i < tailleCmd(l); i++) {
        executeOneCmd(l, l->seq[i]);
    }

    if (l->in) {
        close(fd[0]);
        dup2(in, STDIN_FILENO); 
        close(in);
    }
    if (l->out) {
        close(fd[1]);
        dup2(out, STDOUT_FILENO);
        close(out);
    }
    
    
    return 0;
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
        Execvp(cmd[0], cmd);
        // point théoriquement jamais atteint puisque Execve « remplace » le processus (utile pour supprimer le warning)
        exit(EXIT_SUCCESS);

    // partie du père
    } else {
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
            return ERREUR_EXEC;
        }
    }
}
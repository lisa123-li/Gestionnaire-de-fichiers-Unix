#include "file_system.h"

/**
 * Pourcentage d'implication :
 * GOBALOUKICHENIN Bala : 33%
 * DEHOUCHE Lisa : 33%
 * GABITA William : 33%
 * Fonction principale du système de fichiers
 *
 * Cette fonction implémente une interface en ligne de commande pour interagir
 * avec le système de fichiers personnalisé
 */
 int main() {
    const char* nom_partition = "partition.bin";

    // Vérifier si la partition existe
    FILE* test = fopen(nom_partition, "rb");
    if (test) {
        fclose(test);
        printf("Chargement d'une partition existante...\n");
        charger_partition(nom_partition);
    } else {
        printf("Création d'une nouvelle partition...\n");
        initialiser_partition(nom_partition);
    }

    // Interface utilisateur simple
    char commande[100];
    char param1[MAX_NOM_FICHIER];
    char param2[MAX_NOM_FICHIER];

    printf("\nGestionnaire de fichiers - Tapez 'aide' pour voir les commandes disponibles\n");

    while (1) {
        printf("\n> ");
        if (fgets(commande, sizeof(commande), stdin) == NULL) {
            break;
        }

        commande[strcspn(commande, "\n")] = 0;

        // Réinitialiser les paramètres
        param1[0] = '\0';
        param2[0] = '\0';

        if (strcmp(commande, "aide") == 0) {
            printf("================ COMMANDES DISPONIBLES ================\n\n");

            // Navigation et affichage
            printf("NAVIGATION ET AFFICHAGE:\n");
            printf("  cd <rep>        - Changer de répertoire\n");
            printf("  ls              - Afficher le contenu du répertoire\n\n");
            printf("  ls -i  <nom>         - Afficher le contenu d'un inode'\n\n");

            // Gestion des fichiers et répertoires
            printf("CRÉATION ET SUPPRESSION:\n");
            printf("  mkdir <nom>     - Créer un répertoire\n");
            printf("  rm <nom>        - Supprimer un fichier ou répertoire\n");
            printf("  touch <nom>     - Créer un fichier vide\n\n");
            printf("  save <fichier>  - Sauvegarde de l'état actuel de la partition\n");
            printf("  load <fichier>  - Restauration d’une partition depuis un fichier de sauvegarde\n");

            // Manipulation et contenu
            printf("MANIPULATION DE CONTENU:\n");
            printf("  cat <nom>       - Afficher le contenu d'un fichier\n");
            printf("  cp <src> <dest> - Copier un fichier\n");
            printf("  mv <src> <dest> - Déplacer un fichier\n");
            printf("  write <nom>     - Écrire dans un fichier\n\n");
            printf("  defrag          - Défragmentation en réorganisant les blocs\n");

            // Liens et attributs
            printf("LIENS ET ATTRIBUTS:\n");
            printf("  chmod <nom><droit> - Modifier les droits d'un fichier (notation symbolique)\n");
            printf("  ln <src> <dest>    - Créer un lien physique\n");
            printf("  lns <src> <dest>   - Créer un lien symbolique\n\n");

            // Commande de sortie
            printf("SYSTÈME:\n");
            printf("  quit            - Quitter le programme\n");
            printf("\n====================================================\n");

        } else if (strcmp(commande, "ls") == 0) {
            afficher_repertoire(inode_courant);

        } else if (strncmp(commande, "cd ", 3) == 0) {
            if (sscanf(commande, "cd %s", param1) == 1) {
                changer_repertoire(param1);
            } else {
                erreur("Usage: cd <repertoire>");
            }

        } else if (strncmp(commande, "mkdir ", 6) == 0) {
            if (sscanf(commande, "mkdir %s", param1) == 1) {
                creer_fichier(param1, TYPE_REPERTOIRE);
            } else {
                erreur("Usage: mkdir <nom>");
            }

        } else if (strncmp(commande, "touch ", 6) == 0) {
            if (sscanf(commande, "touch %s", param1) == 1 && strcmp(param1, ".") != 0 && strcmp(param1, "..") != 0) {
                creer_fichier(param1, TYPE_FICHIER);
            } else {
                erreur("Usage: touch <nom>");
            }

        } else if (strncmp(commande, "rm ", 3) == 0) {
            if (sscanf(commande, "rm %s", param1) == 1) {
                supprimer_fichier(param1);
            } else {
                erreur("Usage: rm <nom>");
            }

        } else if (strncmp(commande, "cp ", 3) == 0) {
            if (sscanf(commande, "cp %s %s", param1, param2) == 2) {
                copier_fichier(param1, param2);
            } else {
                erreur("Usage: cp <source> <destination>");
            }

        }else if (strncmp(commande, "chmod ", 6) == 0){
    		char param1[256], param2[256];

    	if (sscanf(commande, "chmod %255s %255s", param1, param2) == 2) {
        	int inode_id = trouver_inode_par_nom(inode_courant, param1);
        	if (inode_id == -1) {
            		erreur("Fichier non trouvé");
        	} else {
                int nouveaux_droits ;

                //convertir les droits en numéro pour pouvoir les modifier (ex rwx => 777)
                nouveaux_droits = convertir_droits_char(param2);

                	if (modifier_droits(inode_id, nouveaux_droits) == 0) {
                    	printf("Droits du fichier '%s' modifiés avec succès.\n", param1);
                    }else{
                        printf("Erreur de modification des droits.\n");
                    }
            }

    } else {
        erreur("Usage : chmod <nom_fichier> <droits>");
    }

         }else if (strncmp(commande, "mv ", 3) == 0) {
            if (sscanf(commande, "mv %s %s", param1, param2) == 2) {
                deplacer_fichier(param1, param2);
            } else {
                erreur("Usage: mv <source> <destination>");
            }

        }
 else if (strncmp(commande, "lns ", 4) == 0) {
            if (sscanf(commande, "lns %s %s", param1, param2) == 2) {
                creer_lien_symbolique(param1, param2);
            } else {
                erreur("Usage: lns <source> <destination>");
            }

        }else if (strncmp(commande, "ls -i", 5) == 0) {
            char param1[MAX_NOM_FICHIER];  // Buffer pour le nom du fichier ou répertoire

            // Récupérer le nom du fichier ou répertoire dans la commande
            if (sscanf(commande, "ls -i %s", param1) == 1) {
                // Rechercher l'inode correspondant au fichier ou répertoire
                Inode *inode = trouver_ind(param1);

                if (inode != NULL) {
                    // Afficher l'inode trouvé en utilisant la fonction afficher_inode
                    afficher_inode(inode);
                } else {
                    printf("Fichier ou répertoire '%s' non trouvé.\n", param1);
                }
            } else {
                printf("Usage: ls -i <fichier_ou_répertoire>\n");
            }
        }

        else if (strncmp(commande, "ln ", 3) == 0) {
            if (sscanf(commande, "ln %s %s", param1, param2) == 2) {
                creer_lien(param1, param2);
            } else {
                erreur("Usage: ln <source> <destination>");
            }

        } else if (strncmp(commande, "cat ", 4) == 0) {
            char fichier[MAX_NOM_FICHIER], extra[10];
            if (sscanf(commande, "cat %s %s", fichier, extra) == 1) {
                int inode_id = trouver_inode_par_nom(inode_courant, fichier);
                if (inode_id == -1) {
                    erreur("Fichier non trouvé");
                } else {
                    char buffer[1024];
                    int taille = inodes[inode_id].taille;
                    int offset = 0;

                    if (taille == 0) {
                        printf("(Fichier vide)\n");
                    } else {
                        while (offset < taille) {
                            int bytes_to_read = sizeof(buffer) - 1;
                            if (offset + bytes_to_read > taille) {
                                bytes_to_read = taille - offset;
                            }

                            int bytes_read = lire_fichier(inode_id, buffer, bytes_to_read, offset);
                            if (bytes_read <= 0) {
                                break;
                            }

                            buffer[bytes_read] = '\0';
                            printf("%s", buffer);
                            offset += bytes_read;
                        }
                        printf("\n");
                    }
                }
            } else {
                erreur("Usage: cat <nom_fichier>");
            }

        }
        else if (strncmp(commande, "save ", 5) == 0) {
	    if (sscanf(commande, "save %s", param1) == 1) {
		sauvegarder_etat(param1);
	    } else {
		erreur("Usage: save <nom_fichier>");
	    }
	}
	else if (strncmp(commande, "load ", 5) == 0) {
	    if (sscanf(commande, "load %s", param1) == 1) {
		restaurer_etat(param1);
	    } else {
		erreur("Usage: load <nom_fichier>");
	    }
	}
	else if (strncmp(commande, "defrag", 6) == 0) {
    // Vérifier si la commande est suivie d'un espace
    if (commande[6] == '\0' || commande[6] == ' ') {
        printf("Contenu du bitmap avant défragmentation :\n");
        afficher_bitmap(bitmap, NB_BLOCS);

        defragmenter();  // Appeler la fonction de défragmentation sans argument

        printf("Contenu du bitmap après défragmentation :\n");
        afficher_bitmap(bitmap, NB_BLOCS);
    } else {
        erreur("Usage: defrag");
    }
}
        else if (strncmp(commande, "write ", 6) == 0) {
            char fichier[MAX_NOM_FICHIER];
            if (sscanf(commande, "write %s", fichier) == 1) {
                int inode_id = trouver_inode_par_nom(inode_courant, fichier);
                if (inode_id == -1) {
                    erreur("Fichier non trouvé");
                } else {
                    // Vérifier que ce n'est pas un lien symbolique
                    if (inodes[inode_id].type == TYPE_LIEN_SYMBOLIQUE ) {

                        erreur("Impossible d'écrire dans un lien symbolique !");
                    } else {
                        if(inodes[inode_id].type == TYPE_REPERTOIRE) {
                            printf("Impossible d'écrire dans un repertoire !");
                        }else{
                        printf("Entrez le contenu à écrire (terminez par une ligne vide) :\n");

                        char buffer[1024] = {0};  // Initialise le buffer
                        char ligne[1024];

                        while (1) {
                            printf("> ");
                            if (fgets(ligne, sizeof(ligne), stdin) == NULL) {
                                break;
                            }

                            // Supprimer le retour à la ligne
                            ligne[strcspn(ligne, "\n")] = 0;

                            // Ligne vide termine la saisie
                            if (strlen(ligne) == 0) {
                                break;
                            }

                            // Vérifier si le buffer peut contenir plus de texte
                            if (strlen(buffer) + strlen(ligne) + 1 < sizeof(buffer)) {
                                strcat(buffer, ligne);
                                strcat(buffer, "\n");
                            } else {
                                erreur("Dépassement de la capacité du tampon !");
                                break;
                            }
                        }

                        // Écrire le contenu dans le fichier
                        int taille = strlen(buffer);
                        int resultat = ecrire_fichier(inode_id, buffer, taille, 0);

                        if (resultat >= 0) {
                            printf("Fichier écrit avec succès (%d octets).\n", taille);
                        } else {
                            erreur("Erreur lors de l'écriture du fichier");
                        }
                    }}
                }
            } else {
                erreur("Usage: write <nom_fichier>");
            }

        } else if (strcmp(commande, "quit") == 0) {
            break;

        } else {
            printf("Commande inconnue. Tapez 'aide' pour voir les commandes disponibles.\n");
        }

        // Sauvegarder les modifications
        sauvegarder_partition();
    }

    // Fermer la partition
    if (partition_file) {
        sauvegarder_partition();
        fclose(partition_file);
    }

    printf("Au revoir !\n");
    return 0;
}

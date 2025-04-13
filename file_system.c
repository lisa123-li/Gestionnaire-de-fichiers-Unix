#include "file_system.h"

/**
 * Pourcentage d'implication : 
 * GOBALOUKICHENIN Bala : 33%
 * DEHOUCHE Lisa : 33%
 * GABITA William : 33%
 * 
 * Implementation des fonctions utilisées
 */

// Définitions des variables globales
uint8_t bitmap[TAILLE_BITMAP];
Inode inodes[NB_INODES];
Superbloc superbloc;
FILE* partition_file = NULL;
int inode_courant = ID_INODE_RACINE;
EntreeRepertoire entrees[MAX_ENTREES_DIR];

/**
 * Valide un nom de fichier selon les règles du système
 * @param nom Le nom à valider
 * @return 1 si valide, 0 sinon
 */
int valider_nom_fichier(const char* nom) {
    if (nom == NULL) return 0;
    
    // Longueur du nom
    size_t longueur = strlen(nom);
    if (longueur == 0 || longueur > MAX_NOM_FICHIER) {
        return 0;
    }
    
    // Caractères interdits dans le nom
    const char* caracteres_interdits = "/\\:*?\"<>|";
    for (size_t i = 0; i < longueur; i++) {
        if (strchr(caracteres_interdits, nom[i]) != NULL) {
            return 0;
        }
    }
    
    return 1;
}
/**
 * Déplace le pointeur dans le fichier partition
 * @param f Le fichier
 * @param offset Décalage par rapport à 'base'
 * @param base Position de référence (SEEK_SET, SEEK_CUR, SEEK_END)
 */
void mySeek(FILE *f, long offset, int base) {
    if (fseek(f, offset, base) != 0) {
        perror("Erreur de déplacement du pointeur");
        exit(EXIT_FAILURE);
    }
}

/**
 * Trouve et réserve un bloc libre dans le bitmap
 * @return Le numéro du bloc trouvé, ou -1 si aucun bloc libre
 */
int trouver_bloc_libre() {
    for (int i = 0; i < NB_BLOCS; i++) {
        if (!(bitmap[i / BITS_PAR_OCTET] & (1 << (i % BITS_PAR_OCTET)))) {
            // Marquer le bloc comme utilisé
            bitmap[i / BITS_PAR_OCTET] |= (1 << (i % BITS_PAR_OCTET));
            superbloc.nb_blocs_libres--;
            return i;
        }
    }
    return -1; // Aucun bloc libre
}

/**
 * Libère un bloc et le marque comme libre dans le bitmap
 * @param num_bloc Le numéro du bloc à libérer
 */
void liberer_bloc(int num_bloc) {
    if (num_bloc < 0 || num_bloc >= NB_BLOCS) {
        erreur("Numéro de bloc invalide");
        return;
    }
    
    // Marquer le bloc comme libre
    bitmap[num_bloc / BITS_PAR_OCTET] &= ~(1 << (num_bloc % BITS_PAR_OCTET));
    superbloc.nb_blocs_libres++;
    
    // Effacer le contenu du bloc (optionnel)
    char buffer[TAILLE_BLOC] = {0};
    ecrire_bloc(num_bloc, buffer);
}

/**
 * Trouve un inode libre dans la table
 * @return L'index de l'inode libre, ou -1 si aucun disponible
 */
int trouver_inode_libre() {
    for (int i = 0; i < NB_INODES; i++) {
        if (inodes[i].taille == 0 && inodes[i].nb_liens == 0) {
            superbloc.nb_inodes_libres--;
            return i;
        }
    }
    return -1; // Aucun inode libre
}

/**
 * Libère un inode en réinitialisant sa structure
 * @param num_inode Le numéro de l'inode à libérer
 */
void liberer_inode(int num_inode) {
    if (num_inode < 0 || num_inode >= NB_INODES) {
        erreur("Numéro d'inode invalide");
        return;
    }

    
    memset(&inodes[num_inode], 0, sizeof(Inode));
    superbloc.nb_inodes_libres++;
}

/**
 * Écrit des données dans un bloc de la partition
 * @param num_bloc Le numéro du bloc à écrire
 * @param donnees Les données à écrire (doivent faire TAILLE_BLOC octets)
 */
void ecrire_bloc(int num_bloc, void* donnees) {
    if (num_bloc < 0 || num_bloc >= NB_BLOCS) {
        erreur("Numéro de bloc invalide");
        return;
    }
    long offset = TAILLE_BLOC * num_bloc;
    mySeek(partition_file, offset, SEEK_SET);

    if (fwrite(donnees, TAILLE_BLOC, 1, partition_file) != 1) {
        erreur("Erreur d'écriture du bloc");
    }

    // Mettre à jour la date de dernière modification du système
    superbloc.derniere_modification = time(NULL);
}



/**
 * Lit des données depuis un bloc de la partition
 * @param num_bloc Le numéro du bloc à lire
 * @param donnees Buffer pour stocker les données lues
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
 int lire_bloc(int num_bloc, void* donnees) {
    if (num_bloc < 0 || num_bloc >= NB_BLOCS) {
        erreur("Numéro de bloc invalide");
        return -1;
    }

    long offset = TAILLE_BLOC * num_bloc;
    fseek(partition_file, offset, SEEK_SET);
    
    if (fread(donnees, TAILLE_BLOC, 1, partition_file) != 1) {
        erreur("Erreur de lecture du bloc");
        return -1;
    }

    return 0;  // Succès
}


/**
 * Affiche un message d'erreur sur stderr
 * @param message Le message d'erreur à afficher
 */
void erreur(const char* message) {
    fprintf(stderr, "Erreur: %s\n", message);
}

/**
 * Crée un nouveau fichier ou répertoire
 * @param nom Le nom du nouveau fichier
 * @param type TYPE_FICHIER ou TYPE_REPERTOIRE
 * @return L'identifiant de l'inode créé, ou -1 en cas d'erreur
 */
int creer_fichier(const char* nom, int type) {
    // Vérifier la longueur du nom
    if (strlen(nom) > MAX_NOM_FICHIER) {
        erreur("Nom de fichier trop long");
        return -1;
    }
    
    // Vérifier si un fichier du même nom existe déjà
    if (trouver_inode_par_nom(inode_courant, nom) != -1) {
        erreur("Un fichier avec ce nom existe déjà");
        return -1;
    }
    
    // Trouver un inode libre
    int inode_id = trouver_inode_libre();
    if (inode_id == -1) {
        erreur("Aucun inode libre");
        return -1;
    }
    
    // Initialiser l'inode
    Inode* new_inode = &inodes[inode_id];
    memset(new_inode, 0, sizeof(Inode));
    
    new_inode->type = type;
    new_inode->date_creation = time(NULL);
    new_inode->date_modification = time(NULL);
    new_inode->date_acces = time(NULL);
    new_inode->nb_liens = 1;
    new_inode->proprietaire = getuid();
    new_inode->groupe = getgid();
    
    // Définir les droits par défaut
    if (type == TYPE_REPERTOIRE) {
        new_inode->droits = 0755;  // rwxr-xr-x
        new_inode->taille = TAILLE_BLOC; // Taille initiale d'un répertoire
        
        // Allouer un bloc pour le répertoire
        int bloc = trouver_bloc_libre();
        if (bloc == -1) {
            liberer_inode(inode_id);
            erreur("Aucun bloc libre");
            return -1;
        }
        new_inode->blocs_directs[0] = bloc;
        
        // Initialiser le contenu du répertoire
        EntreeRepertoire entrees[MAX_ENTREES_DIR] = {0};
        
        // Ajouter les entrées "." et ".."
        strcpy(entrees[0].nom, ".");
        entrees[0].inode = inode_id;
        
        strcpy(entrees[1].nom, "..");
        entrees[1].inode = inode_courant;
        
        // Écrire les entrées dans le bloc
        ecrire_bloc(bloc, entrees);
    } else {
        new_inode->droits = 0644;  // rw-r--r--
        new_inode->taille = 0;     // Taille initiale d'un fichier
    }
    
    // Copier le nom
    strncpy(new_inode->nom, nom, MAX_NOM_FICHIER);
    
    // Ajouter l'entrée au répertoire courant
    if (ajouter_entree_repertoire(inode_courant, nom, inode_id) == -1) {
        // Libérer les ressources en cas d'échec
        if (type == TYPE_REPERTOIRE) {
            liberer_bloc(new_inode->blocs_directs[0]);
        }
        liberer_inode(inode_id);
        return -1;
    }
    
    // Mettre à jour la dernière modification du répertoire parent
    inodes[inode_courant].date_modification = time(NULL);
    
    return inode_id;
}

/**
 * Ajoute une entrée dans un répertoire
 * @param inode_dir L'inode du répertoire parent
 * @param nom Le nom de la nouvelle entrée
 * @param inode L'inode à associer
 * @return 0 en cas de succès, -1 sinon
 */
int ajouter_entree_repertoire(int inode_dir, const char* nom, int inode) {
    // Vérifier que l'inode du répertoire est valide
    if (inode_dir < 0 || inode_dir >= NB_INODES || inodes[inode_dir].type != TYPE_REPERTOIRE) {
        erreur("L'inode n'est pas un répertoire ou l'inode est invalide");
        return -1;
    }

    // Vérifier que le nom n'est pas trop long
    if (strlen(nom) > MAX_NOM_FICHIER) {
        erreur("Nom de fichier trop long");
        return -1;
    }

    // Vérifier que l'inode à ajouter est valide
    if (inode < 0 || inode >= NB_INODES) {
        erreur("Numéro d'inode invalide");
        return -1;
    }

    // Vérifier si un bloc existe déjà pour le répertoire
    if (inodes[inode_dir].blocs_directs[0] == 0) {
        // Allouer un premier bloc uniquement pour les répertoires
        if (inodes[inode_dir].type == TYPE_REPERTOIRE) {
            int nouveau_bloc = trouver_bloc_libre();
            if (nouveau_bloc == -1) {
                erreur("Impossible d'allouer un bloc pour le répertoire");
                return -1;
            }
            inodes[inode_dir].blocs_directs[0] = nouveau_bloc;
        } else {
            // Pour les fichiers (y compris les liens), ne pas allouer de bloc
            erreur("Impossible d'ajouter une entrée à un fichier sans bloc");
            return -1;
        }
    }

    // Lire le contenu du répertoire
    EntreeRepertoire entrees[MAX_ENTREES_DIR];
    lire_bloc(inodes[inode_dir].blocs_directs[0], entrees);

    // Vérifier si le nom existe déjà
    for (int i = 0; i < MAX_ENTREES_DIR; i++) {
        if (strcmp(entrees[i].nom, nom) == 0) {
            erreur("Une entrée avec ce nom existe déjà");
            return -1;
        }
    }

    // Trouver une entrée libre
    int index = -1;
    for (int i = 0; i < MAX_ENTREES_DIR; i++) {
        if (entrees[i].nom[0] == '\0') {
            index = i;
            break;
        }
    }

    if (index == -1) {
        erreur("Répertoire plein");
        return -1;
    }

    // Ajouter la nouvelle entrée
    strncpy(entrees[index].nom, nom, MAX_NOM_FICHIER);
    entrees[index].nom[MAX_NOM_FICHIER] = '\0';  // Assurer la terminaison
    entrees[index].inode = inode;

    // Écrire les modifications uniquement pour les répertoires
    if (inodes[inode_dir].type == TYPE_REPERTOIRE) {
        ecrire_bloc(inodes[inode_dir].blocs_directs[0], entrees);
    }

    // Mise à jour de l'inode du répertoire
    if (inodes[inode_dir].type == TYPE_REPERTOIRE) {
        //inodes[inode_dir].taille = (index + 1) * sizeof(EntreeRepertoire);
        inodes[inode_dir].date_modification = time(NULL);
    }

    return 0;
}

/**
 * Cherche un inode par son nom dans un répertoire
 * @param inode_dir L'inode du répertoire où chercher
 * @param nom Le nom du fichier/dossier à trouver
 * @return L'identifiant de l'inode trouvé ou -1 si non trouvé/erreur
 */
int trouver_inode_par_nom(int inode_dir, const char* nom) {
    // Vérification de la validité de l'inode
    if (inode_dir < 0 || inode_dir >= NB_INODES) {
        erreur("Inode invalide");
        return -1;
    }
    
    // Vérification que l'inode est bien un répertoire
    if (inodes[inode_dir].type != TYPE_REPERTOIRE) {
        erreur("L'inode n'est pas un répertoire");
        return -1;
    }
    
    // Lecture du contenu du répertoire depuis le premier bloc direct
    EntreeRepertoire entrees[MAX_ENTREES_DIR];
    lire_bloc(inodes[inode_dir].blocs_directs[0], entrees);
    
    // Parcours des entrées du répertoire
    for (int i = 0; i < MAX_ENTREES_DIR; i++) {
        // Comparaison des noms et vérification que l'entrée est utilisée
        if (strcmp(entrees[i].nom, nom) == 0 && entrees[i].inode != 0) {
            return entrees[i].inode; // Retourne l'inode si trouvé
        }
    }
    
    return -1; // Retourne -1 si non trouvé
}

/**
 * Supprime une entrée dans un répertoire
 * @param inode_dir L'inode du répertoire
 * @param nom Le nom de l'entrée à supprimer
 * @return 0 si succès, -1 si erreur
 */
int supprimer_entree_repertoire(int inode_dir, const char* nom) {
    // Vérification que l'inode est bien un répertoire
    if (inodes[inode_dir].type != TYPE_REPERTOIRE) {
        erreur("L'inode n'est pas un répertoire");
        return -1;
    }
    
    // Lecture du contenu du répertoire
    EntreeRepertoire entrees[MAX_ENTREES_DIR];
    lire_bloc(inodes[inode_dir].blocs_directs[0], entrees);
    
    // Recherche de l'entrée à supprimer
    for (int i = 0; i < MAX_ENTREES_DIR; i++) {
        if (strcmp(entrees[i].nom, nom) == 0) {
            // Effacement de l'entrée
            memset(&entrees[i], 0, sizeof(EntreeRepertoire));
            
            // Réécriture du bloc modifié
            ecrire_bloc(inodes[inode_dir].blocs_directs[0], entrees);
            
            // Mise à jour de la date de modification
            inodes[inode_dir].date_modification = time(NULL);
            
            return 0; // Succès
        }
    }
    
    erreur("Entrée non trouvée");
    return -1; // Erreur si entrée non trouvée
}

/**
 * Supprime un fichier ou un répertoire vide
 * @param nom Le nom du fichier/dossier à supprimer
 * @return 0 si succès, -1 si erreur
 */
int supprimer_fichier(const char* nom) {
    // Validation du nom de fichier
    if (!valider_nom_fichier(nom)) {
        erreur("Nom de fichier invalide");
        return -1;
    }
    
    // Recherche de l'inode correspondant au nom
    int inode_id = trouver_inode_par_nom(inode_courant, nom);
    if (inode_id == -1) {
        erreur("Fichier non trouvé");
        return -1;
    }
    
    // Vérification des droits d'écriture
    if (!verifier_droits(inode_id, DROIT_ECRITURE)) {
        erreur("Permission refusée");
        return -1;
    }
    
    Inode* inode = &inodes[inode_id];
    
    // Traitement spécial pour les répertoires
    if (inode->type == TYPE_REPERTOIRE) {
        EntreeRepertoire entrees[MAX_ENTREES_DIR];
        lire_bloc(inode->blocs_directs[0], entrees);
        
        // Comptage des entrées non vides (hors . et ..)
        int nb_entrees = 0;
        for (int i = 0; i < MAX_ENTREES_DIR; i++) {
            if (entrees[i].nom[0] != '\0' && 
                strcmp(entrees[i].nom, ".") != 0 && 
                strcmp(entrees[i].nom, "..") != 0) {
                nb_entrees++;
            }
        }
        
        // Vérification que le répertoire est vide
        if (nb_entrees > 0) {
            erreur("Le répertoire n'est pas vide");
            return -1;
        }
    }
    
    // Décrémentation du compteur de liens
    inode->nb_liens--;
    
    // Si c'était le dernier lien, libération des ressources
    if (inode->nb_liens == 0) {
        // Libération des blocs directs
        int nb_blocs = (inode->taille + TAILLE_BLOC - 1) / TAILLE_BLOC;
        for (int i = 0; i < nb_blocs && i < 10; i++) {
            if (inode->blocs_directs[i] != 0) {
                liberer_bloc(inode->blocs_directs[i]);
                inode->blocs_directs[i] = 0;
            }
        }
        
        // Libération des blocs indirects si existants
        if (inode->bloc_indirect != 0) {
            int blocs_indirects[TAILLE_BLOC / sizeof(int)];
            lire_bloc(inode->bloc_indirect, blocs_indirects);
            
            for (int i = 0; i < TAILLE_BLOC / sizeof(int); i++) {
                if (blocs_indirects[i] != 0) {
                    liberer_bloc(blocs_indirects[i]);
                }
            }
            
            liberer_bloc(inode->bloc_indirect);
            inode->bloc_indirect = 0;
        }
        
        // Libération de l'inode
        liberer_inode(inode_id);
    }
    
    // Suppression de l'entrée dans le répertoire parent
    int result = supprimer_entree_repertoire(inode_courant, nom);
    
    return result;
}

/**
 * Lit le contenu d'un fichier
 * @param inode_id L'inode du fichier à lire
 * @param buffer Le buffer où stocker les données lues
 * @param taille Le nombre d'octets à lire
 * @param offset La position de départ dans le fichier
 * @return Le nombre d'octets lus ou -1 en cas d'erreur
 */
int lire_fichier(int inode_id, void* buffer, int taille, int offset) {
    // Vérification de l'identifiant d'inode
    if (inode_id < 0 || inode_id >= NB_INODES) {
        erreur("Numéro d'inode invalide");
        return -1;
    }
    
    Inode* inode = &inodes[inode_id];
    
    // Gestion des liens symboliques
    if (inode->type == TYPE_LIEN_SYMBOLIQUE) {
        // Lecture du chemin cible
        char chemin_source[TAILLE_BLOC];
        lire_bloc(inode->blocs_directs[0], chemin_source);
        
        // Recherche de l'inode cible
        int inode_source = trouver_inode_par_nom(inode_courant, chemin_source);
        
        if (inode_source == -1) {
            erreur("Fichier source du lien symbolique non trouvé");
            return -1;
        }
        
        // Lecture récursive du fichier cible
        return lire_fichier(inode_source, buffer, taille, 0);
    }
    
    // Vérification des droits de lecture
    if (!verifier_droits(inode_id, DROIT_LECTURE)) {
        erreur("Permission refusée");
        return -1;
    }
    
    // Vérification que c'est bien un fichier
    if (inode->type == TYPE_REPERTOIRE)  {
        erreur("L'inode n'est pas un fichier");
        return -1;
    }
    
    // Vérification de l'offset
    if (offset < 0 || offset >= inode->taille) {
        erreur("Offset invalide");
        return -1;
    }
    
    // Ajustement de la taille à lire si nécessaire
    if (offset + taille > inode->taille) {
        taille = inode->taille - offset;
    }
    
    // Lecture des données
    int bytes_read = 0;
    char block_buffer[TAILLE_BLOC];
    
    while (bytes_read < taille) {
        // Calcul du bloc et de l'offset dans le bloc
        int bloc_index = (offset + bytes_read) / TAILLE_BLOC;
        int bloc_offset = (offset + bytes_read) % TAILLE_BLOC;
        int bytes_to_read = TAILLE_BLOC - bloc_offset;
        
        if (bytes_to_read > taille - bytes_read) {
            bytes_to_read = taille - bytes_read;
        }
        
        // Détermination du numéro de bloc
        int num_bloc = -1;
        if (bloc_index < 10) {
            num_bloc = inode->blocs_directs[bloc_index];
        } else if (inode->bloc_indirect != 0) {
            int blocs_indirects[TAILLE_BLOC / sizeof(int)];
            lire_bloc(inode->bloc_indirect, blocs_indirects);
            num_bloc = blocs_indirects[bloc_index - 10];
        }
        
        if (num_bloc == 0 || num_bloc == -1) {
            // Bloc non alloué, remplissage avec des zéros
            memset((char*)buffer + bytes_read, 0, bytes_to_read);
        } else {
            // Lecture effective du bloc
            lire_bloc(num_bloc, block_buffer);
            memcpy((char*)buffer + bytes_read, block_buffer + bloc_offset, bytes_to_read);
        }
        
        bytes_read += bytes_to_read;
    }
    
    // Mise à jour de la date d'accès
    inode->date_acces = time(NULL);
    
    return bytes_read;
}

/**
 * Écrit dans un fichier
 * @param inode_id L'inode du fichier à modifier
 * @param buffer Les données à écrire
 * @param taille Le nombre d'octets à écrire
 * @param offset La position de départ dans le fichier
 * @return Le nombre d'octets écrits ou -1 en cas d'erreur
 */
int ecrire_fichier(int inode_id, void* buffer, int taille, int offset) {
    // Vérification de l'identifiant d'inode
    if (inode_id < 0 || inode_id >= NB_INODES) {
        erreur("Numéro d'inode invalide");
        return -1;
    }
    
    Inode* inode = &inodes[inode_id];
    
    // Vérification des droits d'écriture
    if (!verifier_droits(inode_id, DROIT_ECRITURE)) {
        erreur("Permission refusée");
        return -1;
    }
    
    // Vérification que c'est bien un fichier
    if (inode->type != TYPE_FICHIER) {
        erreur("L'inode n'est pas un fichier");
        return -1;
    }
    
    // Vérification de l'offset
    if (offset < 0) {
        erreur("Offset invalide");
        return -1;
    }
    
    // Si on écrit au début d'un fichier non vide, libération des blocs existants
    if (inode->taille > 0 && offset == 0) {
        // Libération des blocs directs
        for (int i = 0; i < 10; i++) {
            if (inode->blocs_directs[i] != 0) {
                liberer_bloc(inode->blocs_directs[i]);
                inode->blocs_directs[i] = 0;
            }
        }
        
        // Libération des blocs indirects si existants
        if (inode->bloc_indirect != 0) {
            int blocs_indirects[TAILLE_BLOC / sizeof(int)];
            lire_bloc(inode->bloc_indirect, blocs_indirects);
            for (int i = 0; i < TAILLE_BLOC / sizeof(int); i++) {
                if (blocs_indirects[i] != 0) {
                    liberer_bloc(blocs_indirects[i]);
                }
            }
            liberer_bloc(inode->bloc_indirect);
            inode->bloc_indirect = 0;
        }
        
        // Réinitialisation de la taille
        inode->taille = 0;
    }
    
    // Écriture des données
    int bytes_written = 0;
    char block_buffer[TAILLE_BLOC];

    while (bytes_written < taille) {
        // Calcul du bloc et de l'offset dans le bloc
        int bloc_index = (offset + bytes_written) / TAILLE_BLOC;
        int bloc_offset = (offset + bytes_written) % TAILLE_BLOC;
        int bytes_to_write = TAILLE_BLOC - bloc_offset;
        
        if (bytes_to_write > taille - bytes_written) {
            bytes_to_write = taille - bytes_written;
        }

        // Gestion de l'allocation des blocs
        int num_bloc = -1;
        if (bloc_index < 10) {
            if (inode->blocs_directs[bloc_index] == 0) {
                inode->blocs_directs[bloc_index] = trouver_bloc_libre();
            }
            num_bloc = inode->blocs_directs[bloc_index];
        } else {
            if (inode->bloc_indirect == 0) {
                inode->bloc_indirect = trouver_bloc_libre();
                int zero_init[TAILLE_BLOC / sizeof(int)] = {0};
                ecrire_bloc(inode->bloc_indirect, zero_init);
            }
            int blocs_indirects[TAILLE_BLOC / sizeof(int)];
            lire_bloc(inode->bloc_indirect, blocs_indirects);
            if (blocs_indirects[bloc_index - 10] == 0) {
                blocs_indirects[bloc_index - 10] = trouver_bloc_libre();
                ecrire_bloc(inode->bloc_indirect, blocs_indirects);
            }
            num_bloc = blocs_indirects[bloc_index - 10];
        }
        
        if (num_bloc == -1) {
            erreur("Aucun bloc libre");
            return -1;
        }

        // Lecture-modification-écriture du bloc
        lire_bloc(num_bloc, block_buffer);
        memcpy(block_buffer + bloc_offset, (char*)buffer + bytes_written, bytes_to_write);
        ecrire_bloc(num_bloc, block_buffer);

        bytes_written += bytes_to_write;
    }

    // Mise à jour de la taille si nécessaire
    if (offset + taille > inode->taille) {
        inode->taille = offset + taille;
    }
    
    // Mise à jour des dates
    inode->date_modification = time(NULL);
    inode->date_acces = time(NULL);

    return bytes_written;
}

/**
 * Vérifie les droits d'accès
 * @param num_inode L'inode à vérifier
 * @param droits_requis Les droits nécessaires (DROIT_LECTURE/ECRITURE/EXECUTION)
 * @return 1 si les droits sont accordés, 0 sinon
 */
int verifier_droits(int num_inode, int droits_requis) {
    // Vérification de l'identifiant d'inode
    if (num_inode < 0 || num_inode >= NB_INODES) {
        return 0;
    }
    
    Inode* inode = &inodes[num_inode];
    
    // L'utilisateur root a tous les droits
    if (getuid() == 0) {
        return 1;
    }
    
    // Détermination du masque de droits selon l'utilisateur
    int mask = 0;
    
    if (inode->proprietaire == getuid()) {
        // Droits du propriétaire (bits 6-8)
        mask = (inode->droits >> 6) & 0x7;
    } else if (inode->groupe == getgid()) {
        // Droits du groupe (bits 3-5)
        mask = (inode->droits >> 3) & 0x7;
    } else {
        // Droits des autres (bits 0-2)
        mask = inode->droits & 0x7;
    }
   
    // Vérification que le masque contient tous les droits requis
    return (droits_requis & mask) == droits_requis;
}

/**
 * Crée un lien physique entre deux fichiers
 * @param source Le fichier source
 * @param nom_lien Le nom du lien à créer
 * @return 0 si succès, -1 si erreur
 */
int creer_lien(const char* source, const char* nom_lien) {
    // Recherche de l'inode source
    int inode_source = trouver_inode_par_nom(inode_courant, source);
    if (inode_source == -1) {
        erreur("Fichier source non trouvé");
        return -1;
    }

    // Vérification que la source n'est pas un répertoire
    if (inodes[inode_source].type == TYPE_REPERTOIRE) {
        erreur("Impossible de créer un lien physique vers un répertoire");
        return -1;
    }

    // Vérification que le lien n'existe pas déjà
    int inode_lien = trouver_inode_par_nom(inode_courant, nom_lien);
    if (inode_lien != -1) {
        erreur("Un fichier avec ce nom de lien existe déjà");
        return -1;
    }

    // Allocation d'un nouvel inode
    int nouvel_inode = trouver_inode_libre();
    if (nouvel_inode == -1) {
        erreur("Impossible de créer un nouvel inode pour le lien physique");
        return -1;
    }

    // Copie des métadonnées de l'inode source
    memcpy(&inodes[nouvel_inode], &inodes[inode_source], sizeof(Inode));
    
    // Modification des attributs spécifiques
    inodes[nouvel_inode].type = TYPE_LIEN_PHYSIQUE;
    strncpy(inodes[nouvel_inode].nom, nom_lien, MAX_NOM_FICHIER);
    inodes[nouvel_inode].date_creation = time(NULL);
    inodes[nouvel_inode].date_modification = time(NULL);

    // Ajout de l'entrée dans le répertoire
    if (ajouter_entree_repertoire(inode_courant, nom_lien, nouvel_inode) == -1) {
        liberer_inode(nouvel_inode);
        return -1;
    }

    // Incrémentation du compteur de liens de la source
    inodes[inode_source].nb_liens++;

    return 0;
}

/**
 * Crée un lien symbolique
 * @param source Le chemin cible du lien
 * @param destination Le nom du lien symbolique
 * @return L'identifiant de l'inode créé ou -1 si erreur
 */
int creer_lien_symbolique(const char* source, const char* destination) {
    // Vérifier le nom du lien symbolique
    if (!valider_nom_fichier(destination)) {
        erreur("Nom de lien symbolique invalide");
        return -1;
    }

    // Vérifier s'il existe déjà un fichier du même nom
    if (trouver_inode_par_nom(inode_courant, destination) != -1) {
        erreur("Un fichier avec ce nom existe déjà");
        return -1;
    }

    // Trouver un inode libre pour le lien symbolique
    int inode_lien = trouver_inode_libre();
    if (inode_lien == -1) {
        erreur("Aucun inode libre pour le lien symbolique");
        return -1;
    }

    Inode* inode = &inodes[inode_lien];
    memset(inode, 0, sizeof(Inode));
    
    inode->type = TYPE_LIEN_SYMBOLIQUE;
    inode->date_creation = time(NULL);
    inode->date_modification = time(NULL);
    inode->date_acces = time(NULL);
    inode->nb_liens = 1;
    inode->proprietaire = getuid();
    inode->groupe = getgid();
    inode->droits = 0777;  // Accès total au lien

    // Stocker le chemin source comme contenu du lien
    size_t longueur = strlen(source);
    if (longueur >= sizeof(inode->blocs_directs)) {
        erreur("Chemin source trop long pour le lien symbolique");
        liberer_inode(inode_lien);
        return -1;
    }

    // On stocke le chemin dans les blocs du fichier comme si c’était le contenu
    int bloc = trouver_bloc_libre();
    if (bloc == -1) {
        erreur("Aucun bloc libre pour le lien symbolique");
        liberer_inode(inode_lien);
        return -1;
    }

    inode->blocs_directs[0] = bloc;
    inode->taille = longueur + 1;

    char buffer[TAILLE_BLOC] = {0};
    strncpy(buffer, source, TAILLE_BLOC - 1);
    ecrire_bloc(bloc, buffer);

    // Nom du lien symbolique
    strncpy(inode->nom, destination, MAX_NOM_FICHIER);

    // Ajouter l'entrée dans le répertoire courant
    if (ajouter_entree_repertoire(inode_courant, destination, inode_lien) != 0) {
        liberer_bloc(bloc);
        liberer_inode(inode_lien);
        erreur("Échec de l'ajout du lien symbolique dans le répertoire");
        return -1;
    }

    return 0;  // Succès
}

/**
 * Affiche le contenu d'un répertoire
 * @param inode_dir L'inode du répertoire à afficher
 */
void afficher_repertoire(int inode_dir) {
    // Vérification de l'inode
    if (inode_dir < 0 || inode_dir >= NB_INODES) {
        erreur("Indice d'inode invalide !");
        return;
    }

    Inode* inode_repertoire = &inodes[inode_dir];

    // Vérification du type
    if (inode_repertoire->type != TYPE_REPERTOIRE) {
        erreur("L'inode n'est pas un répertoire");
        return;
    }

    // Lecture du bloc de répertoire
    int bloc_repertoire = inode_repertoire->blocs_directs[0];
    if (bloc_repertoire < 0) {
        erreur("Bloc de répertoire invalide !");
        return;
    }

    if (lire_bloc(bloc_repertoire, entrees) == -1) {
        erreur("Erreur lors de la lecture du répertoire !");
        return;
    }

    // Affichage de l'en-tête
    printf("Contenu du répertoire :\n");
    printf("%-20s %-10s %-10s %-10s %-20s\n", "Nom", "Type", "Taille", "Droits", "Modification");
    printf("----------------------------------------------------------------\n");

    // Parcours des entrées
    for (int i = 0; i < MAX_ENTREES_DIR; i++) {
        if (entrees[i].nom[0] != '\0') {  // Entrée valide
            int inode_id = entrees[i].inode;

            // Vérification de l'inode
            if (inode_id < 0 || inode_id >= NB_INODES) {
                erreur("ID d'inode invalide dans le répertoire !");
                continue;
            }

            Inode* inode = &inodes[inode_id];

            // Détermination du type
            char type_char = '-';
            if (inode->type == TYPE_FICHIER) {
                type_char = 'f';
            } else if(inode->type == TYPE_REPERTOIRE) {
                type_char = 'd';
            } else if (inode->type == TYPE_LIEN_SYMBOLIQUE) {
                type_char = 'l';
            }else if(inode->type == TYPE_LIEN_PHYSIQUE){
                type_char = 'p';
            }

            // Construction des permissions
            char droits[11] = { type_char, 
                (inode->droits & 0400) ? 'r' : '-',
                (inode->droits & 0200) ? 'w' : '-',
                (inode->droits & 0100) ? 'x' : '-',
                (inode->droits & 0040) ? 'r' : '-',
                (inode->droits & 0020) ? 'w' : '-',
                (inode->droits & 0010) ? 'x' : '-',
                (inode->droits & 0004) ? 'r' : '-',
                (inode->droits & 0002) ? 'w' : '-',
                (inode->droits & 0001) ? 'x' : '-',
                '\0' };

            // Formatage de la date
            char date_buf[20] = "Date inconnue";
            if (inode->date_modification > 0) {
                struct tm* tm_info = localtime(&inode->date_modification);
                if (tm_info) {
                    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M", tm_info);
                }
            }

            // Texte du type
            char type_str[20];
            if (inode->type == TYPE_REPERTOIRE) {
                strcpy(type_str, "Répertoire");
            } else if (inode->type == TYPE_LIEN_SYMBOLIQUE) {
                strcpy(type_str, "Lien symb.");
            } else if(inode->type == TYPE_LIEN_PHYSIQUE) {
                strcpy(type_str, "Lien ph.");
            } else{
                strcpy(type_str, "Fichier");
            }

            // Affichage des informations
            printf("%-20s %-10s %-10d %-10s %-20s\n", 
                   entrees[i].nom, type_str, inode->taille, droits, date_buf);
        }
    }
}

/**
 * Change le répertoire courant
 * @param chemin Le chemin du répertoire cible
 * @return 0 si succès, -1 si erreur
 */
int changer_repertoire(const char* chemin) {
    // Cas particulier pour remonter au parent
    if (strcmp(chemin, "..") == 0) {
        EntreeRepertoire entrees[MAX_ENTREES_DIR];
        lire_bloc(inodes[inode_courant].blocs_directs[0], entrees);
        
        // Recherche de l'entrée ".."
        for (int i = 0; i < MAX_ENTREES_DIR; i++) {
            if (strcmp(entrees[i].nom, "..") == 0) {
                inode_courant = entrees[i].inode;
                return 0;
            }
        }
        
        return -1;
    } 
    // Cas particulier pour le répertoire courant
    else if (strcmp(chemin, ".") == 0) {
        return 0;
    } 
    // Cas général
    else {
        // Recherche du répertoire
        int inode_id = trouver_inode_par_nom(inode_courant, chemin);
        if (inode_id == -1) {
            erreur("Répertoire non trouvé");
            return -1;
        }
        
        // Vérification du type
        if (inodes[inode_id].type != TYPE_REPERTOIRE) {
            erreur("Ce n'est pas un répertoire");
            return -1;
        }
        
        // Vérification des droits
        if (!verifier_droits(inode_id, DROIT_EXECUTION)) {
            erreur("Permission refusée");
            return -1;
        }
        
        // Changement de répertoire
        inode_courant = inode_id;
        
        // Mise à jour de la date d'accès
        inodes[inode_id].date_acces = time(NULL);
        
        return 0;
    }
}

/**
 * Copie un fichier
 * @param source Le fichier source
 * @param destination Le fichier destination
 * @return 0 si succès, -1 si erreur
 */
int copier_fichier(const char* source, const char* destination) {
    // Recherche de l'inode source
    int inode_source = trouver_inode_par_nom(inode_courant, source);
    if (inode_source == -1) {
        erreur("Fichier source non trouvé");
        return -1;
    }
    
    // Vérification des droits
    if (!verifier_droits(inode_source, DROIT_LECTURE)) {
        erreur("Permission refusée sur le fichier source");
        return -1;
    }
    
    // Vérification que la destination n'existe pas
    int inode_dest = trouver_inode_par_nom(inode_courant, destination);
    if (inode_dest != -1) {
        erreur("La destination existe déjà");
        return -1;
    }
    
    // Création du fichier destination
    inode_dest = creer_fichier(destination, TYPE_FICHIER);
    if (inode_dest == -1) {
        return -1;
    }
    
    // Copie des données par blocs
    char buffer[TAILLE_BLOC];
    int taille = inodes[inode_source].taille;
    int offset = 0;
    
    while (offset < taille) {
        int bytes_to_read = TAILLE_BLOC;
        if (offset + bytes_to_read > taille) {
            bytes_to_read = taille - offset;
        }
        
        // Lecture depuis la source
        if (lire_fichier(inode_source, buffer, bytes_to_read, offset) != bytes_to_read) {
            supprimer_fichier(destination);
            return -1;
        }
        
        // Écriture vers la destination
        if (ecrire_fichier(inode_dest, buffer, bytes_to_read, offset) != bytes_to_read) {
            supprimer_fichier(destination);
            return -1;
        }
        
        offset += bytes_to_read;
    }
    
    return 0;
}

/**
 * Déplace un fichier ou répertoire
 * @param source Le fichier/répertoire source
 * @param destination La nouvelle destination
 * @return 0 si succès, -1 si erreur
 */
int deplacer_fichier(const char* source, const char* destination) {
    // Recherche de l'inode source
    int inode_source = trouver_inode_par_nom(inode_courant, source);
    if (inode_source == -1) {
        erreur("Fichier source non trouvé");
        return -1;
    }
    
    // Vérification que la destination n'existe pas
    if (trouver_inode_par_nom(inode_courant, destination) != -1) {
        erreur("La destination existe déjà");
        return -1;
    }
    
    // Vérification des droits
    if (!verifier_droits(inode_source, DROIT_ECRITURE)) {
        erreur("Permission refusée");
        return -1;
    }
    
    // Ajout de la nouvelle entrée
    if (ajouter_entree_repertoire(inode_courant, destination, inode_source) == -1) {
        return -1;
    }
    
    // Suppression de l'ancienne entrée
    supprimer_entree_repertoire(inode_courant, source);
    
    // Mise à jour de la date de modification
    inodes[inode_courant].date_modification = time(NULL);
    
    return 0;
}

/**
 * Modifie les droits d'un fichier
 * @param inode_id L'inode à modifier
 * @param nouveaux_droits Les nouveaux droits (en octal)
 * @return 0 si succès, -1 si erreur
 */
int modifier_droits(int inode_id, int nouveaux_droits) {
    if (inode_id < 0 || inode_id >= NB_INODES) {
        printf("Erreur: Numéro d'inode invalide.\n");
        return -1;
    }

    Inode* inode = &inodes[inode_id];

    inode->droits = nouveaux_droits; 
    inode->date_modification = time(NULL);

    printf("Les droits du fichier '%s' ont été modifiés avec succès.\n", inode->nom);
    return 0;
}

/**
 * Convertit une chaîne de droits (rwxrwxrwx) en valeur octale
 * @param droits La chaîne de droits
 * @return La valeur octale ou -1 si format invalide
 */
int convertir_droits_char(const char* droits) {
    int result = 0;
    
    // Vérifier que les droits sont bien sous la forme de 9 caractères
    if (strlen(droits) == 9) {
        // Les trois premiers caractères sont pour le propriétaire
        if (droits[0] == 'r') result |= 0400; 
        if (droits[1] == 'w') result |= 0200; 
        if (droits[2] == 'x') result |= 0100;

        // Les trois suivants sont pour le groupe
        if (droits[3] == 'r') result |= 0040;
        if (droits[4] == 'w') result |= 0020; 
        if (droits[5] == 'x') result |= 0010; 

        // Les trois derniers sont pour les autres
        if (droits[6] == 'r') result |= 0004;
        if (droits[7] == 'w') result |= 0002; 
        if (droits[8] == 'x') result |= 0001;

        return result;
    }
    
    return -1; // Format invalide
}


/**
 * Sauvegarde l'état actuel de la partition dans un fichier.
 * 
 * @param fichier_sauvegarde Le chemin du fichier où sauvegarder l'état.
 * @return void
 */
void sauvegarder_etat(const char* fichier_sauvegarde) {
    FILE* f = fopen(fichier_sauvegarde, "wb");
    if (!f) {
        erreur("Impossible d'ouvrir le fichier de sauvegarde");
        return;
    }

    fwrite(&superbloc, sizeof(Superbloc), 1, f);
    fwrite(bitmap, sizeof(bitmap), 1, f);
    fwrite(inodes, sizeof(inodes), 1, f);

    void* buffer = malloc(TAILLE_BLOC);
    if (!buffer) {
        fclose(f);
        erreur("Mémoire insuffisante");
        return;
    }

    for (int i = 0; i < NB_BLOCS; i++) {
        fseek(partition_file, i * TAILLE_BLOC, SEEK_SET);
        fread(buffer, TAILLE_BLOC, 1, partition_file);
        fwrite(buffer, TAILLE_BLOC, 1, f);
    }

    free(buffer);
    fclose(f);
    printf("Partition sauvegardée dans '%s'\n", fichier_sauvegarde);
}

/**
 * Restaure l'état de la partition à partir d'un fichier de sauvegarde.
 * 
 * @param fichier_sauvegarde Le chemin du fichier de sauvegarde.
 * @return void
 */
void restaurer_etat(const char* fichier_sauvegarde) {
    FILE* f = fopen(fichier_sauvegarde, "rb");
    if (!f) {
        erreur("Impossible d'ouvrir le fichier de restauration");
        return;
    }

    fread(&superbloc, sizeof(Superbloc), 1, f);
    fread(bitmap, sizeof(bitmap), 1, f);
    fread(inodes, sizeof(inodes), 1, f);

    void* buffer = malloc(TAILLE_BLOC);
    if (!buffer) {
        fclose(f);
        erreur("Mémoire insuffisante");
        return;
    }

    for (int i = 0; i < NB_BLOCS; i++) {
        fread(buffer, TAILLE_BLOC, 1, f);
        fseek(partition_file, i * TAILLE_BLOC, SEEK_SET);
        fwrite(buffer, TAILLE_BLOC, 1, partition_file);
    }

    free(buffer);
    fclose(f);
    printf("Partition restaurée depuis '%s'\n", fichier_sauvegarde);
}

/**
 * Recherche un inode par son nom dans la table des inodes
 * @param nom Nom du fichier/répertoire à rechercher
 * @return Pointeur vers l'inode trouvé, ou NULL si non trouvé
 */
Inode* trouver_ind(char* nom) {
    for (int i = 0; i < NB_INODES; i++) {
        // Vérification si le nom de l'inode correspond à celui recherché
        if (strcmp(inodes[i].nom, nom) == 0) {
            return &inodes[i];  // Retourne l'inode correspondant
        }
    }
    // Si le fichier ou répertoire n'est pas trouvé, afficher un message d'erreur.
    printf("Inode non trouvé pour %s\n", nom);
    return NULL;
}

/**
 * Affiche les informations détaillées d'un inode (métadonnées d'un fichier/répertoire)
 * @param inode Pointeur vers la structure Inode à afficher
 */
void afficher_inode(const Inode *inode) {
    // Affichage des informations de l'inode
    printf("Nom: %s\n", inode->nom);
    printf("Taille: %d octets\n", inode->taille);
    
    // Affichage du type (fichier, répertoire, lien)
    const char *type_str;
    switch (inode->type) {
        case TYPE_FICHIER:
            type_str = "Fichier";
            break;
        case TYPE_REPERTOIRE:
            type_str = "Répertoire";
            break;
        case TYPE_LIEN_SYMBOLIQUE:
            type_str = "Lien symbolique";
            break;
        case TYPE_LIEN_PHYSIQUE:
            type_str = "Lien physique";
            break;
        default:
            type_str = "Type inconnu";
            break;
    }
    printf("Type: %s\n", type_str);
    
    // Affichage du propriétaire et du groupe
    printf("Propriétaire (UID): %d\n", inode->proprietaire);
    printf("Groupe (GID): %d\n", inode->groupe);
    
    // Affichage des droits d'accès (format Unix)
    printf("Droits: %o\n", inode->droits);
    
    // Affichage des dates de création, modification, et accès
    char date_creation[20], date_modification[20], date_acces[20];
    strftime(date_creation, sizeof(date_creation), "%Y-%m-%d %H:%M:%S", localtime(&inode->date_creation));
    strftime(date_modification, sizeof(date_modification), "%Y-%m-%d %H:%M:%S", localtime(&inode->date_modification));
    strftime(date_acces, sizeof(date_acces), "%Y-%m-%d %H:%M:%S", localtime(&inode->date_acces));
    
    printf("Date de création: %s\n", date_creation);
    printf("Date de dernière modification: %s\n", date_modification);
    printf("Date de dernier accès: %s\n", date_acces);
    
    // Affichage du nombre de liens
    printf("Nombre de liens: %d\n", inode->nb_liens);
    
    // Créer un buffer suffisamment grand pour le contenu complet
    // Calculer d'abord combien d'espace nous avons besoin
    int max_taille_fichier = TAILLE_BLOC * 10;  // Pour blocs directs
    if (inode->bloc_indirect != 0) {
        max_taille_fichier += TAILLE_BLOC * (TAILLE_BLOC / sizeof(int));  // Pour blocs indirects
    }
    
    // Allouer dynamiquement la mémoire nécessaire
    unsigned char *buffer_complet = NULL;
    if (inode->type == TYPE_FICHIER && inode->taille > 0) {
        buffer_complet = (unsigned char *)malloc(inode->taille + 1);  // +1 pour le '\0' terminal
        if (!buffer_complet) {
            printf("Erreur: Impossible d'allouer de la mémoire pour le contenu du fichier\n");
            return;
        }
        memset(buffer_complet, 0, inode->taille + 1);  // Initialiser à zéro
    }
    
    int octets_lus = 0;
    int taille_restante = inode->taille;
    
    // Affichage des blocs directs
    printf("\n=== Blocs directs ===\n");
    for (int i = 0; i < 10 && taille_restante > 0; i++) {
        if (inode->blocs_directs[i] != 0) {
            printf("Bloc direct %d: Numéro de bloc = %d (offset physique = %ld octets)\n", 
                i, inode->blocs_directs[i], (long)inode->blocs_directs[i] * TAILLE_BLOC);
            
            // Lire le contenu du bloc
            unsigned char buffer[TAILLE_BLOC];
            if (lire_bloc(inode->blocs_directs[i], buffer) == 0) {
                int taille_bloc = taille_restante < TAILLE_BLOC ? taille_restante : TAILLE_BLOC;
                
                printf("  Contenu du bloc (octets %d à %d du fichier):\n  ", 
                       octets_lus, octets_lus + taille_bloc - 1);
                
                // Afficher le contenu en texte (si possible)
                printf("Texte: ");
                for (int j = 0; j < taille_bloc; j++) {
                    if (isprint(buffer[j])) {
                        printf("%c", buffer[j]);
                    } else {
                        printf(".");
                    }
                }
                printf("\n");
                
                // Afficher l'hexdump avec 16 octets par ligne
                printf("  Hexdump:\n");
                for (int j = 0; j < taille_bloc; j += 16) {
                    printf("    %04x: ", j);
                    for (int k = 0; k < 16 && j + k < taille_bloc; k++) {
                        printf("%02x ", buffer[j + k]);
                        if (k == 7) printf(" "); // Séparateur au milieu
                    }
                    
                    // Padding pour aligner la partie texte si la ligne est incomplète
                    int padding = 16 - (taille_bloc - j < 16 ? taille_bloc - j : 16);
                    for (int k = 0; k < padding; k++) {
                        printf("   ");
                    }
                    if (padding > 7) printf(" "); // Ajustement du séparateur
                    
                    printf(" |");
                    for (int k = 0; k < 16 && j + k < taille_bloc; k++) {
                        if (isprint(buffer[j + k])) {
                            printf("%c", buffer[j + k]);
                        } else {
                            printf(".");
                        }
                    }
                    printf("|\n");
                }
                
                // Copier dans le buffer complet pour l'affichage final
                if (buffer_complet) {
                    memcpy(buffer_complet + octets_lus, buffer, taille_bloc);
                }
                
                octets_lus += taille_bloc;
                taille_restante -= taille_bloc;
            }
        }
    }
    
    // Affichage du bloc indirect
    if (inode->bloc_indirect != 0 && taille_restante > 0) {
        printf("\n=== Bloc indirect ===\n");
        printf("Bloc indirect: Numéro de bloc = %d (offset physique = %ld octets)\n", 
               inode->bloc_indirect, (long)inode->bloc_indirect * TAILLE_BLOC);
        
        // Lire le bloc indirect qui contient des pointeurs vers d'autres blocs
        int pointeurs_blocs[TAILLE_BLOC / sizeof(int)];
        if (lire_bloc(inode->bloc_indirect, pointeurs_blocs) == 0) {
            printf("Contient des pointeurs vers %lu blocs maximum\n", TAILLE_BLOC / sizeof(int));
            
            // Afficher les blocs référencés par le bloc indirect
            int nb_pointeurs_valides = 0;
            for (int i = 0; i < TAILLE_BLOC / sizeof(int) && taille_restante > 0; i++) {
                if (pointeurs_blocs[i] != 0) {
                    nb_pointeurs_valides++;
                    printf("  Pointeur %d: Bloc %d (offset physique = %ld octets)\n", 
                           i, pointeurs_blocs[i], (long)pointeurs_blocs[i] * TAILLE_BLOC);
                    
                    // Lire le contenu du bloc référencé
                    unsigned char buffer[TAILLE_BLOC];
                    if (lire_bloc(pointeurs_blocs[i], buffer) == 0) {
                        int taille_bloc = taille_restante < TAILLE_BLOC ? taille_restante : TAILLE_BLOC;
                        
                        printf("    Contenu du bloc (octets %d à %d du fichier):\n    ", 
                               octets_lus, octets_lus + taille_bloc - 1);
                        
                        // Afficher le contenu en texte (si possible)
                        printf("Texte: ");
                        for (int j = 0; j < taille_bloc; j++) {
                            if (isprint(buffer[j])) {
                                printf("%c", buffer[j]);
                            } else {
                                printf(".");
                            }
                        }
                        printf("\n");
                        
                        // Afficher l'hexdump avec 16 octets par ligne
                        printf("    Hexdump:\n");
                        for (int j = 0; j < taille_bloc; j += 16) {
                            printf("      %04x: ", j);
                            for (int k = 0; k < 16 && j + k < taille_bloc; k++) {
                                printf("%02x ", buffer[j + k]);
                                if (k == 7) printf(" "); // Séparateur au milieu
                            }
                            
                            // Padding pour aligner la partie texte si la ligne est incomplète
                            int padding = 16 - (taille_bloc - j < 16 ? taille_bloc - j : 16);
                            for (int k = 0; k < padding; k++) {
                                printf("   ");
                            }
                            if (padding > 7) printf(" "); // Ajustement du séparateur
                            
                            printf(" |");
                            for (int k = 0; k < 16 && j + k < taille_bloc; k++) {
                                if (isprint(buffer[j + k])) {
                                    printf("%c", buffer[j + k]);
                                } else {
                                    printf(".");
                                }
                            }
                            printf("|\n");
                        }
                        
                        // Copier dans le buffer complet pour l'affichage final
                        if (buffer_complet) {
                            memcpy(buffer_complet + octets_lus, buffer, taille_bloc);
                        }
                        
                        octets_lus += taille_bloc;
                        taille_restante -= taille_bloc;
                    }
                }
            }
            printf("Total de %d pointeurs valides dans le bloc indirect\n", nb_pointeurs_valides);
        }
    } else if (inode->bloc_indirect == 0) {
        printf("\nPas de bloc indirect utilisé\n");
    }
    
    // Affichage du contenu complet du fichier (si c'est un fichier texte)
    if (inode->type == TYPE_FICHIER && buffer_complet) {
        printf("\n=== Contenu complet du fichier ===\n");
        // Vérifier si le fichier semble être du texte
        int est_texte = 1;
        for (int i = 0; i < inode->taille && est_texte; i++) {
            if (!isprint(buffer_complet[i]) && !isspace(buffer_complet[i]) && buffer_complet[i] != 0) {
                est_texte = 0;
            }
        }
        
        if (est_texte) {
            // Assurer que le buffer est terminé par un caractère nul
            buffer_complet[inode->taille] = '\0';
            printf("%s\n", buffer_complet);
        } else {
            printf("(Fichier contient des données binaires non affichables en texte)\n");
        }
    }
    
    // Libérer la mémoire
    if (buffer_complet) {
        free(buffer_complet);
    }
    
    // Calcul et affichage des statistiques d'utilisation
    int blocs_directs_utilises = 0;
    for (int i = 0; i < 10; i++) {
        if (inode->blocs_directs[i] != 0) {
            blocs_directs_utilises++;
        }
    }
    
    int blocs_indirects_utilises = 0;
    if (inode->bloc_indirect != 0) {
        int pointeurs_blocs[TAILLE_BLOC / sizeof(int)];
        if (lire_bloc(inode->bloc_indirect, pointeurs_blocs) == 0) {
            for (int i = 0; i < TAILLE_BLOC / sizeof(int); i++) {
                if (pointeurs_blocs[i] != 0) {
                    blocs_indirects_utilises++;
                }
            }
        }
    }
    
    printf("\n=== Résumé de l'utilisation des blocs ===\n");
    printf("Blocs directs utilisés: %d/10\n", blocs_directs_utilises);
    printf("Bloc indirect: %s\n", inode->bloc_indirect != 0 ? "Utilisé" : "Non utilisé");
    printf("Blocs via indirection: %d/%lu\n", blocs_indirects_utilises, TAILLE_BLOC / sizeof(int));
    printf("Total des blocs utilisés: %d\n", blocs_directs_utilises + (inode->bloc_indirect != 0 ? 1 : 0) + blocs_indirects_utilises);
    printf("Espace théorique occupé: %d octets\n", (blocs_directs_utilises + (inode->bloc_indirect != 0 ? 1 : 0) + blocs_indirects_utilises) * TAILLE_BLOC);
    printf("Taille réelle du fichier: %d octets\n", inode->taille);
    printf("Taux d'utilisation: %.2f%%\n", inode->taille > 0 ? 
           (float)inode->taille / ((blocs_directs_utilises + (inode->bloc_indirect != 0 ? 1 : 0) + blocs_indirects_utilises) * TAILLE_BLOC) * 100 : 0);
}


/**
 * Défragmente le système de fichiers en réorganisant les blocs
 * pour rendre les fichiers contigus et l'espace libre consolidé
 * @return 0 si succès, -1 si erreur
 */
int defragmenter() {
    printf("Démarrage de la défragmentation...\n");
    
    // Allouer un bitmap temporaire pour le suivi
    uint8_t bitmap_temp[TAILLE_BITMAP];
    memset(bitmap_temp, 0, TAILLE_BITMAP);
    
    // Marquer les blocs réservés au système comme utilisés
    bitmap_temp[0] = 1; // Superbloc
    
    // Réserver les blocs pour la table d'inodes
    int blocs_inodes = (NB_INODES * sizeof(Inode) + TAILLE_BLOC - 1) / TAILLE_BLOC;
    for (int i = 1; i <= blocs_inodes; i++) {
        bitmap_temp[i / BITS_PAR_OCTET] |= (1 << (i % BITS_PAR_OCTET));
    }
    
    MapBloc* map_blocs = malloc(NB_BLOCS * sizeof(MapBloc));
    if (!map_blocs) {
        erreur("Échec d'allocation mémoire pour la défragmentation");
        return -1;
    }
    int nb_blocs_mappés = 0;
    
    // Pour chaque inode, traiter ses blocs
    for (int i = 0; i < NB_INODES; i++) {
        Inode* inode = &inodes[i];
        
        // Ignorer les inodes vides/non utilisés
        if (inode->taille == 0 || inode->nb_liens == 0) continue;
        
        // Calculer le nombre de blocs nécessaires pour ce fichier
        int nb_blocs = (inode->taille + TAILLE_BLOC - 1) / TAILLE_BLOC;
        if (nb_blocs == 0) continue;
        
        // Trouver une zone contiguë suffisamment grande
        int bloc_debut = blocs_inodes + 1; // Premier bloc après la table d'inodes
        int espace_contigu = 0;
        
        while (bloc_debut < NB_BLOCS) {
            // Vérifie si ce bloc est libre dans le bitmap temporaire
            if (!(bitmap_temp[bloc_debut / BITS_PAR_OCTET] & (1 << (bloc_debut % BITS_PAR_OCTET)))) {
                espace_contigu++;
                if (espace_contigu >= nb_blocs) break;
            } else {
                espace_contigu = 0;
            }
            bloc_debut++;
            
            // Si on n'a pas trouvé assez d'espace, on revient au début
            if (bloc_debut + espace_contigu >= NB_BLOCS) {
                erreur("Impossible de trouver suffisamment d'espace contigu");
                free(map_blocs);
                return -1;
            }
        }
        
        // Début de la zone contiguë
        bloc_debut = bloc_debut - espace_contigu + 1;
        
        // Pour les blocs directs
        for (int j = 0; j < nb_blocs && j < 10; j++) {
            if (inode->blocs_directs[j] != 0) {
                // Enregistrer le mapping
                map_blocs[nb_blocs_mappés].ancien_bloc = inode->blocs_directs[j];
                map_blocs[nb_blocs_mappés].nouveau_bloc = bloc_debut + j;
                nb_blocs_mappés++;
                
                // Mettre à jour l'inode (on le fera plus tard)
                // inode->blocs_directs[j] = bloc_debut + j;
                
                // Marquer comme utilisé dans le bitmap temporaire
                bitmap_temp[(bloc_debut + j) / BITS_PAR_OCTET] |= (1 << ((bloc_debut + j) % BITS_PAR_OCTET));
            }
        }
        
        // Pour les blocs indirects si nécessaire
        if (inode->bloc_indirect != 0 && nb_blocs > 10) {
            int blocs_indirects[TAILLE_BLOC / sizeof(int)];
            lire_bloc(inode->bloc_indirect, blocs_indirects);
            
            // Allouer un nouveau bloc pour le bloc indirect
            int nouveau_bloc_indirect = bloc_debut + nb_blocs;
            
            // Marquer comme utilisé
            bitmap_temp[nouveau_bloc_indirect / BITS_PAR_OCTET] |= (1 << (nouveau_bloc_indirect % BITS_PAR_OCTET));
            
            // Enregistrer le mapping pour le bloc indirect
            map_blocs[nb_blocs_mappés].ancien_bloc = inode->bloc_indirect;
            map_blocs[nb_blocs_mappés].nouveau_bloc = nouveau_bloc_indirect;
            nb_blocs_mappés++;
            
            // Mettre à jour les références des blocs indirects
            for (int j = 0; j < TAILLE_BLOC / sizeof(int) && j + 10 < nb_blocs; j++) {
                if (blocs_indirects[j] != 0) {
                    // Enregistrer le mapping
                    map_blocs[nb_blocs_mappés].ancien_bloc = blocs_indirects[j];
                    map_blocs[nb_blocs_mappés].nouveau_bloc = bloc_debut + 10 + j;
                    nb_blocs_mappés++;
                    
                    // Marquer comme utilisé
                    bitmap_temp[(bloc_debut + 10 + j) / BITS_PAR_OCTET] |= (1 << ((bloc_debut + 10 + j) % BITS_PAR_OCTET));
                }
            }
        }
    }
    
    // Copier les données des blocs
    char buffer[TAILLE_BLOC];
    for (int i = 0; i < nb_blocs_mappés; i++) {
        // Lire l'ancien bloc
        lire_bloc(map_blocs[i].ancien_bloc, buffer);
        // Écrire dans le nouveau bloc
        ecrire_bloc(map_blocs[i].nouveau_bloc, buffer);
    }
    
    // Mettre à jour les inodes avec les nouvelles références de blocs
    for (int i = 0; i < NB_INODES; i++) {
        Inode* inode = &inodes[i];
        
        // Ignorer les inodes vides
        if (inode->taille == 0 || inode->nb_liens == 0) continue;
        
        // Mettre à jour les blocs directs
        for (int j = 0; j < 10; j++) {
            if (inode->blocs_directs[j] != 0) {
                // Chercher la nouvelle position
                for (int k = 0; k < nb_blocs_mappés; k++) {
                    if (map_blocs[k].ancien_bloc == inode->blocs_directs[j]) {
                        inode->blocs_directs[j] = map_blocs[k].nouveau_bloc;
                        break;
                    }
                }
            }
        }
        
        // Mettre à jour le bloc indirect
        if (inode->bloc_indirect != 0) {
            // Chercher la nouvelle position du bloc indirect
            for (int k = 0; k < nb_blocs_mappés; k++) {
                if (map_blocs[k].ancien_bloc == inode->bloc_indirect) {
                    inode->bloc_indirect = map_blocs[k].nouveau_bloc;
                    break;
                }
            }
            
            // Lire le contenu du bloc indirect
            int blocs_indirects[TAILLE_BLOC / sizeof(int)];
            lire_bloc(inode->bloc_indirect, blocs_indirects);
            
            // Mettre à jour les références
            for (int j = 0; j < TAILLE_BLOC / sizeof(int); j++) {
                if (blocs_indirects[j] != 0) {
                    for (int k = 0; k < nb_blocs_mappés; k++) {
                        if (map_blocs[k].ancien_bloc == blocs_indirects[j]) {
                            blocs_indirects[j] = map_blocs[k].nouveau_bloc;
                            break;
                        }
                    }
                }
            }
            
            // Réécrire le bloc indirect
            ecrire_bloc(inode->bloc_indirect, blocs_indirects);
        }
    }
    
    // Remplacer l'ancien bitmap par le nouveau
    memcpy(bitmap, bitmap_temp, TAILLE_BITMAP);
    
    // Mettre à jour le superbloc
    superbloc.derniere_modification = time(NULL);
    
    // Sauvegarder les changements
    sauvegarder_partition();
    
    // Libérer la mémoire
    free(map_blocs);
    
    printf("Défragmentation terminée avec succès.\n");
    return 0;
}

/**
 * Initialise une nouvelle partition de système de fichiers
 * @param nom_partition Le nom du fichier de partition
 */
void initialiser_partition(const char* nom_partition) {
    // Ouvrir le fichier en écriture
    partition_file = fopen(nom_partition, "wb+");
    if (!partition_file) {
        perror("Erreur d'ouverture de la partition");
        exit(EXIT_FAILURE);
    }
    
    // Créer un fichier de la bonne taille
    fseek(partition_file, TAILLE_PARTITION - 1, SEEK_SET);
    fputc(0, partition_file);
    fseek(partition_file, 0, SEEK_SET);
    
    // Initialiser le bitmap
    memset(bitmap, 0, TAILLE_BITMAP);
    
    // Réserver les blocs pour le superbloc et la table d'inodes
    bitmap[0] = 1; // Superbloc
    
    // Calculer le nombre de blocs nécessaires pour la table d'inodes
    int blocs_inodes = (NB_INODES * sizeof(Inode) + TAILLE_BLOC - 1) / TAILLE_BLOC;
    for (int i = 1; i <= blocs_inodes; i++) {
        bitmap[i / BITS_PAR_OCTET] |= (1 << (i % BITS_PAR_OCTET));
    }
    
    // Initialiser les inodes
    memset(inodes, 0, NB_INODES * sizeof(Inode));
    
    // Initialiser le superbloc
    strcpy(superbloc.identifiant_fs, "MONFSS");
    superbloc.emplacement_racine = 0;
    superbloc.derniere_modification = time(NULL);
    superbloc.verifier_integrite = 0;
    superbloc.taille_partition = TAILLE_PARTITION;
    superbloc.nb_blocs = NB_BLOCS;
    superbloc.nb_inodes = NB_INODES;
    superbloc.taille_bloc = TAILLE_BLOC;
    superbloc.nb_blocs_libres = NB_BLOCS - blocs_inodes - 1;
    superbloc.nb_inodes_libres = NB_INODES - 1;
    
    // Créer le répertoire racine
    Inode* racine = &inodes[0];
    racine->type = TYPE_REPERTOIRE;
    racine->date_creation = time(NULL);
    racine->date_modification = time(NULL);
    racine->date_acces = time(NULL);
    racine->nb_liens = 1;
    racine->proprietaire = getuid();
    racine->groupe = getgid();
    racine->droits = 0755;  // rwxr-xr-x
    racine->taille = TAILLE_BLOC;
    
    // Allouer un bloc pour le répertoire racine
    int bloc_racine = trouver_bloc_libre();
    racine->blocs_directs[0] = bloc_racine;
    
    // Initialiser le contenu du répertoire racine
    EntreeRepertoire entrees[MAX_ENTREES_DIR] = {0};
    
    // Ajouter les entrées "." et ".."
    strcpy(entrees[0].nom, ".");
    entrees[0].inode = 0; // Lui-même
    
    strcpy(entrees[1].nom, "..");
    entrees[1].inode = 0; // Son parent est lui-même
    
    // Écrire les entrées dans le bloc
    ecrire_bloc(bloc_racine, entrees);
    
    // Écrire le superbloc
    fseek(partition_file, 0, SEEK_SET);
    fwrite(&superbloc, sizeof(Superbloc), 1, partition_file);
    
    // Écrire les inodes
    fwrite(inodes, sizeof(Inode), NB_INODES, partition_file);
    
    // Écrire le bitmap
    fwrite(bitmap, TAILLE_BITMAP, 1, partition_file);
    
    // Définir le répertoire courant
    inode_courant = 0;
    
    printf("Partition initialisée avec succès : %s\n", nom_partition);
}

void afficher_bitmap(uint8_t* bitmap, int nb_blocs) {
    const int blocs_par_ligne = 16;
    
    // Afficher les en-têtes de colonnes
    printf("    ");
    for (int i = 0; i < blocs_par_ligne; i++) {
        printf("%2d ", i);
    }
    printf("\n");
    
    // Afficher une ligne de séparation
    printf("    ");
    for (int i = 0; i < blocs_par_ligne; i++) {
        printf("---");
    }
    printf("\n");
    
    // Afficher le contenu du bitmap
    for (int i = 0; i < nb_blocs; i += blocs_par_ligne) {
        // Afficher le numéro de ligne
        printf("%3d| ", i);
        
        // Afficher les bits pour cette ligne
        for (int j = 0; j < blocs_par_ligne && (i + j) < nb_blocs; j++) {
            int bloc = i + j;
            int est_utilise = bitmap[bloc / BITS_PAR_OCTET] & (1 << (bloc % BITS_PAR_OCTET));
            printf(" %c ", est_utilise ? '1' : '0');
        }
        printf("\n");
    }
    
    // Compter les blocs utilisés et libres
    int blocs_utilises = 0;
    for (int i = 0; i < nb_blocs; i++) {
        if (bitmap[i / BITS_PAR_OCTET] & (1 << (i % BITS_PAR_OCTET))) {
            blocs_utilises++;
        }
    }
    
    // Afficher les statistiques
    printf("\nStatistiques du bitmap:\n");
    printf("- Blocs totaux: %d\n", nb_blocs);
    printf("- Blocs utilisés: %d (%.1f%%)\n", blocs_utilises, (float)blocs_utilises * 100 / nb_blocs);
    printf("- Blocs libres: %d (%.1f%%)\n", nb_blocs - blocs_utilises, 
           (float)(nb_blocs - blocs_utilises) * 100 / nb_blocs);
    printf("\n");
}

/**
 * Charge une partition existante depuis le disque
 * @param nom_partition Le nom du fichier de partition
 */
void charger_partition(const char* nom_partition) {

    partition_file = fopen(nom_partition, "rb+");
    if (partition_file == NULL) {
        erreur("Erreur lors de l'ouverture du fichier de partition");
        exit(EXIT_FAILURE);
    }
    
    // Lire le superbloc
    fread(&superbloc, sizeof(Superbloc), 1, partition_file);
    
    // Vérifier l'identifiant
    if (strcmp(superbloc.identifiant_fs, "MONFSS") != 0) {
        erreur("Ce n'est pas une partition valide");
        fclose(partition_file);
        exit(EXIT_FAILURE);
    }
    
    // Lire les inodes
    fread(inodes, sizeof(Inode), NB_INODES, partition_file);
    
    // Lire le bitmap
    fread(bitmap, TAILLE_BITMAP, 1, partition_file);
    
    // Définir le répertoire courant
    inode_courant = 0;
    
    printf("Partition chargée avec succès : %s\n", nom_partition);
}

/**
 * Sauvegarde l'état du système de fichiers sur le disque
 */
void sauvegarder_partition() {
    if (!partition_file) {
        erreur("Aucune partition ouverte");
        return;
    }
    
    // Mettre à jour la date de dernière modification
    superbloc.derniere_modification = time(NULL);
    
    // Écrire le superbloc
    fseek(partition_file, 0, SEEK_SET);
    fwrite(&superbloc, sizeof(Superbloc), 1, partition_file);
    
    // Écrire les inodes
    fwrite(inodes, sizeof(Inode), NB_INODES, partition_file);
    
    // Écrire le bitmap
    fwrite(bitmap, TAILLE_BITMAP, 1, partition_file);
    
    // S'assurer que tout est écrit
    fflush(partition_file);
}
 

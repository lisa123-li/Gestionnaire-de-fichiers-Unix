/**
 * Pourcentage d'implication : 
 * GOBALOUKICHENIN Bala : 33%
 * DEHOUCHE Lisa : 33%
 * GABITA William : 33%
 * Fonction principale du système de fichiers
 * 
 * Cette fonction implémente une interface en ligne de commande pour interagir
 * avec le système de fichiers personnalisé
 * @file file_system.h
 * @brief Définitions pour un système de fichiers simplifié
 * 
 * Ce fichier contient les constantes, structures et prototypes nécessaires
 * à l'implémentation d'un système de fichiers simplifié de type UNIX.
 */

#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <ctype.h>

// =============================================
// CONSTANTES DE CONFIGURATION DU SYSTÈME
// =============================================

/* Taille de la partition virtuelle (10 Mo) */
#define TAILLE_PARTITION (1024 * 1024 * 10)

/* Taille d'un bloc de données (4 Ko - taille classique pour des systèmes modernes) */
#define TAILLE_BLOC 4096

/* Nombre total de blocs dans la partition */
#define NB_BLOCS (TAILLE_PARTITION / TAILLE_BLOC)

/* Nombre maximum d'inodes (limite le nombre de fichiers) */
#define NB_INODES 256

/* Utile pour le bitmap (1 bit par bloc) */
#define BITS_PAR_OCTET 8

/* Taille du bitmap en octets */
#define TAILLE_BITMAP ((NB_BLOCS + BITS_PAR_OCTET - 1) / BITS_PAR_OCTET)

/* Longueur maximale d'un nom de fichier */
#define MAX_NOM_FICHIER 255

/* Longueur maximale d'un chemin */
#define MAX_CHEMIN 1024

/* Nombre maximal d'entrées dans un répertoire */
#define MAX_ENTREES_DIR 128

/* Inode racine (toujours 0 dans ce système) */
#define ID_INODE_RACINE 0

// =============================================
// TYPES DE FICHIERS
// =============================================

#define TYPE_FICHIER 0        // Fichier régulier
#define TYPE_REPERTOIRE 1     // Répertoire
#define TYPE_LIEN_SYMBOLIQUE 2 // Lien symbolique
#define TYPE_LIEN_PHYSIQUE 3  // Lien physique (hard link)

// =============================================
// DROITS D'ACCÈS (UNIX STYLE)
// =============================================

#define DROIT_LECTURE 4       // Permission de lecture (r--)
#define DROIT_ECRITURE 2      // Permission d'écriture (-w-)
#define DROIT_EXECUTION 1     // Permission d'exécution (--x)

// =============================================
// STRUCTURES DE DONNÉES
// =============================================

/**
 * @struct Superbloc
 * @brief Métadonnées du système de fichiers
 * 
 * Contient les informations critiques sur le système de fichiers,
 * stockées généralement au début de la partition.
 */
typedef struct {
    char identifiant_fs[10];      // Signature du FS ("MYFSv1.0\0")
    int emplacement_racine;       // Bloc contenant le répertoire racine
    time_t derniere_modification; // Timestamp de la dernière modification
    int verifier_integrite;       // Flag de vérification d'intégrité
    int taille_partition;         // Taille totale en octets
    int nb_blocs;                 // Nombre total de blocs
    int nb_inodes;                // Nombre total d'inodes
    int taille_bloc;              // Taille d'un bloc en octets
    int nb_blocs_libres;          // Nombre de blocs libres
    int nb_inodes_libres;         // Nombre d'inodes libres
} Superbloc;

/**
 * @struct Inode
 * @brief Métadonnées d'un fichier/répertoire
 * 
 * Contient toutes les métadonnées d'un fichier, répertoire ou lien.
 * Un inode est identifié par un numéro unique.
 */
typedef struct {
    int taille;                  // Taille du fichier en octets
    int type;                    // Type (fichier, répertoire, lien)
    int proprietaire;            // UID du propriétaire
    int groupe;                  // GID du groupe
    int droits;                  // Permissions (rwxrwxrwx en octal)
    time_t date_creation;        // Date de création
    time_t date_modification;    // Date de dernière modification
    time_t date_acces;           // Date de dernier accès
    int nb_liens;                // Nombre de liens physiques
    int blocs_directs[10];       // 10 blocs directs
    int bloc_indirect;           // Bloc de pointeurs vers d'autres blocs
    char nom[MAX_NOM_FICHIER + 1]; // Nom du fichier (pour les liens)
} Inode;

/**
 * @struct EntreeRepertoire
 * @brief Entrée dans un répertoire
 * 
 * Structure représentant une entrée dans un répertoire,
 * faisant le lien entre un nom et un inode.
 */
typedef struct {
    char nom[MAX_NOM_FICHIER + 1]; // Nom du fichier (8.3 style)
    int inode;                     // Numéro d'inode associé
} EntreeRepertoire;

/**
 * @struct MapBloc
 * @brief Représente un déplacement de bloc lors de la défragmentation.
 * 
 * Structure associant un bloc d'origine à un bloc de destination pour suivre les mouvements lors de la défragmentation.
 * 
 * @param ancien_bloc Bloc d'origine avant défragmentation.
 * @param nouveau_bloc Bloc de destination après défragmentation.
 */
typedef struct {
    int ancien_bloc;
    int nouveau_bloc;
} MapBloc;
    
// =============================================
// VARIABLES GLOBALES
// =============================================

extern uint8_t bitmap[TAILLE_BITMAP];  // Bitmap des blocs libres/alloués
extern Inode inodes[NB_INODES];        // Table des inodes
extern Superbloc superbloc;            // Superbloc du système
extern FILE* partition_file;           // Fichier représentant la partition
extern int inode_courant;              // Inode du répertoire courant
extern EntreeRepertoire entrees[MAX_ENTREES_DIR];

// =============================================
// PROTOTYPES DES FONCTIONS
// =============================================

/* Fonctions de gestion de la partition */
void initialiser_partition(const char* nom_partition);
void charger_partition(const char* nom_partition);
void sauvegarder_partition();
int defragmenter();

/* Gestion des blocs */
int trouver_bloc_libre();
void liberer_bloc(int num_bloc);
void afficher_bitmap(uint8_t* bitmap, int nb_blocs);

/* Gestion des inodes */
int trouver_inode_libre();
void liberer_inode(int num_inode);
void afficher_inode(const Inode *inode);
Inode* trouver_ind(char* nom);

/* Opérations sur les blocs */
void ecrire_bloc(int num_bloc, void* donnees);
int lire_bloc(int num_bloc, void* donnees);

/* Utilitaires */
void erreur(const char* message);
int valider_nom_fichier(const char* nom);
void mySeek(FILE *f, long offset, int base);

/* Opérations sur les fichiers */
int creer_fichier(const char* nom, int type);
int supprimer_fichier(const char* nom);
void sauvegarder_etat(const char* fichier_sauvegarde);
void restaurer_etat(const char* fichier_sauvegarde);

/* Gestion des répertoires */
int ajouter_entree_repertoire(int inode_dir, const char* nom, int inode);
int supprimer_entree_repertoire(int inode_dir, const char* nom);
int trouver_inode_par_nom(int inode_dir, const char* nom);

/* Opérations de lecture/écriture */
int lire_fichier(int inode_id, void* buffer, int taille, int offset);
int ecrire_fichier(int inode_id, void* buffer, int taille, int offset);

/* Gestion des permissions */
int verifier_droits(int num_inode, int droits_requis);

/* Gestion des liens */
int creer_lien(const char* source, const char* nom_lien);
int creer_lien_symbolique(const char* source, const char* destination);

/* Navigation */
void afficher_repertoire(int inode_dir);
int changer_repertoire(const char* chemin);

/* Opérations sur les fichiers */
int copier_fichier(const char* source, const char* destination);
int deplacer_fichier(const char* source, const char* destination);
void sauvegarder_etat(const char* fichier_sauvegarde);
void restaurer_etat(const char* fichier_sauvegarde);


/* Gestion des permissions */
int modifier_droits(int inode_id, int nouveaux_droits);
int convertir_droits_num(const char* droits);
int convertir_droits_char(const char* droits);

#endif // FILE_SYSTEM_H

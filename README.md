# Gestionnaire-de-fichiers-Unix
# 🗂️ Gestionnaire de Fichiers Personnalisé

Ce projet implémente un mini système de fichiers en C, permettant à l'utilisateur d'interagir avec une partition virtuelle via une interface en ligne de commande. Il prend en charge des commandes classiques comme `ls`, `mkdir`, `touch`, `chmod`, `write`, etc.

---

## 📁 Structure du projet


- `main.c` : Contient la boucle principale du programme et l'interface en ligne de commande.  
- `file_system.c` : Contient les fonctions principales de gestion du système de fichiers.  
- `file_system.h` : Contient les définitions, constantes, types et en-têtes nécessaires.  
- `Makefile` : Automatisation de la compilation, documentation et installation.  
- `Doxyfile` : Fichier de configuration pour générer la documentation avec Doxygen.

📝 **NB** : Tous ces fichiers doivent se trouver dans le même répertoire.

---

## ⚙️ Compilation

# Avec Makefile

1. Ouvrir un terminal.

2. Se placer dans le dossier contenant tous les fichiers :
 cd chemin/vers/ton/projet

3. Compiler le projet avec : make
 -- Cela génère un exécutable nommé gestionnairefs. --
 
4. Génération de la documentation : make doc 

# Sans Makefile

1. Ouvrir un terminal

2. Se placer dans le dossier contenant tous les fichiers :
 cd chemin/vers/ton/projet

3. Compiler le projet avec : gcc -o gestionnairefs main.c file_system.c

## ▶ Installation du programme

Execution des commandes : 
 
```bash
 make install
```

```bash
 export PATH="$HOME/.local/bin:$PATH"
```

```bash
 source ~/.bashrc    # ou source ~/.zshrc
```

## ▶️ Exécution

Option 1 :  Depuis le dossier courant

Une fois compilé, le prgramme peut être exécuté :

```bash
./gestionnairefs
```

Si une partition nommée `partition.bin` existe, elle sera chargée. Sinon, une nouvelle partition sera créée.

---

Option 2 : Depuis n’importe où (si installé avec make install)

```bash
gestionnairefs
```


## 📚 Commandes disponibles

- `aide` : Affiche l’aide avec les commandes disponibles.
- `cat <nom>` : Affiche le contenu d’un fichier.
- `cd <rep>` : Change de répertoire.
- `chmod <nom> <droit>` : Modifie les droits d’un fichier.
- `cp <src> <dest>` : Copie un fichier.
- `defrag` : Défragmentation le système de fichiers en réorganisant les blocs.
- `ln <src> <dest>` : Crée un lien physique.
- `lns <src> <dest>` : Crée un lien symbolique.
- `ls` : Affiche le contenu du répertoire courant.
- `mkdir <nom>` : Crée un nouveau répertoire.
- `mv <src> <dest>` : Déplace ou renomme un fichier.
- `rm <nom>` : Supprime un fichier ou répertoire.
- `save <backup.bin>` : Sauvegarde de l’état actuel de la partition dans un fichier.
- `load <backup.bin>` : Restauration d’une partition depuis un fichier de sauvegarde.
- `touch <nom>` : Crée un fichier vide.
- `write <nom>` : Permet d’écrire dans un fichier (mode interactif).
- `quit` : Sauvegarde et quitte le programme.

Pour exécuter une commande, il suffit de suivre la manière dont elle est présenter en remplace ce qu'il y a '< >' par l'information souhaité : 
- `<rep>` : A remplacer par le nom du repertoire souhaité.
- `<nom>` : A remplacer par le nom du fichier souhaité.
- `<src>` : A remplacer par le nom du fichier source.
- `<dest>` : A remplacer par le nom du fichier de destination.
- `<droit>` : A remplacer par les droits souhaités (notation symbolique).

📝 **NB** : "backup.bin" est un fichier par défaut, il est tout à fait possible de le nommer comme on le souhaite en n'oubliant pas l'extention ".bin".
---

## 🧠 Concepts clés implémentés

- Système de fichiers virtuel basé sur une partition binaire (`partition.bin`).
- Inodes pour la gestion des fichiers.
- Répertoires hiérarchiques.
- Permissions de fichiers (`chmod`).
- Liens physiques et symboliques.
- Persistance entre les exécutions via sauvegarde automatique.

---

## 📝 Exemple d'utilisation

```bash
> mkdir documents
> cd documents
> touch notes.txt
> write notes.txt
> cat notes.txt
> chmod notes.txt rw-
> ls
```

---

## 📌 Remarques

- Le système de fichiers est entièrement simulé, aucun fichier réel du système n'est affecté.
- Les tailles de fichiers sont limitées par le buffer en mémoire (actuellement à 1024 octets pour l’écriture).
- La partition est sauvegardée automatiquement après chaque commande.


## Commande make:
make            # Compile le projet (équivalent à make all)
make all        # Compile les fichiers source et génère l'exécutable 'gestionnairefs'
make install    # Installe l'exécutable dans ~/.local/bin
make uninstall  # Supprime l'exécutable installé
make clean      # Supprime les fichiers objets (.o) et l'exécutable
make check      # Vérifie la présence des fichiers et outils nécessaires
make help       # Affiche cette aide

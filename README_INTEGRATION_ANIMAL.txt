INTEGRATION DU MODULE ANIMAL

Le module Animal a été intégré au projet principal Smart-Pet-Care-Center-Management.

Fichiers ajoutés :
- animal.h
- animal.cpp

Fichiers modifiés :
- SmartPetCare.pro : ajout de animal.cpp et animal.h
- gsmartpetcare.h : déclaration du modèle Animal, des slots et de l'état du module
- gsmartpetcare.cpp : initialisation de la page Animal, navigation et fonctionnalités Animal

Fichiers conservés :
- main.cpp, connection.h/.cpp : connexion Oracle existante conservée
- service.*, RendezVous.* : modules existants conservés
- gsmartpetcare.ui et resources.qrc : interface principale conservée

Fonctionnalités intégrées :
- Ajouter, modifier, supprimer et afficher les animaux
- Recherche, filtre par espèce et tri
- Photo de l'animal
- Export PDF
- Statistiques et chatbot Animal
- Dossier médical local

Base de données :
Aucune modification de la base de données ni de la configuration de connexion n'a été effectuée.
Le code Animal utilise la table Oracle ANIMAL et les colonnes suivantes :
ID_ANIMAL, NOM_ANIMAL, ESPECE, SEXE, DATE_NAISSANCE, PHOTO, PROPRIETAIRE,
TEL_PROPRIETAIRE, EMAIL_PROPRIETAIRE, SUIVI_SANTE.

Le dossier médical est conservé localement dans dossiers_medicaux.txt via QSettings.
Les identifiants de connexion Oracle restent ceux du projet principal.

Ouverture :
1. Ouvrir SmartPetCare.pro dans Qt Creator.
2. Vérifier le kit Qt déjà utilisé par le projet.
3. Exécuter Run qmake.
4. Faire Rebuild All.
5. Lancer l'application et ouvrir le menu Animaux.

Note de vérification :
L'environnement de préparation ne contient pas Qt/qmake ni le pilote Oracle du projet.
Le code et les références UI ont été vérifiés statiquement, mais la compilation et les opérations Oracle
doivent être validées dans l'environnement Qt Creator du projet.

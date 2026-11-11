ADAPTATION A LA BASE Oracle (export.sql)

1) Module Employé : identique à l'original (boutons, droits, statuts, postes).
   Seules corrections dans employe.cpp :
   - MOY_DE_PASSE -> MOT_DE_PASSE (nom réel de la colonne Oracle)
   - vérification du mot de passe : accepte le hash "sel$hash" ET le mot de passe en clair de la base.
   Exécutez donnees_employe_connexion.sql (statut Actif + poste Responsable RH pour l'employé 1),
   puis connexion : identifiant 1 / mot de passe ali2004.

2) Module Animal (gsmartpetcare.cpp / gsmartpetcare.h)
   - Colonnes du tableau elargies (ID 90, Nom 170, Espece 130, Sexe 110, Naissance 160,
     Proprietaire 190, Telephone 160, Suivi 200, Actions 230) + marges internes,
     lignes plus hautes (56 px), boutons d'action plus espaces.
   - L'espace libre est reparti sur Nom / Proprietaire / Suivi quand la fenetre est large ;
     une barre de defilement horizontale apparait si la fenetre est trop etroite.

Apres copie : Run qmake puis Rebuild All dans Qt Creator.

3) Refactoring : attributs/méthodes déplacés hors de service.h
   - animal.h / animal.cpp      : Animal::rechercherParTexte(), Animal::statistiquesServices()
                                  (anciennement Service::rechercherAnimaux / statistiquesAnimal ;
                                   les attributs idAnimal, nomAnimal, espece, proprietaire = champs id, nom, espece, proprietaire d'Animal)
   - RendezVous.h / .cpp        : attributs idRdv, dateRdv, typeRdv (+ getters/setters) et
                                  RendezVous::rendezVousAnimal() (anciennement Service::rendezVousAnimal)
   - service.h                  : un service rendu (INCLURE) porte son RendezVous : getRdv() / setRdv()
   - gsmartpetcare.h/.cpp       : HistoriqueServices utilise Animal et RendezVous.

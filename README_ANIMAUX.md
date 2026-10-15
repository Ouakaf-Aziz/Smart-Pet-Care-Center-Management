# Intégration du module Animaux

Le module Animaux (ex-projet « PetNova CHATBOT DIAGNOSTIC ») est intégré comme les modules
Services, Rendez-vous et Employés : **un seul `gsmartpetcare.ui`**, une classe qui pilote la page.

## Ce qui a été ajouté
- `gsmartpetcare.ui` : page `pageAnimaux` (dans le `QScrollArea` `scrollAnimaux`, 7e page de `stackContenu`).
  Le bouton de menu « Gestion des animaux » (`btnNavAnimaux`) existait déjà : il est maintenant relié.
- `gestionanimaux.h/.cpp` : classe `GestionAnimaux` (CRUD, recherche, tri, filtre par espèce, export PDF,
  rappels de santé, chatbot de diagnostic, bouton « Historique des services »).
- `statistiquesanimaux.h/.cpp` : statistiques (espèces, sexe, tranches d'âge, naissances par mois),
  avec les mêmes graphiques que les autres modules (`DonutChart`, `BarChart`, `LineChart`).
- `SmartPetCare.pro` : nouveaux fichiers ajoutés (aucun module Qt supplémentaire : l'export PDF utilise
  `QPdfWriter`, plus besoin de `printsupport`).

## Liens avec les autres modules
- **Services** : la liste des animaux de l'historique des services rendus vient maintenant du module Animaux
  (les animaux de test temporaires ont été supprimés). Ajout / modification / suppression d'un animal met à
  jour l'historique ; supprimer un animal efface aussi son historique de services.
- **Employés** : l'assistant de diagnostic affiche le nom de l'employé connecté (plus de saisie du nom).
- **Accueil** : la pastille de la carte « Module Animaux » affiche le nombre d'animaux.

## Boutons rouges
Tous les boutons « Annuler » sont rouges (#C0392B, survol #A5311F), y compris celui du module Animaux
(il était gris dans le projet d'origine).

## Nouveautés par rapport au projet PetNova d'origine
- Le courriel, la photo et le suivi santé sont réellement conservés (ils étaient perdus à l'enregistrement).
- Nouveau champ « Suivi santé » dans le formulaire : alimente les rappels et les statistiques.
- Validation du téléphone et de l'e-mail ; confirmation avant suppression.
- Les données restent en mémoire (comme les autres modules) : tout est perdu à la fermeture.

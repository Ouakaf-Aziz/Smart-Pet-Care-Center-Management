# Intégration du module Employés

Tout est dans un seul `gsmartpetcare.ui` (services, rendez-vous, employés).

- Connexion : email + mot de passe des employés (ou reconnaissance faciale). Comptes de démo :
  rh@petnova.tn / admin123 ; sonia.trabelsi@petnova.tn / petnova123 (comptable) ; autres : petnova123.
  Les comptes « Inactif » sont refusés.
- Menu « Gestion des employés » : liste, CRUD, tri, recherche, export PDF, statistiques, rôles et privilèges.
- Bouton « Annuler » rouge (#C0392B) partout.
- Reconnaissance faciale optionnelle : OpenCV >= 4.5.4 + modèles .onnx dans models/ (voir models/LISEZMOI.md).
  Windows : décommenter les lignes OpenCV dans SmartPetCare.pro.
- Fichiers ajoutés : gestionemployes, statistiquesemployes, connexion, privileges, faceauth, facedialog,
  faceengine, linechart, donneesemployes.h, models/.

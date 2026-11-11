# Intégration du module Employés

Tout est dans un seul `gsmartpetcare.ui`. Seul le module employés a du code (requêtes SQL dans `employe.cpp`) ;
les pages Services, Rendez-vous et Animaux sont conservées en interface seule, sans aucun métier.

- Connexion : email + mot de passe des employés (ou reconnaissance faciale). Comptes de démo :
  rh@petnova.tn / admin123 ; sonia.trabelsi@petnova.tn / petnova123 (comptable) ; autres : petnova123.
  Les comptes « Inactif » sont refusés.
- Menu « Gestion des employés » : liste, CRUD, tri, recherche, export PDF, statistiques, rôles et privilèges.
- Bouton « Annuler » rouge (#C0392B) partout.
- Reconnaissance faciale optionnelle : OpenCV >= 4.5.4 + modèles .onnx dans models/ (voir models/LISEZMOI.md).
  Windows : décommenter les lignes OpenCV dans SmartPetCare.pro.
- Tout le code du module (SQL, gestion, connexion, privilèges, statistiques, graphiques, reconnaissance faciale)
  est regroupé dans `employe.cpp` / `employe.h`. Le dossier models/ contient les modèles .onnx.

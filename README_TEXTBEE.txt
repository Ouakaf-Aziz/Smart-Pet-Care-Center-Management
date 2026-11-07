RAPPEL SMS AUTOMATIQUE - TEXTBEE
================================

CE QU'IL FAUT FAIRE (une seule fois)
------------------------------------
1) Ouvrir textbee.ini (a la racine du projet) et renseigner :
     api_key=...    (textbee.dev -> Generate API key)
     device_id=...  (textbee.dev -> Devices -> ID du telephone)
     rappel_avant_heures=2   (le SMS part 2 h avant le debut du RDV)
2) Telephone Android : app TextBee ouverte/autorisee, SMS autorises, internet, appareil "Active",
   SIM avec credit / forfait SMS.
3) Compiler (Build -> Rebuild All) et lancer l'application.

Le fichier textbee.ini est cherche dans cet ordre : dossier de donnees de l'application
(AppData/Roaming/PetNova/GestionEmployes), dossier de l'exe et ses dossiers parents
(donc la racine du projet), dossier courant. Les variables d'environnement
TEXTBEE_API_KEY / TEXTBEE_DEVICE_ID sont prioritaires si elles existent.
Le chemin du fichier lu est ecrit dans rappels_sms.log.

OU EST LE CODE
--------------
Classe Service : methodes statiques du rappel declarees dans service.h, definies en fin de service.cpp.
main.cpp ne contient que la ligne  Service::demarrerRappels(&a);

FONCTIONNEMENT
--------------
- Verification au demarrage puis toutes les 5 minutes, tant que l'application est ouverte.
- SMS envoye a ANIMAL.TEL_PROPRIETAIRE quand le RDV (DATE_RDV + heure de HEURE_RDV) n'a pas
  commence et debute dans moins de rappel_avant_heures heures.
- Message : nom du proprietaire, animal, type de RDV, services (table INCLURE / SERVICE),
  date et heure.
- RDV annules (statut commencant par ANNUL) ignores. Un RDV n'est jamais notifie deux fois
  (rappels_sms.ini, local : la cle est numero_RDV + date + "_avant").
- Max 10 SMS par cycle, espaces de 3 s. Numeros a 8 chiffres -> +216 ajoute.
- Journal : rappels_sms.log (401/403 = cle invalide, 404 = device_id incorrect).
- Aucune modification Oracle (SELECT uniquement).

TESTER
------
Creer un NOUVEAU RDV pour un animal dont le proprietaire a un numero valide, aujourd'hui,
dans 30 a 60 minutes. Laisser l'application ouverte (verification toutes les 5 min).
simulation=true permet de voir dans le journal le SMS qui serait envoye, sans rien envoyer.
Pour renvoyer un RDV deja notifie : supprimer sa ligne finissant par _avant dans
%APPDATA%/PetNova/GestionEmployes/rappels_sms.ini

IMPORTANT
---------
- Les rappels ne partent que si l'application est ouverte.
- HTTP 2xx = demande acceptee par TextBee, pas forcement SMS delivre (SIM sans credit = "Non envoye").
- Endpoint : POST https://api.textbee.dev/api/v1/gateway/devices/{deviceId}/send-sms
- Ne partagez pas votre cle API (textbee.ini est dans .gitignore).

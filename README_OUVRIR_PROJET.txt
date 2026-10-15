PETNOVA - OUVERTURE SUR UN AUTRE PC

1. Extraire TOUT le ZIP dans un dossier local simple (ex: C:/PetNova).
2. Installer Qt Creator avec un kit Desktop Qt et un compilateur compatible.
3. Dans Qt Creator: Fichier > Ouvrir fichier ou projet > SmartPetCare.pro.
4. Configurer un kit Desktop puis Build > Run qmake, Build > Rebuild.
5. Ne pas ouvrir un .exe copie depuis un autre ordinateur: il lui manque potentiellement les DLL Qt.
6. En cas d erreur, relever le texte exact dans la fenetre Compile Output.

Un seul fichier .ui: gsmartpetcare.ui.
Le bouton Historique des services reste dans le module Services uniquement.

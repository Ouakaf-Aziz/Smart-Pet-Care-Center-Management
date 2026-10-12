# Modèles de reconnaissance faciale

Deux fichiers `.onnx` sont nécessaires dans ce dossier (ils ne sont pas inclus : 37 Mo) :

| Fichier | Rôle | Licence |
|---|---|---|
| `face_detection_yunet_2023mar.onnx` | détecte les visages dans l'image | MIT |
| `face_recognition_sface_2021dec.onnx` | transforme un visage en empreinte de 128 valeurs | Apache 2.0 |

Téléchargement automatique : `./telecharger_modeles.sh` (Linux/macOS) ou `telecharger_modeles.ps1` (Windows).

À la main : https://github.com/opencv/opencv_zoo (dossiers `models/face_detection_yunet` et
`models/face_recognition_sface`). Ce sont des fichiers Git LFS : utilisez le bouton « Download raw file »,
pas « Copy raw contents » ; un fichier de quelques dizaines d'octets est un pointeur LFS, pas le modèle
(l'application le détecte et vous le dit).

Avec OpenCV < 4.8, si `face_detection_yunet_2023mar.onnx` ne se charge pas, le script télécharge aussi
`face_detection_yunet_2022mar.onnx`, essayé automatiquement ensuite.

Le dossier est copié à côté de l'exécutable à la compilation (CMake) ; l'application cherche aussi
dans `./models` et dans le dossier des sources.

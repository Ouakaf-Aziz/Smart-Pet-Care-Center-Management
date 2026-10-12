#!/bin/sh
# Télécharge les deux modèles OpenCV Zoo nécessaires à la reconnaissance faciale.
# (YuNet : MIT, SFace : Apache 2.0). Lancer depuis n'importe où : le script se place dans models/.
cd "$(dirname "$0")" || exit 1
BASE=https://github.com/opencv/opencv_zoo/raw/main/models

get() {  # nom  dossier  taille_minimale  obligatoire
    echo "Téléchargement de $1 ..."
    curl -fL -o "$1" "$BASE/$2/$1" 2>/dev/null
    taille=$(wc -c < "$1" 2>/dev/null || echo 0)
    if [ "$taille" -lt "$3" ]; then
        rm -f "$1"
        [ "$4" = "oui" ] && { echo "ECHEC : $1 (fichier absent ou incomplet)"; exit 1; }
        echo "  (facultatif, ignoré)"
    else
        echo "  OK ($taille octets)"
    fi
}

get face_detection_yunet_2023mar.onnx   face_detection_yunet   100000  oui
get face_recognition_sface_2021dec.onnx face_recognition_sface  1000000 oui
# Facultatif : version de YuNet pour OpenCV < 4.8 (utilisée automatiquement si la 2023mar ne se charge pas).
get face_detection_yunet_2022mar.onnx   face_detection_yunet   100000  non
echo "Terminé."

QT       += core gui widgets

CONFIG   += c++17

# Décommentez la ligne suivante pour que la compilation échoue si vous utilisez
# une API Qt dépréciée avant Qt 6.0.0
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

SOURCES += \
    main.cpp \
    gsmartpetcare.cpp \
    gestionservices.cpp \
    historiqueservices.cpp \
    statistiquesservices.cpp \
    donutchart.cpp \
<<<<<<< Updated upstream
    barchart.cpp
=======
    barchart.cpp \
    linechart.cpp \
    statistiquesdialog.cpp \
    gestionemployes.cpp \
    statistiquesemployes.cpp \
    connexion.cpp \
    privileges.cpp \
    faceauth.cpp \
    facedialog.cpp \
    faceengine.cpp
>>>>>>> Stashed changes

HEADERS += \
    gsmartpetcare.h \
    gestionservices.h \
    historiqueservices.h \
    statistiquesservices.h \
    donutchart.h \
    barchart.h \
<<<<<<< Updated upstream
    donnees.h
=======
    linechart.h \
    donnees.h \
    statistiquesdialog.h \
    gestionemployes.h \
    statistiquesemployes.h \
    connexion.h \
    privileges.h \
    faceauth.h \
    facedialog.h \
    faceengine.h \
    donneesemployes.h
>>>>>>> Stashed changes

# Un seul fichier .ui pour toute l'application (services, rendez-vous, employés)
FORMS += \
    gsmartpetcare.ui

RESOURCES += \
    resources.qrc

# --- Module employés : reconnaissance faciale (OpenCV >= 4.5.4), OPTIONNELLE -------------------
# Sans OpenCV l'application se compile et fonctionne ; seul le bouton « Reconnaissance faciale »
# est inactif. Les deux modèles .onnx (YuNet + SFace) vont dans le dossier models/
# (voir models/LISEZMOI.md).
DEFINES += FACE_MODELS_DIR=\\\"$$PWD/models\\\"

# Linux / macOS : installez libopencv-dev (ou brew install opencv), c'est détecté automatiquement.
unix {
    CONFIG += link_pkgconfig
    packagesExist(opencv4) {
        PKGCONFIG += opencv4
        DEFINES += HAVE_OPENCV
    }
}
# Windows : décommentez et adaptez ces lignes (OpenCV compilé avec le même compilateur que Qt).
# OPENCV_DIR = C:/opencv/build
# INCLUDEPATH += $$OPENCV_DIR/include
# LIBS += -L$$OPENCV_DIR/x64/vc16/lib -lopencv_world480
# DEFINES += HAVE_OPENCV

# Règles de déploiement par défaut
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

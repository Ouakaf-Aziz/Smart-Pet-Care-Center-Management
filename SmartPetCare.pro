QT       += core gui widgets sql
CONFIG   += console

CONFIG   += c++17

# Décommentez la ligne suivante pour que la compilation échoue si vous utilisez
# une API Qt dépréciée avant Qt 6.0.0
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

SOURCES += \
    employe.cpp \
    connection.cpp \
    main.cpp \
    gsmartpetcare.cpp

HEADERS += \
    employe.h \
    connection.h \
    gsmartpetcare.h

# Un seul fichier .ui pour toute l'application (seul le module employés a du code ; services, rendez-vous et animaux : interface seule)
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

# Windows : OpenCV 4.10.0 compilé avec MinGW 13.1.0 (le même compilateur que Qt)
win32 {
    OPENCV_DIR = C:/opencv4/opencv-4.10.0/build/install
    exists($$OPENCV_DIR/include) {
        INCLUDEPATH += $$OPENCV_DIR/include
        LIBS += -L$$OPENCV_DIR/x64/mingw/lib \
                -lopencv_core4100 \
                -lopencv_imgproc4100 \
                -lopencv_imgcodecs4100 \
                -lopencv_videoio4100 \
                -lopencv_objdetect4100 \
                -lopencv_dnn4100 \
                -lopencv_calib3d4100 \
                -lopencv_features2d4100 \
                -lopencv_flann4100
        DEFINES += HAVE_OPENCV
    }
}

# Règles de déploiement par défaut
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
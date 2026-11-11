QT       += core gui widgets sql network
CONFIG   += console
CONFIG   += c++17

SOURCES += \
    employe.cpp \
    main.cpp \
    connection.cpp \
    gsmartpetcare.cpp \
    RendezVous.cpp \
    service.cpp \
    animal.cpp

HEADERS += \
    employe.h \
    connection.h \
    gsmartpetcare.h \
    RendezVous.h \
    service.h \
    animal.h

FORMS += \
    gsmartpetcare.ui

RESOURCES += \
    resources.qrc

# --- Optional face recognition support ---------------------------------------
DEFINES += FACE_MODELS_DIR=\\\"$$PWD/models\\\"

unix {
    CONFIG += link_pkgconfig
    packagesExist(opencv4) {
        PKGCONFIG += opencv4
        DEFINES += HAVE_OPENCV
    }
}

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

QT       += core gui widgets sql network
CONFIG   += console
CONFIG   += c++17

SOURCES += \
    main.cpp \
    connection.cpp \
    gsmartpetcare.cpp \
    RendezVous.cpp \
    service.cpp \
    animal.cpp

HEADERS += \
    connection.h \
    gsmartpetcare.h \
    RendezVous.h \
    service.h \
    animal.h

FORMS += \
    gsmartpetcare.ui

RESOURCES += \
    resources.qrc

# Règles de déploiement par défaut
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

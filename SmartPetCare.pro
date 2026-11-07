QT       += core gui widgets sql
CONFIG   += console
CONFIG   += c++17

SOURCES += \
    main.cpp \
    connection.cpp \
    gsmartpetcare.cpp \
    RendezVous.cpp

HEADERS += \
    connection.h \
    gsmartpetcare.h \
    RendezVous.h

FORMS += \
    gsmartpetcare.ui

RESOURCES += \
    resources.qrc

# Règles de déploiement par défaut
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

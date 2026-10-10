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
    barchart.cpp

HEADERS += \
    gsmartpetcare.h \
    gestionservices.h \
    historiqueservices.h \
    statistiquesservices.h \
    donutchart.h \
    barchart.h \
    donnees.h

FORMS += \
    gsmartpetcare.ui

RESOURCES += \
    resources.qrc

# Règles de déploiement par défaut
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

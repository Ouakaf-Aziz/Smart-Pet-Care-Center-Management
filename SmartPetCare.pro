QT += core gui widgets sql network
CONFIG += c++17

SOURCES += \
    main.cpp \
    gsmartpetcare.cpp \
    service.cpp \
    connection.cpp

HEADERS += \
    gsmartpetcare.h \
    service.h \
    connection.h

FORMS += gsmartpetcare.ui
RESOURCES += resources.qrc

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

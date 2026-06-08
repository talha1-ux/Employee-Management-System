QT += core gui widgets printsupport multimedia multimediawidgets concurrent

CONFIG += c++17

TARGET   = AdvancedEMS
TEMPLATE = app

SOURCES += \
    main.cpp \
    gui/LoginWindow.cpp \
    gui/Dashboard.cpp \
    gui/FingerprintDialog.cpp \
    gui/FaceIdDialog.cpp \
    lib/Employee.cpp \
    lib/Database.cpp

HEADERS += \
    gui/LoginWindow.h \
    gui/Dashboard.h \
    gui/FingerprintDialog.h \
    gui/FaceIdDialog.h \
    lib/User.h \
    lib/UserDatabase.h \
    lib/Employee.h \
    lib/Manager.h \
    lib/Database.h

INCLUDEPATH += lib gui

# No external dependencies — Qt6 only.
# Modules used: Widgets, Multimedia, MultimediaWidgets, Concurrent, PrintSupport

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

QT += quick
CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp

RESOURCES += qml.qrc

INCLUDEPATH += D:/workspace/qtghost/
#win32:LIBS += D:/workspace/build-qtghost-Desktop_Qt_5_10_1_MinGW_32bit-Debug/debug/qtghost.dll

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../build-qtghost-Desktop_Qt_5_10_1_MinGW_32bit-Debug/release/ -lqtghost
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../build-qtghost-Desktop_Qt_5_10_1_MinGW_32bit-Debug/debug/ -lqtghost
else:unix: LIBS += -L$$PWD/../build-qtghost-Desktop_Qt_5_10_1_MinGW_32bit-Debug/ -lqtghost

INCLUDEPATH += $$PWD/../qtghost
DEPENDPATH += $$PWD/../qtghost

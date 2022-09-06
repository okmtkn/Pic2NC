QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/cuttingpoint.cpp \
    src/imageprocessing.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/ncdata.cpp \
    src/scannertwain.cpp

HEADERS += \
    src/cuttingpoint.h \
    src/imageprocessing.h \
    src/mainwindow.h \
    src/ncdata.h \
    src/scannertwain.h \
    src/twain.h

FORMS += \
    mainwindow.ui


INCLUDEPATH += C:\OpenCV\opencv4.6.0\build_vs15\include
#LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_calib3d460.dll.a
LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_core460.dll.a
#LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_dnn460.dll.a
#LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_features2d460.dll.a
#LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_flann460.dll.a
#LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_highgui460.dll.a
#LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_imgcodecs460.dll.a
LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_imgproc460.dll.a
#LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_ml460.dll.a
#LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_objdetect460.dll.a
#LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_photo460.dll.a
#LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_stitching460.dll.a
#LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_ts460.a
#LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_video460.dll.a
#LIBS += C:\OpenCV\opencv4.6.0\release\lib\libopencv_videoio460.dll.a

RC_ICONS = resource/pic2nc.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    CHANGELOG.md

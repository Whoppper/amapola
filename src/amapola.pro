
QT       += core gui multimedia multimediawidgets xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = amapola
TEMPLATE = app

include(SubtitlesParsers/SubtitlesParsers.pri)
include(Tools/Tools.pri)

SOURCES += main.cpp\
    mainwindow.cpp \
    videoplayer.cpp \
    waveform.cpp \
    subtitlestable.cpp \
    filereader.cpp \
    subtitles.cpp \
    filewriter.cpp \
    settings.cpp \
    attributesconverter.cpp \
    subtitlefileparser.cpp \
    textedit.cpp \
    waveformslider.cpp \
    application.cpp \
    amapolaprojfileparser.cpp \
    subexportdialog.cpp \
    subimportmanager.cpp \
    imagesexporter.cpp \
    inputsizedialog.cpp \

HEADERS += mainwindow.h \
    videoplayer.h \
    waveform.h \
    subtitlestable.h \
    filereader.h \
    subtitles.h \
    filewriter.h \
    settings.h \
    attributesconverter.h \
    subtitlefileparser.h \
    textedit.h \
    waveformslider.h \
    application.h \
    amapolaprojfileparser.h \
    subexportdialog.h \
    subimportmanager.h \
    imagesexporter.h \
    inputsizedialog.h

FORMS += mainwindow.ui \
    waveform.ui \
    videoplayer.ui \
    settings.ui \
    subexportdialog.ui

CONFIG += qwt

QWT_DIR = $$PWD/../lib/qwt
INCLUDEPATH +=  $${QWT_DIR}/src/
LIBS += -L$${QWT_DIR}/lib -lqwt

OTHER_FILES += \
    ../lib/README.md


TRANSLATIONS = ../locale/fr_FR.ts \
    ../locale/es_ES.ts


unix {
    isEmpty(PREFIX) {
        PREFIX = /usr
    }
    DATADIR =$${PREFIX}/share
    SHAREDIR = $${DATADIR}/$${TARGET}
    TRANSLATIONS_PATH = $${SHAREDIR}/translations

    INSTALLS += target translations

    translations.path = $${TRANSLATIONS_PATH}
    translations.files = ../locale/*.qm
}

win32 {
    TRANSLATIONS_PATH = locale
}

mac {
}

TRANSLATIONS_PATH_STR = '\\"$${TRANSLATIONS_PATH}\\"'
DEFINES += TRANSLATIONS_PATH=\"$${TRANSLATIONS_PATH_STR}\"

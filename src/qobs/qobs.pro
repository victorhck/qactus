QT += core network
TARGET = qobs
TEMPLATE = lib
DEFINES += QOBS_LIBRARY
include(../defines.pri)

SOURCES += obsaccess.cpp \
    obsxmlreader.cpp \
    obsrequest.cpp \
    obs.cpp \
    obsfile.cpp \
    obsresult.cpp \
    obsrevision.cpp \
    obsstatus.cpp \
    obsabout.cpp \
    obsxmlwriter.cpp
HEADERS += obsaccess.h \
    obsxmlreader.h \
    obsrequest.h \
    obs.h \
    obsfile.h \
    obsresult.h \
    obsrevision.h \
    obsstatus.h \
    obsabout.h \
    obsxmlwriter.h

unix {
    contains(QMAKE_HOST.arch, x86_64) {
        target.path = $$PREFIX/lib64
    } else {
        target.path = $$PREFIX/lib
    }
}
INSTALLS += target


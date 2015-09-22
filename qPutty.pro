include(qPutty.pri)
include(linkHack.pri)
QT += widgets
TEMPLATE = app
DEPENDPATH *= .
isEmpty(VERSION) {
    VERSION=$$system(git describe --tags)
}
isEmpty(PUTTY_SRC_VERSION){
    PUTTY_SRC_VERSION=$$system(git submodule status $$PUTTY_HOME | sed \'s/^.*(//;s/)$//\')
}
DEFINES *= VERSION=\\\"$$VERSION\\\" PUTTY_VERSION=\\\"$$PUTTY_SRC_VERSION\\\"

CONFIG(debug,debug|release) {
    message("MODE: Debug")
}
CONFIG(release,debug|release) {
    message("MODE: Release")
    CONFIG *= silent warn_off
}

SOURCES *= putty.cpp

isEmpty(PREFIX){
    PREFIX=/usr/local
}

unix {
    desktopEntry.path = $$PREFIX/share/applications/
    desktopEntry.files = qPutty.desktop

    target.path = $$PREFIX/bin
    QT+=x11extras
    INSTALLS+=target desktopEntry
}

macx {
    ICON = putty-32.png
    QMAKE_PKGINFO_TYPEINFO = qtty
    QMAKE_INFO_PLIST = Info.plist
    QMAKE_EXTRA_TARGETS*=patchInfoPlist
    POST_TARGETDEPS*=patchInfoPlist
    SHORT_VERSION=$$sprintf(%1.%2,$$VERSION,$$PUTTY_SRC_VERSION)
    APP=$$sprintf(%1.app,$$TARGET)
    patchInfoPlist.depends=$$APP/Contents/$$QMAKE_INFO_PLIST
    patchInfoPlist.commands=$(SED) -e \'s/@SHORT_VERSION@/$$SHORT_VERSION/g\' -i \"\"  $<
}

message($$TARGET "with PUTTY_HOME="$$PUTTY_HOME" GEN_SBCSDAT="$$GEN_SBCSDAT" KERBEROS="$$KERBEROS" GSSGLUE="$$GSSGLUE)

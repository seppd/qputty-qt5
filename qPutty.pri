#DEFINES *= OPTIMISE_SCROLL

isEmpty(QPUTTY_HOME){
    QPUTTY_HOME=.
}
isEmpty(PUTTY_HOME){
    PUTTY_HOME=./putty
}
isEmpty(KERBEROS){
    KERBEROS=yes
    isEmpty(GSSGLUE){
        macx {
            GSSGLUE=no
            DEFINES *= STATIC_GSSAPI NO_LIBDL
        } else {
            GSSGLUE=yes
        }
    }
}
isEmpty(GEN_SBCSDAT){
    GEN_SBCSDAT=yes
}
isEmpty(PUTTY_RELEASE){
    PUTTY_RELEASE=no
}

RESOURCES *= $$QPUTTY_HOME/qPutty.qrc
INCLUDEPATH *= $$QPUTTY_HOME $$PUTTY_HOME/./ $$PUTTY_HOME/charset/
SOURCES *= $$PUTTY_HOME/be_all_s.c \
        $$PUTTY_HOME/be_misc.c \
        $$PUTTY_HOME/callback.c \
        $$PUTTY_HOME/cmdline.c \
        $$PUTTY_HOME/conf.c \
        $$PUTTY_HOME/config.c \
        $$PUTTY_HOME/cproxy.c \
        $$PUTTY_HOME/dialog.c \
        $$PUTTY_HOME/ldisc.c \
        $$PUTTY_HOME/ldiscucs.c \
        $$PUTTY_HOME/logging.c \
        $$PUTTY_HOME/minibidi.c \
        $$PUTTY_HOME/misc.c \
        $$PUTTY_HOME/pinger.c \
        $$PUTTY_HOME/portfwd.c \
        $$PUTTY_HOME/proxy.c \
        $$PUTTY_HOME/raw.c \
        $$PUTTY_HOME/rlogin.c \
        $$PUTTY_HOME/sercfg.c \
        $$PUTTY_HOME/settings.c \
        $$PUTTY_HOME/ssh.c \
        $$PUTTY_HOME/sshaes.c \
        $$PUTTY_HOME/ssharcf.c \
        $$PUTTY_HOME/sshbcrypt.c \
        $$PUTTY_HOME/sshblowf.c \
        $$PUTTY_HOME/sshbn.c \
        $$PUTTY_HOME/sshccp.c \
        $$PUTTY_HOME/sshcrc.c \
        $$PUTTY_HOME/sshcrcda.c \
        $$PUTTY_HOME/sshdes.c \
        $$PUTTY_HOME/sshdh.c \
        $$PUTTY_HOME/sshdss.c \
        $$PUTTY_HOME/sshecc.c \
        $$PUTTY_HOME/sshmd5.c \
        $$PUTTY_HOME/sshpubk.c \
        $$PUTTY_HOME/sshrand.c \
        $$PUTTY_HOME/sshrsa.c \
        $$PUTTY_HOME/sshsh256.c \
        $$PUTTY_HOME/sshsh512.c \
        $$PUTTY_HOME/sshsha.c \
        $$PUTTY_HOME/sshshare.c \
        $$PUTTY_HOME/sshzlib.c \
        $$PUTTY_HOME/telnet.c \
        $$PUTTY_HOME/terminal.c \
        $$PUTTY_HOME/timing.c \
        $$PUTTY_HOME/tree234.c \
        $$PUTTY_HOME/version.c \
        $$PUTTY_HOME/wcwidth.c \
        $$PUTTY_HOME/wildcard.c \
        $$PUTTY_HOME/x11fwd.c \
        $$PUTTY_HOME/import.c \
        $$PUTTY_HOME/charset/fromucs.c \
        $$PUTTY_HOME/charset/macenc.c \
        $$PUTTY_HOME/charset/localenc.c \
        $$PUTTY_HOME/charset/mimeenc.c \
        $$PUTTY_HOME/charset/sbcs.c \
        $$PUTTY_HOME/charset/toucs.c \
        $$PUTTY_HOME/charset/slookup.c \
        $$PUTTY_HOME/charset/utf8.c \
        $$PUTTY_HOME/charset/xenc.c

win32{
    INCLUDEPATH *= $$PUTTY_HOME/windows
    LIBS *= -lcomctl32 -luser32 -ladvapi32 -lcomdlg32 -lgdi32 -lshell32 -lwinspool -lkernel32 -lole32 -loleaut32 -luuid -lodbc32 -lodbccp32
    DEFINES -= UNICODE
    DEFINES *= _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE _CRT_SECURE_NO_WARNINGS _WINDOWS SECURITY_WIN32
    SOURCES *= $$PUTTY_HOME/windows/winmisc.c \
            $$PUTTY_HOME/windows/windefs.c \
            $$PUTTY_HOME/windows/wintime.c \
            $$PUTTY_HOME/windows/wincfg.c \
            $$PUTTY_HOME/windows/winnet.c \
            $$PUTTY_HOME/windows/winnoise.c \
            $$PUTTY_HOME/windows/winprint.c \
            $$PUTTY_HOME/windows/winproxy.c \
            $$PUTTY_HOME/windows/winser.c \
            $$PUTTY_HOME/windows/winstore.c \
            $$PUTTY_HOME/windows/winucs.c \
            $$PUTTY_HOME/windows/winhandl.c \
            $$PUTTY_HOME/windows/winhelp.c \
            $$PUTTY_HOME/windows/winpgntc.c \
            $$PUTTY_HOME/windows/winshare.c \
            $$PUTTY_HOME/windows/winutils.c

    contains(PUTTY_RELEASE,yes){
        DEFINES *= PUTTY_RELEASE
        KERBEROS=no
    } else {
        SOURCES *= $$PUTTY_HOME/windows/winx11.c \
                   $$PUTTY_HOME/windows/winjump.c
        contains(KERBEROS,yes){
            SOURCES *= $$PUTTY_HOME/windows/wingss.c \
                       $$PUTTY_HOME/sshgssc.c \
                       $$PUTTY_HOME/pgssapi.c 
        } else {
            DEFINES *= NO_GSSAPI
            SOURCES *= $$PUTTY_HOME/nogss.c 
        }
    }
} else {
    INCLUDEPATH *= $$PUTTY_HOME/unix
    LIBS *= -ldl -lX11
    QMAKE_CXXFLAGS *= -Wno-write-strings
    SOURCES *= $$PUTTY_HOME/time.c \ 
            $$PUTTY_HOME/unix/ux_x11.c \
            $$PUTTY_HOME/unix/uxagentc.c \
            $$PUTTY_HOME/unix/uxcfg.c \
            $$PUTTY_HOME/unix/uxmisc.c \
            $$PUTTY_HOME/unix/uxnet.c \
            $$PUTTY_HOME/unix/uxnoise.c \
            $$PUTTY_HOME/unix/uxprint.c \
            $$PUTTY_HOME/unix/uxproxy.c \
            $$PUTTY_HOME/unix/uxsel.c \
            $$PUTTY_HOME/unix/uxser.c \
            $$PUTTY_HOME/unix/uxsignal.c \
            $$PUTTY_HOME/unix/uxstore.c \
            $$PUTTY_HOME/unix/uxucs.c \
            $$PUTTY_HOME/unix/xkeysym.c \
            $$PUTTY_HOME/unix/xpmpucfg.c \
            $$PUTTY_HOME/unix/xpmputty.c \
            $$PUTTY_HOME/unix/gtkcfg.c \
            $$PUTTY_HOME/unix/uxpeer.c \
            $$PUTTY_HOME/unix/uxshare.c \
            $$QPUTTY_HOME/uxputty.c

    contains(PUTTY_RELEASE,yes){
        DEFINES *= PUTTY_RELEASE
        KERBEROS=no
    } else {
        contains(KERBEROS,yes){
            contains(GSSGLUE,yes) {
                LIBS *= -lgssglue
                QMAKE_INCDIR=/usr/include/gssglue
            } else {
                LIBS *= -lgssapi_krb5
            }
            SOURCES *= $$PUTTY_HOME/unix/uxgss.c \
                       $$PUTTY_HOME/sshgssc.c \
                       $$PUTTY_HOME/pgssapi.c 
        } else {
            KERBEROS=no
            DEFINES *= NO_GSSAPI
            SOURCES *= $$PUTTY_HOME/nogss.c 
        }
    }
}

contains(GEN_SBCSDAT,yes) {
    include($$QPUTTY_HOME/sbcsdat.pri)
}

# Input
FORMS   *= $$QPUTTY_HOME/eventLog.ui\
           $$QPUTTY_HOME/passphrase.ui
HEADERS *= $$QPUTTY_HOME/terminalCursor.h \
           $$QPUTTY_HOME/terminalText.h \
           $$QPUTTY_HOME/qPutty.h \
           $$QPUTTY_HOME/configWidget.h \
           $$QPUTTY_HOME/configDialog.h \
           $$QPUTTY_HOME/puttyGridLayout.h \
           $$QPUTTY_HOME/abstractTerminalWidget.h \
           $$QPUTTY_HOME/terminalWidget.h \
           $$QPUTTY_HOME/diffEvent.h \
           $$QPUTTY_HOME/structs.h 
SOURCES *= $$QPUTTY_HOME/terminalCursor.cpp \
           $$QPUTTY_HOME/terminalText.cpp \
           $$QPUTTY_HOME/terminalWidget.cpp \
           $$QPUTTY_HOME/utils.c \
           $$QPUTTY_HOME/qPutty.cpp \
           $$QPUTTY_HOME/qtdlg.cpp \
           $$QPUTTY_HOME/qtwin.cpp \
           $$QPUTTY_HOME/puttyGridLayout.cpp \
           $$QPUTTY_HOME/configWidget.cpp \
           $$QPUTTY_HOME/diffEvent.cpp \
           $$QPUTTY_HOME/configDialog.cpp 

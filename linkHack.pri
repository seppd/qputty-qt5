# hack so that putty can be linked
isEmpty(OBJECTS_DIR) {
    win32 {
        CONFIG(debug,debug|release) {
            OBJECTS_DIR=debug
        }
        CONFIG(release,debug|release) {
            OBJECTS_DIR=release
        }
    } else {
        OBJECTS_DIR=.
    }
}

TARGETS=$$OBJECTS_DIR/sbcsdat$$QMAKE_EXT_OBJ
!contains(TEMPLATE,vcapp) {
    TARGETS+=$$OBJECTS_DIR/utf8$$QMAKE_EXT_OBJ
}

win32 {
    OBJECTS_DIR=($$replace(OBJECTS_DIR,/,\))
    TARGETS=($$replace(TARGETS,/,\))
}

QMAKE_EXTRA_TARGETS*=$$TARGETS
PRE_TARGETDEPS*=$$TARGETS
QMAKE_LFLAGS*=$$TARGETS
QMAKE_CLEAN*=$$TARGETS

sbcsdat_o_dep=$$PUTTY_HOME/charset/sbcsdat.c $$PUTTY_HOME/charset/charset.h $$PUTTY_HOME/charset/internal.h

win32 {
    $$OBJECTS_DIR\sbcsdat.obj.depends=$$sbcsdat_o_dep
} else {
    $$OBJECTS_DIR/sbcsdat.o.depends=$$sbcsdat_o_dep
    $$OBJECTS_DIR/sbcsdat.o.commands=$(CC) -c $(CFLAGS) $(INCPATH) -o $$OBJECTS_DIR/sbcsdat$$QMAKE_EXT_OBJ $$PUTTY_HOME/charset/sbcsdat.c
}


utf8_o_dep=$$PUTTY_HOME/charset/utf8.c $$PUTTY_HOME/charset/charset.h $$PUTTY_HOME/charset/internal.h

win32 {
    $$OBJECTS_DIR\utf8.obj.depends=$$utf8_o_dep
} else {
    $$OBJECTS_DIR/utf8.o.depends=$$utf8_o_dep
    $$OBJECTS_DIR/utf8.o.commands=$(CC) -c $(CFLAGS) $(INCPATH) -o $$OBJECTS_DIR/utf8$$QMAKE_EXT_OBJ $$PUTTY_HOME/charset/utf8.c
}

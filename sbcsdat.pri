win32 {
    QMAKE_EXTRA_TARGETS*=$$PUTTY_HOME\charset\sbcsdat.c
    $$PUTTY_HOME\charset\sbcsdat.c.depends = $$PUTTY_HOME\charset\sbcs.dat $$PUTTY_HOME\charset\sbcsgen.pl
    $$PUTTY_HOME\charset\sbcsdat.c.commands = cd $$PUTTY_HOME\charset && perl sbcsgen.pl && cd $$HOME
} else {
    QMAKE_EXTRA_TARGETS*=$$PUTTY_HOME/charset/sbcsdat.c
    $$PUTTY_HOME/charset/sbcsdat.c.depends = $$PUTTY_HOME/charset/sbcs.dat $$PUTTY_HOME/charset/sbcsgen.pl
    $$PUTTY_HOME/charset/sbcsdat.c.commands = cd $$PUTTY_HOME/charset && perl sbcsgen.pl && cd $$HOME
}


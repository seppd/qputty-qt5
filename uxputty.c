/*
 *      Copyright (C) 2009 - 2011 Four J's Development Tools Europe, Ltd.
 *      http://www.fourjs.com
 *  
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
/*
 * Unix PuTTY main program.
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "putty.h"
#include "storage.h"
#include "utils.h"

/*
 * Stubs to avoid uxpty.c needing to be linked in.
 */
const int use_pty_argv = FALSE;
char **pty_argv;		       /* never used */

/*
 * Clean up and exit.
 */
void cleanup_exit(int code)
{
    /*
     * Clean up.
     */
    sk_cleanup();
    random_save_seed();
    exit(code);
}

#ifdef PUTTY_RELEASE
const char platform_x11_best_transport[] = "unix";
#endif

Backend *select_backend(Conf *conf)
{
#ifdef PUTTY_RELEASE
    int i;
    Backend *back = NULL;
    for (i = 0; backends[i].backend != NULL; i++)
        if (backends[i].protocol == conf_get_int(conf, CONF_protocol)) {
            back = backends[i].backend;
            break;
        }
#else
    Backend *back = backend_from_proto(conf_get_int(conf, CONF_protocol));
#endif
    assert(back != NULL);
    return back;
}

int cfgbox(Conf *conf)
{
    char *title = dupcat(appname, " Configuration", NULL);
    int ret = do_config_box(title, conf, 0, 0);
    sfree(title);
    return ret;
}

static int got_host = 0;

const int use_event_log = 1, new_session = 1, saved_sessions = 1;

int process_nonoption_arg(const char *arg, Conf *conf, int *allow_launch)
{
    char *p, *q = arg;

    if (got_host) {
        /*
         * If we already have a host name, treat this argument as a
         * port number. NB we have to treat this as a saved -P
         * argument, so that it will be deferred until it's a good
         * moment to run it.
         */
        int ret = cmdline_process_param("-P", arg, 1, conf);
        assert(ret == 2);
    } else if (!strncmp(q, "telnet:", 7)) {
        /*
         * If the hostname starts with "telnet:",
         * set the protocol to Telnet and process
         * the string as a Telnet URL.
         */
        char c;

        q += 7;
        if (q[0] == '/' && q[1] == '/')
            q += 2;
        conf_set_int(conf, CONF_protocol, PROT_TELNET);
        p = q;
        while (*p && *p != ':' && *p != '/')
            p++;
        c = *p;
        if (*p)
            *p++ = '\0';
        if (c == ':')
            conf_set_int(conf, CONF_port, atoi(p));
        else
            conf_set_int(conf, CONF_port, -1);
        conf_set_str(conf, CONF_host, q);
        got_host = 1;
    } else {
        got_host=handleHostnameCmdlineParam(arg,conf);
    }
    if (got_host)
	*allow_launch = TRUE;
    return 1;
}

/*
 * X11-forwarding-related things suitable for Gtk app.
 */

char *platform_get_x_display(void) {
    const char *display = getenv("DISPLAY");
    return dupstr(display);
}

const int share_can_be_downstream = TRUE;
const int share_can_be_upstream = TRUE;

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
 * qtwin.c: the main code that runs a PuTTY terminal emulator and
 * backend in a QTextEdit
 */

#define KEY_DEBUGGING

#define PUTTY_DO_GLOBALS	       /* actually _define_ globals */
#define CAT2(x,y) x ## y
#define CAT(x,y) CAT2(x,y)
#define ASSERT(x) enum {CAT(assertion_,__LINE__) = 1 / (x)}

#include "structs.h"

#ifdef Q_OS_WIN
typedef WId WID;
#endif

#ifdef Q_OS_MAC
#define DEFAULT_FONT "Monaco,10"
typedef long WID;
#endif

#ifdef Q_OS_UNIX
#include <QX11Info>
#define DEFAULT_FONT "Courier New,10"
typedef long WID;
#endif

#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QMessageBox>
#include <QApplication>
#include "qPutty.h"
#include "configDialog.h"
extern "C" {
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include "utils.h"
#ifndef Q_OS_WIN
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
extern int use_pty_argv;
#else
    char *do_select(SOCKET skt, int startup);
    void write_aclip(void *frontend, char *data, int len, int must_deselect);
    void cleanup_exit(int code);
    Backend *select_backend(Config *cfg);
    #define WM_IGNORE_CLIP (WM_APP + 2)
    const int use_pty_argv = FALSE;
#endif

#ifdef Q_OS_WIN
extern "C++" void logevent_dlg(void* estuff,const char* string);
#endif

int pt_main(int argc, char **argv);




char **pty_argv;	       /* declared in pty.c */


/*
 * Timers are global across all sessions (even if we were handling
 * multiple sessions, which we aren't), so the current timer ID is
 * a global variable.
 */
struct draw_ctx {
    struct gui_data *inst;
};

char *gdk_get_display();
extern Backend *select_backend(Conf *conf);
char *x_get_default(const char *)
{
    return 0;
}
}
QPutty* qPutty; // will define at QPutty::QPutty()

QString app_name = "pterm";
void start_backend(struct gui_data *inst);


WID get_windowid(void *)
{
    return qPutty->winId();
}

void connection_fatal(void *frontend, const char *p, ...)
{
    struct gui_data *inst = (struct gui_data *)frontend;
    va_list ap;
    char *msg;
    va_start(ap, p);
    msg = dupvprintf(p, ap);
    va_end(ap);
    inst->exited = TRUE;
    QMessageBox::critical(qPutty,"Fatal error",msg);
    sfree(msg);
    if (conf_get_int(inst->conf, CONF_close_on_exit) == FORCE_ON)
        cleanup_exit(1);
}

#ifndef Q_OS_WIN
/*
 * Default settings that are specific to pterm.
 */
FontSpec *platform_default_fontspec(const char *name)
{
    if (!strcmp(name, "Font"))
        return fontspec_new(DEFAULT_FONT);
    else
        return fontspec_new("");
}

Filename *platform_default_filename(const char *name)
{
    if (!strcmp(name, "LogFileName"))
        return filename_from_str("putty.log");
    else
        return filename_from_str("");
}

char *platform_default_s(const char *name)
{
    if (!strcmp(name, "SerialLine"))
        return dupstr("/dev/ttyS0");
    return NULL;
}

int platform_default_i(const char *name, int def)
{
    if (!strcmp(name, "CloseOnExit"))
        return 2;  /* maps to FORCE_ON after painful rearrangement :-( */
    if (!strcmp(name, "WinNameAlways"))
        return 0;  /* X natively supports icon titles, so use 'em by default */
    return def;
}
#endif

void frontend_echoedit_update(void *frontend, int echo, int edit)
{
    /*
     * This is a stub in pterm. If I ever produce a Unix
     * command-line ssh/telnet/rlogin client (i.e. a port of plink)
     * then it will require some termios manoeuvring analogous to
     * that in the Windows plink.c, but here it's meaningless.
     */
}

char *get_ttymode(void *frontend, const char *mode)
{
    struct gui_data *inst = (struct gui_data *)frontend;
    return term_get_ttymode(inst->term, mode);
}

int from_backend(void *frontend, int is_stderr, const char *data, int len)
{
    struct gui_data *inst = (struct gui_data *)frontend;
    return term_data(inst->term, is_stderr, data, len);
}

int from_backend_untrusted(void *frontend, const char *data, int len)
{
    struct gui_data *inst = (struct gui_data *)frontend;
    return term_data_untrusted(inst->term, data, len);
}

int get_userpass_input(prompts_t *p, const unsigned char *in, int inlen)
{
    struct gui_data *inst = (struct gui_data *)p->frontend;
    int ret;
    ret = cmdline_get_passwd_input(p, in, inlen);
    if (ret == -1)
	ret = term_get_userpass_input(inst->term, p, in, inlen);
    return ret;
}

void logevent(void *gd, const char *msg)
{
    struct gui_data *inst = (struct gui_data *)gd;
    log_eventlog(inst->logctx,msg);
    logevent_dlg(inst->eventlogstuff,msg);
}

int font_dimension(void *frontend, int which)/* 0 for width, 1 for height */
{
    struct gui_data *inst = (struct gui_data *)frontend;

    if (which)
	return inst->font_height;
    else
	return inst->font_width;
}

/*
 * Translate a raw mouse button designation (LEFT, MIDDLE, RIGHT)
 * into a cooked one (SELECT, EXTEND, PASTE).
 * 
 * In Unix, this is not configurable; the X button arrangement is
 * rock-solid across all applications, everyone has a three-button
 * mouse or a means of faking it, and there is no need to switch
 * buttons around at all.
 */
#if 0
static Mouse_Button translate_button(Mouse_Button button)
{
    /* struct gui_data *inst = (struct gui_data *)frontend; */

    if (button == MBT_LEFT)
	return MBT_SELECT;
    if (button == MBT_MIDDLE)
	return MBT_PASTE;
    if (button == MBT_RIGHT)
	return MBT_EXTEND;
    return 0;			       /* shouldn't happen */
}
#endif

void *get_window(void *)
{
    return 0;
}

/*
 * Minimise or restore the window in response to a server-side
 * request.
 */
void set_iconic(void *, int )
{
/*
#if GTK_CHECK_VERSION(2,0,0)
    struct gui_data *inst = (struct gui_data *)frontend;
    if (iconic)
	gtk_window_iconify(GTK_WINDOW(inst->window));
    else
	gtk_window_deiconify(GTK_WINDOW(inst->window));
#endif
*/
}

/*
 * Move the window in response to a server-side request.
 */
void move_window(void *, int , int )
{
}

/*
 * Move the window to the top or bottom of the z-order in response
 * to a server-side request.
 */
void set_zorder(void *, int )
{
}

/*
 * Refresh the window in response to a server-side request.
 */
void refresh_window(void *frontend)
{
    struct gui_data *inst = (struct gui_data *)frontend;
    term_invalidate(inst->term);
}

/*
 * Maximise or restore the window in response to a server-side
 * request.
 */
void set_zoomed(void *, int )
{
}

/*
 * Report whether the window is iconic, for terminal reports.
 */
int is_iconic(void *)
{
    return 0;
}

/*
 * Report the window's position, for terminal reports.
 */
void get_window_pos(void *, int *, int *)
{
}

/*
 * Report the window's pixel size, for terminal reports.
 */
void get_window_pixels(void *, int *, int *)
{
}

/*
 * Return the window or icon title.
 */
char *get_window_title(void *frontend, int icon)
{
    struct gui_data *inst = (struct gui_data *)frontend;
    return icon ? inst->icontitle : inst->wintitle;
}


void draw_backing_rect(struct gui_data *)
{
}


void frontend_keypress(void *handle)
{
    struct gui_data *inst = (struct gui_data *)handle;

    /*
     * If our child process has exited but not closed, terminate on
     * any keypress.
     */
    if (inst->exited)
        exit(0);
}

void notify_remote_exit(void *frontend)
{
    struct gui_data *inst = (struct gui_data *)frontend;
    int exitcode;

    if (!inst->exited && (exitcode = inst->back->exitcode(inst->backhandle)) >= 0) 
    {
	inst->exited = TRUE;
    if (conf_get_int(inst->conf, CONF_close_on_exit) == FORCE_ON ||
            (conf_get_int(inst->conf, CONF_close_on_exit) == AUTO &&
             exitcode != INT_MAX))
        {
            qPutty->close(exitcode);
        }
        else
        {
            if(exitcode!=INT_MAX)
            {
                logevent(inst,"Connection closed by remote host");
            }
        }
    }
}

void timer_change_notify(unsigned long next)
{
    long ticks;

    ticks = next - GETTICKCOUNT();
    if (ticks <= 0)
        ticks = 1;

    qPutty->timerChangeNotify(ticks,next);
}

#ifndef Q_OS_WIN
uxsel_id *uxsel_input_add(int fd, int rwx) {
    return qPutty->registerFd(fd,rwx);
}

void uxsel_input_remove(uxsel_id *id) {
    qPutty->releaseFd(id);
}
#endif

void set_busy_status(void *frontend, int status)
{
    struct gui_data *inst = (struct gui_data *)frontend;
    inst->busy_status = status;
}

int frontend_is_utf8(void *frontend)
{
    struct gui_data *inst = (struct gui_data *)frontend;
    return inst->ucsdata.line_codepage == CS_UTF8;
}

/*
 * set or clear the "raw mouse message" mode
 */
void set_raw_mouse_mode(void *, int )
{
}

void request_resize(void *, int , int )
{
}

void set_window_background(struct gui_data *)
{
}

void palette_set(void *frontend, int n, int , int , int )
{
    struct gui_data *inst = (struct gui_data *)frontend;
    if (n >= 16)
	n += 256 - 16;
    //if (n > NALLCOLOURS)
        //return;
    //real_palette_set(inst, n, r, g, b);
    if (n == 258) {
        /* Default Background changed. Ensure space between text area and
         * window border is redrawn */
        set_window_background(inst);
        //draw_backing_rect(inst);
        //gtk_widget_queue_draw(inst->area);
    }
}

void palette_reset(void *frontend)
{
    struct gui_data *inst = (struct gui_data *)frontend;
    /* This maps colour indices in inst->cfg to those used in inst->cols. */
    static const int ww[] = {
        256, 257, 258, 259, 260, 261,
        0, 8, 1, 9, 2, 10, 3, 11,
        4, 12, 5, 13, 6, 14, 7, 15
    };
    //gboolean success[NALLCOLOURS];
    int i;

    assert(lenof(ww) == NCFGCOLOURS);

    for (i = 0; i < (int)NCFGCOLOURS; i++) {
        inst->cols[ww[i]] = QColor(conf_get_int_int(inst->conf, CONF_colours, i*3+0),
                                   conf_get_int_int(inst->conf, CONF_colours, i*3+1),
                                   conf_get_int_int(inst->conf, CONF_colours, i*3+2));
    }

    for (i = 0; i < NEXTCOLOURS; i++) {
        if (i < 216) {
            int r = i / 36, g = (i / 6) % 6, b = i % 6;
            inst->cols[i+16]=QColor(r?r*0x28+0x37:0,g?g*0x28+0x37:0,b?b*0x28+0x37:0);
        } else {
            int shade = i - 216;
            shade = shade * 0x0a + 0x08;
            inst->cols[i+16]=QColor(shade,shade,shade);
        }
    }
}

/* Ensure that all the cut buffers exist - according to the ICCCM, we must
 * do this before we start using cut buffers.
 */
void init_cutbuffers()
{
}

/* Store the data in a cut-buffer. */
void store_cutbuffer(char * , int )
{
    /* ICCCM says we must rotate the buffers before storing to buffer 0. */
}

/* Retrieve data from a cut-buffer.
 * Returned data needs to be freed with XFree().
 */
char * retrieve_cutbuffer(int * )
{
    return 0;
}

void write_clip(void *,wchar_t* data,int*,int len,int must_deselect)
{
    qPutty->setSelectedText(QString::fromWCharArray(data,len),must_deselect);
}

void request_paste(void *)
{
    qPutty->requestPaste();
}

void get_clip(void *frontend, wchar_t ** p, int *len)
{
    struct gui_data *inst = (struct gui_data *)frontend;

    if (p) {
	*p = inst->pastein_data;
	*len = inst->pastein_data_len;
    }
}

void set_title(void *frontend, char *title)
{
    struct gui_data *inst = (struct gui_data *)frontend;
    sfree(inst->wintitle);
    inst->wintitle = dupstr(title);
    qPutty->setTitle(title);
}

void set_icon(void *frontend, char *title)
{
    struct gui_data *inst = (struct gui_data *)frontend;
    sfree(inst->icontitle);
    inst->icontitle = dupstr(title);
}

void set_sbar(void *frontend, int total, int start, int page)
{
    if(conf_get_int(((struct gui_data *)frontend)->conf, CONF_scrollbar))
    {
        qPutty->updateScrollBar(total,start,page);
    }
}

void sys_cursor(void *, int , int )
{
}

/*
 * This is still called when mode==BELL_VISUAL, even though the
 * visual bell is handled entirely within terminal.c, because we
 * may want to perform additional actions on any kind of bell (for
 * example, taskbar flashing in Windows).
 */
void do_beep(void *, int mode)
{
    if (mode == BELL_DEFAULT)
        qApp->beep();
}

int char_width(Context , int )
{
    /*
     * Under X, any fixed-width font really _is_ fixed-width.
     * Double-width characters will be dealt with using a separate
     * font. For the moment we can simply return 1.
     */
    return 1;
}

Context get_ctx(void *frontend)
{
#ifdef Q_OS_WIN
    return get_windowid(0)?GetDC(get_windowid(0)):0;
#else
    struct gui_data *inst = (struct gui_data *)frontend;
    struct draw_ctx *dctx;

    dctx = snew(struct draw_ctx);
    dctx->inst = inst;
    return dctx;
#endif
}

void free_ctx(Context ctx)
{
#ifdef Q_OS_WIN
    ReleaseDC(get_windowid(0),ctx);
#else
    struct draw_ctx *dctx = (struct draw_ctx *)ctx;
    sfree(dctx);
#endif
}

/*
 * Draw a line of text in the window, at given character
 * coordinates, in given attributes.
 *
 * We are allowed to fiddle with the contents of `text'.
 */
void do_text_internal(Context , int , int , wchar_t *, int , unsigned long , int )
{
}

void do_text(Context , int x, int y, wchar_t *text, int len, unsigned long attr, int )
{
    qPutty->insertText(x,y,QString::fromWCharArray(text,len),attr);
}

void do_cursor(Context , int x, int y, wchar_t *text, int len, unsigned long attr, int )
{
    qPutty->setCursor(x,y,QString::fromWCharArray(text,len),attr);
}

void do_scroll(Context , int , int , int lines)
{
    qPutty->scroll(lines);
}

void modalfatalbox(const char *p, ...)
{
    va_list ap;
    fprintf(stderr, "FATAL ERROR: ");
    va_start(ap, p);
    vfprintf(stderr, p, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

void cmdline_error(const char *p, ...)
{
    va_list ap;
    fprintf(stderr, "%s: ", appname);
    va_start(ap, p);
    vfprintf(stderr, p, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

char *gdk_get_display()
{
#ifdef Q_OS_UNIX
    return (char*)QX11Info::display();
#else
    return 0;
#endif
}
const char *get_x_display(void *)
{
    return gdk_get_display();
}

static void help(FILE *fp) {
    if(fprintf(fp,
"pterm option summary:\n"
"\n"
"  --display DISPLAY         Specify X display to use (note '--')\n"
"  -name PREFIX              Prefix when looking up resources (default: pterm)\n"
"  -fn FONT                  Normal text font\n"
"  -fb FONT                  Bold text font\n"
#ifdef Q_OS_UNIX
"  -geometry GEOMETRY        Position and size of window (size in characters)\n"
#endif
"  -sl LINES                 Number of lines of scrollback\n"
"  -fg COLOUR, -bg COLOUR    Foreground/background colour\n"
"  -bfg COLOUR, -bbg COLOUR  Foreground/background bold colour\n"
"  -cfg COLOUR, -bfg COLOUR  Foreground/background cursor colour\n"
"  -T TITLE                  Window title\n"
"  -ut, +ut                  Do(default) or do not update utmp\n"
"  -ls, +ls                  Do(default) or do not make shell a login shell\n"
"  -sb, +sb                  Do(default) or do not display a scrollbar\n"
"  -log PATH                 Log all output to a file\n"
"  -nethack                  Map numeric keypad to hjklyubn direction keys\n"
"  -xrm RESOURCE-STRING      Set an X resource\n"
"  -e COMMAND [ARGS...]      Execute command (consumes all remaining args)\n"
	 ) < 0 || fflush(fp) < 0) {
	perror("output error");
	exit(1);
    }
}

int do_cmdline(int argc, char **argv, int do_everything, int *allow_launch,
               struct gui_data *inst, Conf *conf)
{
    int err = 0;
    char *val;

    /*
     * Macros to make argument handling easier. Note that because
     * they need to call `continue', they cannot be contained in
     * the usual do {...} while (0) wrapper to make them
     * syntactically single statements; hence it is not legal to
     * use one of these macros as an unbraced statement between
     * `if' and `else'.
     */
#define EXPECTS_ARG { \
    if (--argc <= 0) { \
	err = 1; \
	fprintf(stderr, "%s: %s expects an argument\n", appname, p); \
        continue; \
    } else \
	val = *++argv; \
}
#define SECOND_PASS_ONLY { if (!do_everything) continue; }

    while (--argc > 0) {
	char *p = *++argv;
        int ret;

	/*
	 * Shameless cheating. Debian requires all X terminal
	 * emulators to support `-T title'; but
	 * cmdline_process_param will eat -T (it means no-pty) and
	 * complain that pterm doesn't support it. So, in pterm
	 * only, we convert -T into -title.
	 */
	if ((cmdline_tooltype & TOOLTYPE_NONNETWORK) &&
	    !strcmp(p, "-T"))
	    p = (char*)"-title";

        ret = cmdline_process_param(p, (argc > 1 ? argv[1] : NULL),
                                    do_everything ? 1 : -1, conf);

	if (ret == -2) {
	    cmdline_error("option \"%s\" requires an argument",p);
	} else if (ret == 2) {
	    --argc, ++argv;            /* skip next argument */
            continue;
	} else if (ret == 1) {
            continue;
        }

	if (!strcmp(p, "-fn") || !strcmp(p, "-font")) {
        FontSpec *fs;
	    EXPECTS_ARG;
        SECOND_PASS_ONLY;
        fs = fontspec_new(val);
        conf_set_fontspec(conf, CONF_font, fs);
        fontspec_free(fs);
	} else if (!strcmp(p, "-fb")) {
        FontSpec *fs;
	    EXPECTS_ARG;
	    SECOND_PASS_ONLY;
        fs = fontspec_new(val);
        conf_set_fontspec(conf, CONF_boldfont, fs);
        fontspec_free(fs);
	} else if (!strcmp(p, "-fw")) {
        FontSpec *fs;
	    EXPECTS_ARG;
	    SECOND_PASS_ONLY;
        fs = fontspec_new(val);
        conf_set_fontspec(conf, CONF_widefont, fs);
        fontspec_free(fs);
	} else if (!strcmp(p, "-fwb")) {
        FontSpec *fs;
	    EXPECTS_ARG;
	    SECOND_PASS_ONLY;
        fs = fontspec_new(val);
        conf_set_fontspec(conf, CONF_wideboldfont, fs);
        fontspec_free(fs);
	} else if (!strcmp(p, "-cs")) {
	    EXPECTS_ARG;
	    SECOND_PASS_ONLY;
        conf_set_str(conf, CONF_line_codepage, val);

	} else if (!strcmp(p, "-geometry")) {
#ifdef Q_OS_UNIX
	    int flags, x, y;
	    unsigned int w, h;
	    EXPECTS_ARG;
	    SECOND_PASS_ONLY;

	    flags = XParseGeometry(val, &x, &y, &w, &h);
	    if (flags & WidthValue)
            conf_set_int(conf, CONF_width, w);
        if (flags & HeightValue)
            conf_set_int(conf, CONF_height, h);

            if (flags & (XValue | YValue)) {
                inst->xpos = x;
                inst->ypos = y;
                inst->gotpos = TRUE;
                inst->gravity = ((flags & XNegative ? 1 : 0) |
                                 (flags & YNegative ? 2 : 0));
            }
#endif
	} else if (!strcmp(p, "-sl")) {
	    EXPECTS_ARG;
	    SECOND_PASS_ONLY;
        conf_set_int(conf, CONF_savelines, atoi(val));

	} /*else if (!strcmp(p, "-fg") || !strcmp(p, "-bg") ||
		   !strcmp(p, "-bfg") || !strcmp(p, "-bbg") ||
		   !strcmp(p, "-cfg") || !strcmp(p, "-cbg")) {
	    GdkColor col;

	    EXPECTS_ARG;
	    SECOND_PASS_ONLY;
	    if (!gdk_color_parse(val, &col)) {
		err = 1;
		fprintf(stderr, "%s: unable to parse colour \"%s\"\n",
                        appname, val);
	    } else {
		int index;
		index = (!strcmp(p, "-fg") ? 0 :
			 !strcmp(p, "-bg") ? 2 :
			 !strcmp(p, "-bfg") ? 1 :
			 !strcmp(p, "-bbg") ? 3 :
			 !strcmp(p, "-cfg") ? 4 :
			 !strcmp(p, "-cbg") ? 5 : -1);
		assert(index != -1);
		cfg->colours[index][0] = col.red / 256;
		cfg->colours[index][1] = col.green / 256;
		cfg->colours[index][2] = col.blue / 256;
	    }

	} */else if (use_pty_argv && !strcmp(p, "-e")) {
	    /* This option swallows all further arguments. */
	    if (!do_everything)
		break;

	    if (--argc > 0) {
		int i;
		pty_argv = snewn(argc+1, char *);
		++argv;
		for (i = 0; i < argc; i++)
		    pty_argv[i] = argv[i];
		pty_argv[argc] = NULL;
		break;		       /* finished command-line processing */
	    } else
		err = 1, fprintf(stderr, "%s: -e expects an argument\n",
                                 appname);

	} else if (!strcmp(p, "-title")) {
	    EXPECTS_ARG;
	    SECOND_PASS_ONLY;
        conf_set_str(conf, CONF_wintitle, val);

	} else if (!strcmp(p, "-log")) {
        Filename *fn;
        EXPECTS_ARG;
        SECOND_PASS_ONLY;
        fn = filename_from_str(val);
        conf_set_filename(conf, CONF_logfilename, fn);
        conf_set_int(conf, CONF_logtype, LGTYP_DEBUG);
        filename_free(fn);

	} else if (!strcmp(p, "-ut-") || !strcmp(p, "+ut")) {
	    SECOND_PASS_ONLY;
        conf_set_int(conf, CONF_stamp_utmp, 0);

	} else if (!strcmp(p, "-ut")) {
	    SECOND_PASS_ONLY;
        conf_set_int(conf, CONF_stamp_utmp, 1);

	} else if (!strcmp(p, "-ls-") || !strcmp(p, "+ls")) {
	    SECOND_PASS_ONLY;
        conf_set_int(conf, CONF_login_shell, 0);

	} else if (!strcmp(p, "-ls")) {
	    SECOND_PASS_ONLY;
        conf_set_int(conf, CONF_login_shell, 1);

	} else if (!strcmp(p, "-nethack")) {
	    SECOND_PASS_ONLY;
        conf_set_int(conf, CONF_nethack_keypad, 1);

	} else if (!strcmp(p, "-sb-") || !strcmp(p, "+sb")) {
	    SECOND_PASS_ONLY;
        conf_set_int(conf, CONF_scrollbar, 0);

	} else if (!strcmp(p, "-sb")) {
	    SECOND_PASS_ONLY;
        conf_set_int(conf, CONF_scrollbar, 1);

	} else if (!strcmp(p, "-name")) {
	    EXPECTS_ARG;
	    app_name = val;

	} else if (!strcmp(p, "-xrm")) {
#ifndef Q_OS_WIN
	    EXPECTS_ARG;
	    provide_xrm_string(val);
#endif

	} else if(!strcmp(p, "-help") || !strcmp(p, "--help")) {
	    help(stdout);
	    exit(0);

        } else if (!strcmp(p, "-pgpfp")) {
            pgp_fingerprints();
            exit(1);

	} 

        else if(p[0] != '-' && (!do_everything ||
#ifdef Q_OS_WIN
                                (*allow_launch=handleHostnameCmdlineParam(p,cfg->host,sizeof(cfg->host)))
#else
                                process_nonoption_arg(p, conf, allow_launch)
#endif
        ))
        {
            /* do nothing */
	} 
        else {
	    err = 1;
	    fprintf(stderr, "%s: unrecognized option '%s'\n", appname, p);
	}
    }

    return err;
}


void setup_fonts_ucs(struct gui_data *inst)
{
    FontSpec *fs, *fsbasic;

    fsbasic = conf_get_fontspec(inst->conf, CONF_font);
    inst->fonts[0] = new QFont();
    inst->fonts[0]->fromString(fsbasic->name);

    fs = conf_get_fontspec(inst->conf, CONF_boldfont);
    inst->fonts[1] = new QFont();
    inst->fonts[1]->fromString(fs->name[0]?fs->name:fsbasic->name);
    inst->fonts[1]->setBold(true);

    fs = conf_get_fontspec(inst->conf, CONF_widefont);
    inst->fonts[2] = new QFont();
    inst->fonts[2]->fromString(fs->name[0]?fs->name:fsbasic->name);

    fs = conf_get_fontspec(inst->conf, CONF_wideboldfont);
    inst->fonts[3] = new QFont();
    inst->fonts[3]->fromString(fs->name[0]?fs->name:fsbasic->name);
    inst->fonts[3]->setBold(true);

    QFont::StyleStrategy aa;
    switch(conf_get_int(inst->conf, CONF_font_quality))
    {
        case FQ_DEFAULT:
        case FQ_NONANTIALIASED:
            aa=QFont::NoAntialias;
            break;
        default:
            aa=QFont::PreferAntialias;
    }
    for(int i=0;i<4;++i)
    {
        inst->fonts[i]->setStyleStrategy(aa);
    }

    QFontMetrics fm(*inst->fonts[0]);
    inst->font_width = fm.averageCharWidth();
    inst->font_height = fm.height();

#ifdef Q_OS_WIN
    init_ucs(&inst->cfg,&inst->ucsdata);
#else
    if(!ConfigWidget::isValidFont(*inst->fonts[0]))
    {
        conf_set_int(inst->conf, CONF_vtmode, VT_POORMAN);
    }
    inst->direct_to_font = init_ucs(&inst->ucsdata, conf_get_str(inst->conf, CONF_line_codepage),
                    conf_get_int(inst->conf, CONF_utf8_override), CS_ISO8859_1,
                    conf_get_int(inst->conf, CONF_vtmode));
#endif
}

void set_geom_hints(struct gui_data *)
{
}

void update_specials_menu(void *)
{
}

void start_backend(struct gui_data *inst)
{
    //extern Backend *select_backend(Config *cfg);
    char *realhost;
    const char *error;
    char *s;

    inst->back = select_backend(inst->conf);

    error = inst->back->init((void *)inst, &inst->backhandle, inst->conf,
                             conf_get_str(inst->conf, CONF_host),
                             conf_get_int(inst->conf, CONF_port),
                             &realhost, conf_get_int(inst->conf, CONF_tcp_nodelay),
                             conf_get_int(inst->conf, CONF_tcp_keepalives));

    if (error) {
        char *msg = dupprintf("Unable to open connection to %s:\n%s",
                              conf_get_str(inst->conf, CONF_host), error);
        inst->exited = TRUE;
        //fatal_message_box(inst->window, msg);
        QMessageBox::warning(qPutty,"Warning",msg);
        sfree(msg);
        exit(0);
    }

    s = conf_get_str(inst->conf, CONF_wintitle);
    if (s[0]) {
        set_title(inst, conf_get_str(inst->conf, CONF_wintitle));
        set_icon(inst, conf_get_str(inst->conf, CONF_wintitle));
    } else {
        QByteArray title = qPutty->defaultTitle(realhost).toLocal8Bit();
        set_title(inst, title.data());
        set_icon(inst, title.data());
    }
    sfree(realhost);

    inst->back->provide_logctx(inst->backhandle, inst->logctx);

    term_provide_resize_fn(inst->term, inst->back->size, inst->backhandle);

    inst->ldisc =
    ldisc_create(inst->conf, inst->term, inst->back, inst->backhandle,
		     inst);

}

#ifdef Q_OS_WIN
char *do_select(SOCKET skt, int startup)
{
    int msg, events;
    WId hwnd=get_windowid(0);
    if (startup) 
    {
       msg = WM_NETEVENT;
       events = (FD_CONNECT | FD_READ | FD_WRITE |
                 FD_OOB | FD_CLOSE | FD_ACCEPT);
    } 
    else 
    {
       msg = events = 0;
    }
    if (!hwnd)
       return "do_select(): internal error (hwnd==NULL)";
    if (p_WSAAsyncSelect(skt, hwnd, msg, events) == SOCKET_ERROR) 
    {
       switch (p_WSAGetLastError()) 
       {
         case WSAENETDOWN:
           return "Network is down";
         default:
           return "WSAAsyncSelect(): unknown error";
       }
    }
    return NULL;
}

void write_aclip(void *frontend, char *data, int len, int must_deselect)
{
    HGLOBAL clipdata;
    void *lock;

    clipdata = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, len + 1);
    if (!clipdata)
       return;
    lock = GlobalLock(clipdata);
    if (!lock)
       return;
    memcpy(lock, data, len);
    ((unsigned char *) lock)[len] = 0;
    GlobalUnlock(clipdata);

    if (!must_deselect)
       SendMessage(hwnd, WM_IGNORE_CLIP, TRUE, 0);

    if (OpenClipboard(hwnd)) 
    {
       EmptyClipboard();
       SetClipboardData(CF_TEXT, clipdata);
       CloseClipboard();
    } else
       GlobalFree(clipdata);

    if (!must_deselect)
       SendMessage(hwnd, WM_IGNORE_CLIP, FALSE, 0);
}

void cleanup_exit(int code)
{
    sk_cleanup();
    random_save_seed();
    exit(code);
}

Backend* select_backend(Config *cfg)
{
#ifdef PUTTY_RELEASE
    int i;
    Backend *back = NULL;
    for (i = 0; backends[i].backend != NULL; i++)
        if (backends[i].protocol == cfg->protocol) {
            back = backends[i].backend;
            break;
        }
#else
    Backend *back = backend_from_proto(cfg->protocol);
#endif
    assert(back != NULL);
    return back;
}

#endif

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
 * qtdlg.c - QT stubs of the PuTTY configuration box.
 *           does not much for the moment
 */

#include <QMessageBox>
#include <QPushButton>
#include <QDebug>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QStack>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QTableWidget>
#include <QListWidget>
#include "qPutty.h"
#include "configDialog.h"
#include "ui_eventLog.h"

// Putty's internal debug macro from misc.h breaks qDebug.
#if defined (debug) && !defined (QT_NO_DEBUG_OUTPUT) && defined (Q_OS_WIN)
#undef debug
#define debug debug
#endif

#ifdef TESTMODE
#define PUTTY_DO_GLOBALS	       /* actually _define_ globals */
#endif
extern QPutty* qPutty;

extern "C" {
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include "putty.h"
#include "storage.h"
#include "dialog.h"
#include "tree234.h"
void old_keyfile_warning(void);
}

#define FLAG_UPDATING_COMBO_LIST 1

static int string_width(char *)
{
    return 0;
}

int messagebox(void *, char *title, char *msg, int minwid, ...)
{
    QMessageBox msgBox(QMessageBox::NoIcon,title,msg,QMessageBox::NoButton,qPutty);
    va_list ap;
    va_start(ap,minwid);
    int count=-1;
    while (1) 
    {
        char *name = va_arg(ap, char *);
        int shortcut, type, value;
        if (name == NULL)
        {
            break;
        }
        shortcut = va_arg(ap, int);
        type = va_arg(ap, int);
        value = va_arg(ap, int);
        QPushButton* b=msgBox.addButton(name,type<0?QMessageBox::RejectRole:QMessageBox::AcceptRole);
        b->setShortcut(shortcut);
        if(type>0) msgBox.setDefaultButton(b);
        ++count;
    }
    va_end(ap);

    return qAbs(msgBox.exec()-count);
}

void *dlg_get_privdata(union control *ctrl, void *dlg)
{
    return ((ConfigWidget*)dlg)->privData(ctrl);
}

void dlg_set_privdata(union control *, void *, void *)
{
}

void *dlg_alloc_privdata(union control *ctrl, void *dlg, size_t size)
{
    return ((ConfigWidget*)dlg)->setPrivData(ctrl,size);
}

union control *dlg_last_focused(union control *, void *)
{
    return 0;
}

void dlg_radiobutton_set(union control *ctrl, void *dlg, int which)
{
    ((ConfigWidget*)dlg)->setRadioButton(ctrl,which);
}

int dlg_radiobutton_get(union control *ctrl, void *dlg)
{
    return ((ConfigWidget*)dlg)->radioButton(ctrl);
}

void dlg_checkbox_set(union control *ctrl, void *dlg, int checked)
{
    ((ConfigWidget*)dlg)->setCheckBox(ctrl,checked);
}

int dlg_checkbox_get(union control *ctrl, void *dlg)
{
    return ((ConfigWidget*)dlg)->checkBox(ctrl);
}

void dlg_editbox_set(union control *ctrl, void *dlg, char const *text)
{
    ((ConfigWidget*)dlg)->setEditBox(ctrl,text);
}

char *dlg_editbox_get(union control *ctrl, void *dlg)
{
    return ((ConfigWidget*)dlg)->editBox(ctrl);
}


/* The `listbox' functions can also apply to combo boxes. */
void dlg_listbox_clear(union control *ctrl, void *dlg)
{
    ((ConfigWidget*)dlg)->listBoxClear(ctrl);
}

void dlg_listbox_del(union control *ctrl, void *dlg, int index)
{
    ((ConfigWidget*)dlg)->listBoxDel(ctrl,index);
}

void dlg_listbox_add(union control *ctrl, void *dlg, char const *text)
{
    ((ConfigWidget*)dlg)->listBoxAdd(ctrl,text);
}

/*
 * Each listbox entry may have a numeric id associated with it.
 * Note that some front ends only permit a string to be stored at
 * each position, which means that _if_ you put two identical
 * strings in any listbox then you MUST not assign them different
 * IDs and expect to get meaningful results back.
 */
void dlg_listbox_addwithid(union control *ctrl, void *dlg,
			   char const *text, int id)
{
    ((ConfigWidget*)dlg)->listBoxAddWithId(ctrl,text,id);
}

int dlg_listbox_getid(union control *ctrl, void *dlg, int index)
{
    return ((ConfigWidget*)dlg)->listBoxGetId(ctrl,index);
}

/* dlg_listbox_index returns <0 if no single element is selected. */
int dlg_listbox_index(union control *ctrl, void *dlg)
{
    return ((ConfigWidget*)dlg)->listBoxIndex(ctrl);
}

int dlg_listbox_issel(union control* ctrl, void* dlg, int index)
{
    return ((ConfigWidget*)dlg)->listBoxIndex(ctrl)==index;
}

void dlg_listbox_select(union control *ctrl, void *dlg, int index)
{
    ((ConfigWidget*)dlg)->listBoxSelect(ctrl,index);
}

void dlg_text_set(union control *, void *, char const *)
{
}

void dlg_label_change(union control *ctrl, void *dlg, char const *text)
{
    ((ConfigWidget*)dlg)->setLabel(ctrl,text);
}

void dlg_filesel_set(union control *ctrl, void *dlg, Filename *fn)
{
    ((ConfigWidget*)dlg)->setFileBox(ctrl,fn->path);
}

Filename *dlg_filesel_get(union control *ctrl, void *dlg)
{
    return filename_from_str(((ConfigWidget*)dlg)->fileBox(ctrl));
}

void dlg_fontsel_set(union control *ctrl, void *dlg, FontSpec *fs)
{
    ((ConfigWidget*)dlg)->setFileBox(ctrl,fs->name);
}

FontSpec *dlg_fontsel_get(union control *ctrl, void *dlg)
{
#ifdef Q_OS_WIN
    struct dlgparam *dp = (struct dlgparam *)dlg;
    struct winctrl *c = dlg_findbyctrl(dp, ctrl);
    assert(c && c->ctrl->generic.type == CTRL_FONTSELECT);
    return fontspec_copy((FontSpec *)c->data);
#else
    return fontspec_new(((ConfigWidget*)dlg)->fileBox(ctrl));
#endif
}

/*
 * Bracketing a large set of updates in these two functions will
 * cause the front end (if possible) to delay updating the screen
 * until it's all complete, thus avoiding flicker.
 */
void dlg_update_start(union control *, void *)
{
    /*
     * Apparently we can't do this at all in GTK. GtkCList supports
     * freeze and thaw, but not GtkList. Bah.
     */
}

void dlg_update_done(union control *, void *)
{
    /*
     * Apparently we can't do this at all in GTK. GtkCList supports
     * freeze and thaw, but not GtkList. Bah.
     */
}

void dlg_set_focus(union control *, void *)
{
}

/*
 * During event processing, you might well want to give an error
 * indication to the user. dlg_beep() is a quick and easy generic
 * error; dlg_error() puts up a message-box or equivalent.
 */
void dlg_beep(void *)
{
    //gdk_beep();
}


void dlg_error_msg(void *dlg, char *msg)
{
    ((ConfigWidget*)dlg)->errorMessage(msg);
}

/*
 * This function signals to the front end that the dialog's
 * processing is completed, and passes an integer value (typically
 * a success status).
 */
void dlg_end(void* dlg, int value)
{
    if(!value)
    {
        ((ConfigWidget*)dlg)->reject();
    }
    else
    {
        ((ConfigWidget*)dlg)->accept();
    }
}

void dlg_refresh(union control *ctrl, void *dlg)
{
    ((ConfigWidget*)dlg)->refresh(ctrl);
}

void dlg_coloursel_start(union control *ctrl, void *dlg, int r, int g, int b)
{
    ((ConfigWidget*)dlg)->selectColor(ctrl,r,g,b);
}

int dlg_coloursel_results(union control* ctrl, void *dlg,int* r,int* g,int* b)
{
    ((ConfigWidget*)dlg)->selectedColor(ctrl,r,g,b);
    return 1;
}

/* ----------------------------------------------------------------------
 * Signal handlers while the dialog box is active.
 */


int get_listitemheight(void)
{
    return 8;
}

int do_config_box(const char *title, Conf *conf, int midsession, int protcfginfo)
{
    ConfigDialog cd(title,conf,midsession,protcfginfo);
    cd.exec();
    return cd.result();
}


int reallyclose(void *)
{
    char *title = dupcat(appname, " Exit Confirmation", NULL);
    int ret = messagebox(0,
			 title, "Are you sure you want to close this session?",
			 string_width("Most of the width of the above text"),
			 "Yes", 'y', +1, 1,
			 "No", 'n', -1, 0,
			 NULL);
    sfree(title);
    return ret;
}

int verify_ssh_host_key(void *, char *host, int port, char *keytype,
                        char *keystr, char *fingerprint,
                        void (*)(void *, int ), void *)
{
    static const char absenttxt[] =
	"The server's host key is not cached. You have no guarantee "
	"that the server is the computer you think it is.\n"
	"The server's %s key fingerprint is:\n"
	"%s\n"
	"If you trust this host, press \"Accept\" to add the key to "
	"PuTTY's cache and carry on connecting.\n"
	"If you want to carry on connecting just once, without "
	"adding the key to the cache, press \"Connect Once\".\n"
	"If you do not trust this host, press \"Cancel\" to abandon the "
	"connection.";
    static const char wrongtxt[] =
	"WARNING - POTENTIAL SECURITY BREACH!\n"
	"The server's host key does not match the one PuTTY has "
	"cached. This means that either the server administrator "
	"has changed the host key, or you have actually connected "
	"to another computer pretending to be the server.\n"
	"The new %s key fingerprint is:\n"
	"%s\n"
	"If you were expecting this change and trust the new key, "
	"press \"Accept\" to update PuTTY's cache and continue connecting.\n"
	"If you want to carry on connecting but without updating "
	"the cache, press \"Connect Once\".\n"
	"If you want to abandon the connection completely, press "
	"\"Cancel\" to cancel. Pressing \"Cancel\" is the ONLY guaranteed "
	"safe choice.";
    char *text;
    int ret;

    /*
     * Verify the key.
     */
    ret = verify_host_key(host, port, keytype, keystr);

    if (ret == 0)		       /* success - key matched OK */
	return 1;

    if(!qPutty->isAlwaysAcceptHostKey())
    {
        text = dupprintf((ret == 2 ? wrongtxt : absenttxt), keytype, fingerprint);

        ret = messagebox(0,
		     "PuTTY Security Alert", text,
		     string_width(fingerprint),
		     "Accept", 'a', 0, 2,
		     "Connect Once", 'o', 0, 1,
		     "Cancel", 'c', -1, 0,
		     NULL);

        sfree(text);
    }
    else
    {
        ret=2;
    }

    if (ret == 2) {
        store_host_key(host, port, keytype, keystr);
        return 1;		       /* continue with connection */
    } else if (ret == 1)
        return 1;		       /* continue with connection */
    return 0;			       /* do not continue with connection */
}

/*
 * Ask whether the selected algorithm is acceptable (since it was
 * below the configured 'warn' threshold).
 */
int askalg(void *, const char *algtype, const char *algname,
	   void (*)(void *, int ), void *)
{
    static const char msg[] =
	"The first %s supported by the server is "
	"%s, which is below the configured warning threshold.\n"
	"Continue with connection?";
    char *text;
    int ret;

    text = dupprintf(msg, algtype, algname);
    ret = messagebox(0,
		     "PuTTY Security Alert", text,
		     string_width("Continue with connection?"),
		     "Yes", 'y', 0, 1,
		     "No", 'n', 0, 0,
		     NULL);
    sfree(text);

    if (ret) {
        return 1;
    } else {
        return 0;
    }
}

void old_keyfile_warning(void)
{
    /*
     * This should never happen on Unix. We hope.
     */
}

void fatal_message_box(void *, char *msg)
{
    QMessageBox::warning(qPutty,"PuTTY Fatal Error", msg);
}

void fatalbox(char *p, ...)
{
    va_list ap;
    char *msg;
    va_start(ap, p);
    msg = dupvprintf(p, ap);
    va_end(ap);
    fatal_message_box(NULL, msg);
    sfree(msg);
    cleanup_exit(1);
}

void nonfatal_message_box(void *, char *msg)
{
    QMessageBox::information(qPutty,"PuTTY Info", msg);
}

void nonfatal(char *p, ...)
{
    va_list ap;
    char *msg;
    va_start(ap, p);
    msg = dupvprintf(p, ap);
    va_end(ap);
    nonfatal_message_box(NULL, msg);
    sfree(msg);
}

void about_box(void *)
{
    qPutty->about();
}


void showeventlog(void* eStuff)
{
    struct eventlog_stuff* es=(struct eventlog_stuff*)eStuff;
    QDialog d(qPutty);
    Ui::EventLog el;
    el.setupUi(&d);
    for(int i=0;i<es->nevents;++i)
    {
        el.plainTextEdit->appendPlainText(es->events[i]);
    }
    d.exec();
}

void *eventlogstuff_new(void)
{
    struct eventlog_stuff *es;
    es = snew(struct eventlog_stuff);
    memset(es, 0, sizeof(*es));
    return es;
}

void logevent_dlg(void *estuff, const char *string)
{
    struct eventlog_stuff *es = (struct eventlog_stuff *)estuff;

    char timebuf[40];
    struct tm tm;

    if (es->nevents >= es->negsize) {
        es->negsize += 64;
        es->events = sresize(es->events, es->negsize, char *);
    }

    tm=ltime();
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S\t", &tm);

    es->events[es->nevents] = snewn(strlen(timebuf) + strlen(string) + 1, char);
    strcpy(es->events[es->nevents], timebuf);
    strcat(es->events[es->nevents], string);
    /*
    if (es->window) {
	dlg_listbox_add(es->listctrl, &es->dp, es->events[es->nevents]);
    }
    */
    es->nevents++;
    qPutty->eventLogUpdate(es->nevents);
}

int askappend(void *, Filename *filename,
	      void (*)(void *, int ), void *)
{
    static const char msgtemplate[] =
	"The session log file \"%.*s\" already exists. "
	"You can overwrite it with a new session log, "
	"append your session log to the end of it, "
	"or disable session logging for this session.";
    char *message;
    char *mbtitle;
    int mbret;

    message = dupprintf(msgtemplate, FILENAME_MAX, filename->path);
    mbtitle = dupprintf("%s Log to File", appname);

    mbret = messagebox(0, mbtitle, message,
		       string_width("LINE OF TEXT SUITABLE FOR THE"
				    " ASKAPPEND WIDTH"),
		       "Overwrite", 'o', 1, 2,
		       "Append", 'a', 0, 1,
		       "Disable", 'd', -1, 0,
		       NULL);

    sfree(message);
    sfree(mbtitle);

    return mbret;
}

int from_backend_eof(void *frontend)
{
    return TRUE;   /* do respond to incoming EOF with outgoing */
}

#ifdef Q_OS_WIN
void show_help(HWND hwnd)
{
    launch_help(hwnd,0);
}

void modal_about_box(HWND )
{
    qPutty->about();
}

int dlg_get_fixed_pitch_flag(void *)
{
    qDebug() << "not implemented" << "dlg_get_fixed_pitch_flag";
    return 0;
}
void dlg_set_fixed_pitch_flag(void *, int )
{
    qDebug() << "not implemented" << "dlg_set_fixed_pitch_flag";
}
#endif


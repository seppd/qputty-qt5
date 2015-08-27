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
#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include <QFont>
#include <QColor>
#include <QColormap>

#ifdef __cplusplus
extern "C" {
#endif
#include <putty.h>
#include <terminal.h>
#ifdef __cplusplus
}
#endif

#define NEXTCOLOURS 240 /* 216 colour-cube plus 24 shades of grey */
#define NALLCOLOURS (NCFGCOLOURS + NEXTCOLOURS)

struct gui_data {
    QFont* fonts[4];                 /* normal, bold, wide, widebold */
    int xpos, ypos, gotpos, gravity;
    QColor cols[NALLCOLOURS];
    QColormap *colmap;
    wchar_t *pastein_data;
    int direct_to_font;
    int pastein_data_len;
    char *pasteout_data, *pasteout_data_ctext, *pasteout_data_utf8;
    int pasteout_data_len, pasteout_data_ctext_len, pasteout_data_utf8_len;
    int font_width, font_height;
    int width, height;
    int ignore_sbar;
    int mouseptr_visible;
    int busy_status;
    quint32 term_paste_idle_id;
    int alt_keycode;
    int alt_digits;
    char *wintitle;
    char *icontitle;
    int master_fd, master_func_id;
    void *ldisc;
    Backend *back;
    void *backhandle;
    Terminal *term;
    void *logctx;
    int exited;
    struct unicode_data ucsdata;
    Conf *conf;
    void *eventlogstuff;
    const char *progname;
    int ngtkargs;
    quint32 input_event_time; /* Timestamp of the most recent input event. */
    int reconfiguring;
};

struct eventlog_stuff {
    struct controlbox *eventbox;
    union control *listctrl;
    char **events;
    int nevents, negsize;
    char *seldata;
    int sellen;
    int ignore_selchange;
};

#endif

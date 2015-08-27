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
#ifndef _Q_PUTTY_H_
#define _Q_PUTTY_H_

#include <QWidget>
#include <QScrollBar>
#include <QPalette>
#include <QGridLayout>
#include <QStringList>
#include "structs.h"

struct conf_tag {
    tree234 *tree;
};

class AbstractTerminalWidget;
class QMenu;
class QTemporaryFile;
class QPutty: public QWidget
{
    Q_OBJECT
    public:
    QPutty(QWidget* parent=0);
    virtual ~QPutty();
    int run(const QStringList& args);
    QString defaultTitle(const QString& hostname) const;
    void setTitle(const QString&);
    void updateScrollBar(int length,int value,int pageStep);
    void updateCursorVisibility(int x,int y);
    void setCursor(int col,int line,const QString& text,unsigned long attr);
    void setSelectedText(const QString& text,bool mustDeselect);
    virtual void insertText(int col,int line,const QString& text,unsigned long attr);
    virtual void close(int exitCode);
    virtual void scroll(int lines);
#ifdef Q_OS_WIN
    bool winEvent(MSG* msg,long* result);
#else
    int registerFd(int fd,int rwx);
    void releaseFd(int id);
#endif
    void timerChangeNotify(long ticks,long nextNow);
    virtual void eventLogUpdate(int eventNo);
    virtual bool isAlwaysAcceptHostKey() const;

    public slots:
    virtual void requestPaste();
    virtual void showEventLog();
    virtual void about();
    virtual void reconfigure();
    virtual void newSession();
    virtual void dupSession();
    virtual void savedSession();
    virtual void setTitle();

    signals:
    void finished(int exitCode);

    protected:
    virtual void closeEvent(QCloseEvent* event);
    static char** toArgv(const QStringList& args);
    void resizeEvent(QResizeEvent*);
    void changeEvent(QEvent*);
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual AbstractTerminalWidget* createTerminalWidget();
    virtual void setupPutty();
    virtual void setupDefaults();
    virtual int parseCommandLine(const QStringList& args);
    virtual int showConfigDialog();
    virtual void setupTerminalControl();
    virtual void setupScrollBar();
    virtual void setupLogging();
    virtual void setupFonts();
    virtual void setupPalette();
    virtual void setupOverwriteDefaults();
    virtual void setupWidget();
    virtual void startDetachedProcess(const QString& cmd);
    virtual QMenu* savedSessionContextMenu() const;
    virtual bool isScalingMode();

    private:
#ifdef Q_OS_WIN
    void setupDefaultFontName(FontSpec *fs);
#endif

    protected:
    AbstractTerminalWidget* _terminalWidget;
    QScrollBar _vBar;
    QPalette _palette;
    QGridLayout _layout;
    struct gui_data _inst;
    bool _running;
    int _shownLogEvents;
    int _eventLogCount;
    QSize _oldSize;
    QTemporaryFile* _ppk;
};

#endif

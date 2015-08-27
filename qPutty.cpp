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
#include "qPutty.h"
#include "abstractTerminalWidget.h"
#include <QMessageBox>
#include <QKeyEvent>
#include <QPainter>
#include <QTextLayout>
#include <QApplication>
#include <QFileInfo>
#include <QProcess>
#include "platform.h"
#include "terminalWidget.h"
#include "version.h"
#include <csignal>
#include <QMenu>
#include <QDir>
#include <QTemporaryFile>
#include "ui_passphrase.h"

extern "C" {
#include <storage.h>
#include <ssh.h>
}

extern "C++" {
    int do_cmdline(int argc, char **argv, int do_everything, int *allow_launch,struct gui_data *inst, Conf *conf);
    void setup_fonts_ucs(struct gui_data *inst);
    void start_backend(struct gui_data *inst);
    void showeventlog(void* es);
}

extern QPutty* qPutty; //qtwin.cpp

QPutty::QPutty(QWidget* parent): QWidget(parent),_terminalWidget(0),_vBar(Qt::Vertical),_layout(this),_running(false),_shownLogEvents(0),_eventLogCount(0),_ppk(0)
{
    _layout.setContentsMargins(0,0,0,0);
    _layout.setSpacing(0);
    _layout.setRowStretch(1,1);
    _layout.setColumnStretch(2,1);
    _vBar.setRange(0,0);
    setAttribute(Qt::WA_Resized);
    qPutty=this;
}

QPutty::~QPutty()
{
    free((char*)_inst.progname);
    struct eventlog_stuff* els=((struct eventlog_stuff*)_inst.eventlogstuff);
    for(int i=0;i<els->nevents;++i)
    {
        sfree(els->events[i]);
    }
    sfree(els);
    for(int i=0;i<4;++i)
    {
        delete(_inst.fonts[i]);
    }
    if(_terminalWidget)
    {
        delete _terminalWidget;
    }
    delete _ppk;
    conf_free(_inst.conf);
}

int 
QPutty::run(const QStringList& args)
{
    if(_running) return 1;
#ifndef Q_OS_WIN
    block_signal(SIGCHLD, 1);
#endif
    setupPutty();
    _inst.progname=strdup(qPrintable(qAppName()));
    _inst.conf = conf_new();
    setupDefaults();
    int ret=parseCommandLine(args);
    if(ret<1) return ret;
    setupLogging();
    setupFonts();
    setupPalette();
    setupTerminalControl();
    setupScrollBar();
    setupOverwriteDefaults();
    setupWidget();
    start_backend(&_inst);
    ldisc_send(_inst.ldisc, NULL, 0, 0);
#ifndef Q_OS_WIN
    block_signal(SIGCHLD, 0);
    block_signal(SIGPIPE, 1);
#endif
    _inst.exited = FALSE;
    _running=true;
    resize(sizeHint());
    return ret;
}

QString
QPutty::defaultTitle(const QString& hostname) const
{
    return QString("%1 - %2").arg(hostname).arg(_inst.progname);
}

void 
QPutty::setTitle(const QString& text)
{
    setWindowTitle(QString("%1 %2x%3 %4").arg(text).arg(_inst.term->cols).arg(_inst.term->rows).arg(_shownLogEvents<_eventLogCount?'*':' '));
}

void 
QPutty::updateScrollBar(int length,int value,int pageStep)
{
    if(_vBar.maximum()!=length-pageStep)
    {
        _vBar.setRange(0,length-pageStep);
    }
    if(_vBar.value()!=value)
    {
        _vBar.blockSignals(true);
        _vBar.setValue(value);
        _vBar.blockSignals(false);
    }
}

void 
QPutty::setCursor(int x,int y,const QString& text,unsigned long attr)
{
    _terminalWidget->setCursor(x,y,text,attr);
}

void 
QPutty::setSelectedText(const QString& text,bool mustDeselect)
{
    _terminalWidget->setSelectedText(text);
    if(mustDeselect)
    {
        term_deselect(_inst.term);
    }
}

void 
QPutty::requestPaste()
{
    _terminalWidget->requestPaste();
}

void 
QPutty::showEventLog()
{
    showeventlog(_inst.eventlogstuff);
    _shownLogEvents=((struct eventlog_stuff*)_inst.eventlogstuff)->nevents;
    setTitle();
}

void 
QPutty::about()
{
    QMessageBox::about(this,QString("About %1").arg(_inst.progname),QString("QT Version: %1\n QPutty Version: %2\n Putty Version: %3\n").arg(QT_VERSION_STR).arg(VERSION).arg(PUTTY_VERSION));
}

void 
QPutty::reconfigure()
{
    QByteArray ba=QString("%1 Reconfiguration").arg(_inst.progname).toLocal8Bit();
    Conf conf2=*_inst.conf;
    if(do_config_box(ba.data(),&conf2,1,1))
    {
        *_inst.conf=conf2;
        _inst.reconfiguring=TRUE;
        setupLogging();
        setupFonts();
        setupPalette();
        setupTerminalControl();
        setupScrollBar();
        setupWidget();
        log_reconfig(_inst.logctx,&conf2);
        if(_inst.ldisc) ldisc_send(_inst.ldisc,NULL,0,0);
        term_reconfig(_inst.term,&conf2);
        if(_inst.back) _inst.back->reconfig(_inst.backhandle,&conf2);
        _inst.reconfiguring=FALSE;
        resize(_inst.font_width*_inst.term->cols+((conf_get_int(_inst.conf, CONF_scrollbar))?_vBar.width():0),_inst.font_height*_inst.term->rows);
    }
}

void 
QPutty::newSession()
{
    startDetachedProcess(QCoreApplication::arguments().at(0));
}

void 
QPutty::dupSession()
{
    char* error=save_settings((char*)"QPuttyDupTempSession",_inst.conf);
    if(error)
    {
        QMessageBox::critical(this,"Create temporary config failed!",error);
        return;
    }
    startDetachedProcess(QString("%1 --loadtmp QPuttyDupTempSession").arg(QCoreApplication::arguments().at(0)));
}

void 
QPutty::savedSession()
{
    QAction* a=qobject_cast<QAction*>(sender());
    if(a)
    {
        startDetachedProcess(QString("%1 -load %2").arg(QCoreApplication::arguments().at(0)).arg(a->text()));
    }
}

void 
QPutty::setTitle()
{
    setTitle(_inst.wintitle);
}

void
QPutty::insertText(int x,int y,const QString& text,unsigned long attr)
{
    _terminalWidget->insertText(x,y,text,attr);
}

void 
QPutty::close(int exitCode)
{
    if(QWidget::close())
    {
        emit finished(exitCode);
    }
}

void 
QPutty::scroll(int lines)
{
    _terminalWidget->scroll(lines);
}

#ifdef Q_OS_WIN
bool
QPutty::winEvent(MSG*msg,long* result)
{
    switch(msg->message)
    {
        case WM_NETEVENT:
            select_result(msg->wParam,msg->lParam);
            result=0;
            return true;
    }
    return false;
}

#else
int 
QPutty::registerFd(int fd,int rwx)
{
    return _terminalWidget->registerFd(fd,rwx);
}

void
QPutty::releaseFd(int id)
{
    _terminalWidget->releaseFd(id);
}
#endif

void 
QPutty::timerChangeNotify(long ticks,long nextNow)
{
    _terminalWidget->timerChangeNotify(ticks,nextNow);
}

void 
QPutty::eventLogUpdate(int eventNo)
{
    _eventLogCount=eventNo;
}

bool 
QPutty::isAlwaysAcceptHostKey() const
{
    return false;
}

void 
QPutty::closeEvent(QCloseEvent* event)
{
    if(!_inst.exited && conf_get_int(_inst.conf, CONF_warn_on_close))
    { 
        if(!reallyclose(&_inst))
        {
            event->ignore();
            return;
        }
    }
    event->accept();
}

char** 
QPutty::toArgv(const QStringList& args)
{
    char** argv=new char*[args.size()+1];
    for (int i=0;i<args.size();++i)
    {
        argv[i]=strdup(args[i].toLocal8Bit().data());
    }
    argv[args.size()]=0;
    return argv;
}

void 
QPutty::changeEvent(QEvent *e)
{
    if(e->type()==QEvent::WindowStateChange)
    {
        int changed=windowState()^((QWindowStateChangeEvent*)e)->oldState();
        if(changed & Qt::WindowMaximized)
        {
            if((conf_get_int(_inst.conf, CONF_resize_action)==RESIZE_TERM) && !isMaximized())
            {
                QSize s(size());
                s.setWidth(s.width()-((conf_get_int(_inst.conf, CONF_scrollbar))?_vBar.width():0));
#if defined(Q_OS_UNIX) || defined(Q_OS_MAC)
                QCoreApplication::postEvent(_terminalWidget,new QResizeEvent(s,_terminalWidget->sizeHint()));
#else
                QCoreApplication::postEvent(_terminalWidget,new QResizeEvent(_terminalWidget->sizeHint(),s));
#endif
            }
            else if(conf_get_int(_inst.conf, CONF_resize_action)==RESIZE_EITHER)
            {
                if(!isMaximized())
                {
                    QSize s(_terminalWidget->sizeHint());
                    _terminalWidget->scale(s.width(),s.height());
                }
#if defined(Q_OS_UNIX) || defined(Q_OS_MAC)
                else if(isMaximized())
                {
                    _terminalWidget->resize(_oldSize.width()-((conf_get_int(_inst.conf, CONF_scrollbar))?_vBar.width():0),_oldSize.height());
                    QCoreApplication::postEvent(this,new QResizeEvent(size(),_oldSize));
                }
#endif
            }
        }
    }
    QWidget::changeEvent(e);
}

/* 
 * Unix issue with maximize.
 * If maximize is pressed resizeEvent is called but isMaximized() is not true.
 * So the terminal widget is resized to the wrong size, to fix it a second resize event
 * is send with the right size inside changeEvent.
 */
void 
QPutty::resizeEvent(QResizeEvent *re)
{
    if(conf_get_int(_inst.conf, CONF_resize_action)==RESIZE_DISABLED)
    {
        setMaximumSize(sizeHint());
        setMinimumSize(sizeHint());
    }
    else if( conf_get_int(_inst.conf, CONF_resize_action)!=RESIZE_TERM && isScalingMode())
    {
        _terminalWidget->scale(re->size().width()-((conf_get_int(_inst.conf, CONF_scrollbar))?_vBar.width():0),re->size().height());
    }
    else
    {
        _terminalWidget->resize(re->size().width()-((conf_get_int(_inst.conf, CONF_scrollbar))?_vBar.width():0),re->size().height());
    } 
    _oldSize.setWidth((((re->oldSize().width()-((conf_get_int(_inst.conf, CONF_scrollbar))?_vBar.width():0))/_inst.font_width/_inst.term->cols)*_inst.font_width*_inst.term->cols)+((conf_get_int(_inst.conf, CONF_scrollbar))?_vBar.width():0));
    _oldSize.setHeight((re->oldSize().height()/_inst.font_height/_inst.term->rows)*_inst.font_height*_inst.term->rows);
    QWidget::resizeEvent(re);
}

void 
QPutty::setupPutty()
{
    memset(&_inst,0,sizeof(_inst));
    memset(&_inst.ucsdata,0,sizeof(_inst.ucsdata));
    sk_init();
}

void 
QPutty::setupDefaults()
{
    flags = FLAG_VERBOSE | FLAG_INTERACTIVE;
    _inst.alt_keycode = -1;
    _inst.busy_status = BUSY_NOT;
    do_defaults(NULL, _inst.conf);
}

int 
QPutty::parseCommandLine(const QStringList& args)
{
    int allow_launch=FALSE;
    int loadTmp=-1;
    char** argv;
    for(int i=0;i<args.size();++i)
    {
        if(((QString)args[i]).simplified()=="--version")
        {
            printf("%s-%s-%s-%s\n",_inst.progname,VERSION,PUTTY_VERSION,QT_VERSION_STR);
            return 0;
        }
        if(((QString)args[i]).simplified()=="--loadtmp")
        {
            QStringList correctedArgs(args);
            correctedArgs[i]="-load";
            loadTmp=i;
            argv=toArgv(correctedArgs);
        }
    }
    if(loadTmp==-1)
    {
        argv=toArgv(args);
    }
    if(do_cmdline(args.size(),argv,1,&allow_launch,&_inst,_inst.conf))
    {
        return -1;
    }
    cmdline_run_saved(_inst.conf);
    if(loaded_session)
    {
        allow_launch=TRUE;
    }
    if(loadTmp!=-1)
    {
        del_settings(argv[loadTmp+1]);
    }
    for(int i=0;argv[i]!=0;++i)
    {
        free(argv[i]);
    }
    delete[] (argv);
    if((!allow_launch || !conf_launchable(_inst.conf)) && !showConfigDialog())
    {
        return -1;
    }
    return 1;
}

int 
QPutty::showConfigDialog()
{
    QByteArray ba=QString("%1 Configuration").arg(_inst.progname).toLocal8Bit();
    return do_config_box(ba.data(),_inst.conf,0,0);
}

void
QPutty::setupTerminalControl()
{
    if(!_inst.reconfiguring)
    {
        _vBar.disconnect(_terminalWidget);
        delete _terminalWidget;
        _terminalWidget=createTerminalWidget();
        connect(&_vBar,SIGNAL(valueChanged(int)),_terminalWidget,SLOT(scrollTermTo(int)));
        connect(_terminalWidget,SIGNAL(termSizeChanged()),this,SLOT(setTitle()));
        _layout.addWidget((QWidget*)_terminalWidget->realObject(),0,1);
    }
    _inst.width = conf_get_int(_inst.conf, CONF_width);
    _inst.height = conf_get_int(_inst.conf, CONF_height);
    if(!_inst.reconfiguring)
    {
        _inst.term = term_init(_inst.conf, &_inst.ucsdata, &_inst);
        term_provide_logctx(_inst.term, _inst.logctx);
    }
    term_size(_inst.term,_inst.height,_inst.width,conf_get_int(_inst.conf, CONF_savelines));
    if(!_inst.reconfiguring)
    {
#ifdef Q_OS_WIN
    sk_init();
#else
    uxsel_init();
#endif
    }
    _terminalWidget->init();
}

void
QPutty::setupScrollBar()
{
    if(conf_get_int(_inst.conf, CONF_scrollbar))
    {
        if(conf_get_int(_inst.conf, CONF_scrollbar_on_left))
        {
            _layout.addWidget(&_vBar,0,0);
        }
        else
        {
            _layout.addWidget(&_vBar,0,2);
        }
        _vBar.setPalette(QApplication::palette());
    }
    else
    {
        _vBar.hide();
    }
}

void 
QPutty::setupLogging()
{
    if(!_inst.reconfiguring)
    {
        _inst.eventlogstuff=eventlogstuff_new();
    }
    _inst.logctx = log_init(&_inst, _inst.conf);
}

void
QPutty::setupFonts()
{
#ifdef Q_OS_MAC
    conf_set_int(_inst.conf, CONF_utf8_override, 0);
#endif
#ifdef Q_OS_WIN
    if(!_inst.reconfiguring)
    {
    setupDefaultFontName(conf_get_fontspec(_inst.conf, CONF_font));
    setupDefaultFontName(conf_get_fontspec(_inst.conf, CONF_boldfont));
    setupDefaultFontName(conf_get_fontspec(_inst.conf, CONF_widefont));
    setupDefaultFontName(conf_get_fontspec(_inst.conf, CONF_wideboldfont));
    }
#endif
    setup_fonts_ucs(&_inst);
}

void
QPutty::setupPalette()
{
    palette_reset(&_inst);
    _palette.setColor(QPalette::WindowText,_inst.cols[ATTR_DEFFG]);
    _palette.setColor(QPalette::Text,_inst.cols[ATTR_DEFFG]);
    _palette.setColor(QPalette::HighlightedText,_inst.cols[ATTR_DEFFG]);
    _palette.setColor(QPalette::Window,_inst.cols[258]);
    _palette.setColor(QPalette::Base,_inst.cols[258]);
    _palette.setColor(QPalette::Highlight,_inst.cols[258]);
    setPalette(_palette);
}


void 
QPutty::setupOverwriteDefaults()
{
#ifdef Q_OS_WIN
    conf_set_int(_inst.conf, CONF_alt_f4, 1);
    conf_set_int(_inst.conf, CONF_alt_space, 1);
#endif
    if(strlen(conf_get_filename(_inst.conf, CONF_keyfile)->path)!=0)
    {
        Filename *ppkif=filename_from_str(conf_get_filename(_inst.conf, CONF_keyfile)->path);
        int t=key_type(ppkif);
        switch(t)
        {
            case SSH_KEYTYPE_OPENSSH:
            case SSH_KEYTYPE_SSHCOM:
                const char* error=NULL;
                struct ssh2_userkey* ssh2key=import_ssh2(ppkif,t,"1",&error);
                while(1)
                {
                    if(ssh2key)
                    {
                        if(ssh2key==SSH2_WRONG_PASSPHRASE)
                        {
                            QDialog d;
                            Ui::Passphrase pd;
                            pd.setupUi(&d);
                            QString h=QString("%1 %2").arg(pd.headline->text()).arg(conf_get_filename(_inst.conf, CONF_keyfile)->path);
                            pd.headline->setText(h);
                            if(!d.exec())
                            {
                                break;
                            }
                            ssh2key=import_ssh2(ppkif,t,(char*)qPrintable(pd.lineEdit->text()),&error);
                        }
                        else
                        {
                            QDir d(conf_get_filename(_inst.conf, CONF_keyfile)->path);
                            d.cdUp();
                            _ppk=new QTemporaryFile(QString("%1/ppk").arg(d.absolutePath()));
                            _ppk->open();
                            Filename *ppkof=filename_from_str(qPrintable(_ppk->fileName()));
                            ssh2_save_userkey(ppkof,ssh2key,NULL);
                            strncpy(conf_get_filename(_inst.conf, CONF_keyfile)->path,qPrintable(_ppk->fileName()),strlen(conf_get_filename(_inst.conf, CONF_keyfile)->path));
                            break;
                        }
                    }
                    else
                    {
                        fprintf(stderr,"error ssh key: %s!\n",error!=NULL?error:"unkown error");
                    }
                }
        }
    }
}

void 
QPutty::setupWidget()
{
    if(conf_get_int(_inst.conf, CONF_resize_action)==RESIZE_DISABLED)
    {
        setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    }
    else
    {
        setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
    }
#ifdef Q_OS_WIN
    if(conf_get_int(_inst.conf, CONF_alwaysontop))
    {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }
    else
    {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    }
#endif
}

AbstractTerminalWidget* 
QPutty::createTerminalWidget()
{
    return new PTerminal::Widget(&_inst);
}

void 
QPutty::contextMenuEvent(QContextMenuEvent* event)
{
    if((event->modifiers() & Qt::ControlModifier) || conf_get_int(_inst.conf, CONF_mouse_is_xterm)==2)
    {
        QMenu* menu=new QMenu(this);
        _terminalWidget->contextMenu(menu);
        QAction* a;
        menu->addSeparator();
        a=menu->addAction(tr("&New Session"),this,SLOT(newSession()));
        a=menu->addAction(tr("&Duplicate Session"),this,SLOT(dupSession()));
        QMenu* savedSessionMenu=savedSessionContextMenu();
        a=menu->addMenu(savedSessionMenu);
        menu->addSeparator();
        a=menu->addAction(tr("Chan&ge Settings"),this,SLOT(reconfigure()));
        menu->addSeparator();
        a=menu->addAction(tr("Event &Log"),this,SLOT(showEventLog()));
        menu->addSeparator();
        a=menu->addAction(tr("About"),this,SLOT(about()));
        menu->exec(event->globalPos());
        delete savedSessionMenu;
        delete menu;
    }
}

void 
QPutty::startDetachedProcess(const QString& cmd)
{
    if(!QProcess::startDetached(cmd))
    {
        QMessageBox::critical(this,"Executing error",QString("Can't execute %1!").arg(cmd));
    }
}

QMenu* 
QPutty::savedSessionContextMenu() const
{
    struct sesslist sesslist;
    get_sesslist(&sesslist,TRUE);
    QMenu* m=new QMenu(tr("&Saved Sessions"));
    QAction* a;
    for(int i=1;i<sesslist.nsessions;++i) 
    {
        a=m->addAction(sesslist.sessions[i],this,SLOT(savedSession()));
    }
    get_sesslist(&sesslist,FALSE);
    return m;
}

bool 
QPutty::isScalingMode()
{
    if((isMaximized() && conf_get_int(_inst.conf, CONF_resize_action)==RESIZE_EITHER) || conf_get_int(_inst.conf, CONF_resize_action)==RESIZE_FONT)
    {
        _layout.setRowStretch(1,0);
        _layout.setColumnStretch(2,0);
        return true;
    }
    _layout.setRowStretch(1,1);
    _layout.setColumnStretch(2,1);
    return false;
}

#ifdef Q_OS_WIN
void
QPutty::setupDefaultFontName(FontSpec *fs)
{
    strncpy(fs->name,qPrintable(QString("%1,%2").arg(fs->name).arg(fs->height)),sizeof(fs->name));
    fs->name[sizeof(fs->name)-1] = '\0';
}
#endif

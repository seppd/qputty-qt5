/*
 *      Copyright (C) 2010 - 2011 Four J's Development Tools Europe, Ltd.
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
#include "terminalWidget.h"
#include <QApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QClipboard>
#include <QMimeData>
#include <QMenu>
#include <QDebug>
#include "diffEvent.h"
#include "structs.h"

//extern void qt_x11_set_global_double_buffer(bool);
//qt_x11_set_global_double_buffer(false);

extern "C" {
#include "platform.h"
}

extern "C++" {
    void start_backend(struct gui_data *inst);
}

namespace PTerminal
{
Widget::Widget(struct ::gui_data* inst,QWidget* p):AbstractTerminalWidget(p),_inst(inst),_selectedText(""),_nextNow(0),_timer(this),_cursor(this),_text(this),_minDelta(120),_transform(QTransform::fromScale(1,1))
{
    setFocusPolicy(Qt::StrongFocus);
    setFont(QFont(platform_default_fontspec("Font")->name));
    setAttribute(Qt::WA_OpaquePaintEvent);
    QWidget::setCursor(QCursor(Qt::IBeamCursor));
    //setFocus(Qt::ActiveWindowFocusReason);
    QObject::connect(&_timer,SIGNAL(timeout()),SLOT(timeout()));
}

Widget::~Widget()
{
    foreach(int id,_fdObservers.keys())
    {
        QSocketNotifier* sn=_fdObservers.take(id);
        QObject::disconnect(sn,SIGNAL(activated(int)),this,0);
        delete sn;
    }
}

void
Widget::init()
{
    setFont(*_inst->fonts[0]);
    _text.init();
    _cursor.setShape(conf_get_int(_inst->conf, CONF_cursor_type));
    removeEventFilter(this);
    installEventFilter(this);
}

void
Widget::scroll(int lines)
{
    if(lines>=_inst->term->rows-1) return;
    QWidget::scroll(0,-(lines*_inst->font_height*_transform.m22()));
}

void
Widget::setCursor(int ,int ,const QString& text,unsigned long attr)
{
    _cursor.setText(decode(text),attr);
}

void
Widget::insertText(int col,int line,const QString& text,unsigned long /*attr*/)
{
    int x=col*_inst->font_width;
    int y=line*_inst->font_height;
    update(_transform.mapRect(QRect(x,y,text.size()*_inst->font_width,_inst->font_height)));
}

void 
Widget::sendText(const QString& text) const
{
    ldisc_send(_inst->ldisc,text.toLocal8Bit().data(),text.size(),0);
}

void 
Widget::setSelectedText(const QString& text)
{
    _selectedText=text;
#ifdef Q_OS_WIN
    QApplication::clipboard()->setText(text);
#else
    QApplication::clipboard()->setText(text,QClipboard::Selection);
#endif
}

void
Widget::requestPaste()
{
    paste();
}

void
Widget::contextMenu(QMenu* menu) const
{
    if(!menu) return;
    QAction* a;
    a=menu->addAction(tr("&Copy"),this,SLOT(copy()));
    a->setEnabled(canCopy());
    a=menu->addAction(tr("&Paste"),this,SLOT(paste()));
    a->setEnabled(canPaste());
    menu->addSeparator();
    a=menu->addAction(tr("Copy &All"),this,SLOT(copyAll()));
    menu->addSeparator();
    a=menu->addAction(tr("C&lear Scrollback"),this,SLOT(clearScrollback()));
    a=menu->addAction(tr("Rese&t Terminal"),this,SLOT(reset()));
    a=menu->addAction(tr("&Restart Session"),this,SLOT(restart()));
}

#ifndef Q_OS_WIN
int 
Widget::registerFd(int fd,int rwx)
{
    int pId=(fd+1)<<3; // fd start at 0
    QSocketNotifier* sn;
    if(rwx&1)
    {
        int rId=pId+QSocketNotifier::Read;
        Q_ASSERT(!_fdObservers.contains(rId));
        sn=new QSocketNotifier(fd,QSocketNotifier::Read);
        _fdObservers[rId]=sn;
        QObject::connect(sn,SIGNAL(activated(int)),this,SLOT(fdReadInput(int)));
    }
    if(rwx&2)
    {
        int wId=pId+QSocketNotifier::Write;
        Q_ASSERT(!_fdObservers.contains(wId));
        sn=new QSocketNotifier(fd,QSocketNotifier::Write);
        _fdObservers[wId]=sn;
        QObject::connect(sn,SIGNAL(activated(int)),this,SLOT(fdWriteInput(int)));
    }
#ifndef Q_OS_MAC
    if(rwx&4)
    {
        int eId=pId+QSocketNotifier::Exception;
        Q_ASSERT(!_fdObservers.contains(eId));
        sn=new QSocketNotifier(fd,QSocketNotifier::Exception);
        _fdObservers[eId]=sn;
        /* Disable exception notifier if rlogin, because get fatal error "invalid arguments". */
        if(conf_get_int(_inst->conf, CONF_protocol)==PROT_RLOGIN)
        {
            sn->setEnabled(false);
        }
        QObject::connect(sn,SIGNAL(activated(int)),this,SLOT(fdExceptionInput(int)));
    }
#endif
    return pId+rwx; // id
}

void
Widget::releaseFd(int id)
{
    int pId=id>>3<<3;
    QSocketNotifier* sn;
    if(id&1)
    {
        int rId=pId+QSocketNotifier::Read;
        sn=_fdObservers[rId];
        QObject::disconnect(sn,SIGNAL(activated(int)),this,SLOT(fdReadInput(int)));
        _fdObservers.remove(rId);
        sn->setEnabled(false);
        sn->deleteLater();
    }
    if(id&2)
    {
        int wId=pId+QSocketNotifier::Write;
        sn=_fdObservers[wId];
        QObject::disconnect(sn,SIGNAL(activated(int)),this,SLOT(fdWriteInput(int)));
        _fdObservers.remove(wId);
        sn->setEnabled(false);
        sn->deleteLater();
    }
#ifndef Q_OS_MAC
    if(id&4)
    {
        int eId=pId+QSocketNotifier::Exception;
        sn=_fdObservers[eId];
        QObject::disconnect(sn,SIGNAL(activated(int)),this,SLOT(fdExceptionInput(int)));
        _fdObservers.remove(eId);
        sn->setEnabled(false);
        sn->deleteLater();
    }
#endif
}
#endif

void 
Widget::timerChangeNotify(long ticks,long nextNow)
{
    _timer.stop();
    _timer.start(ticks);
    _nextNow=nextNow;
}

void
Widget::scrollTermTo(int lineNo)
{
    term_scroll(_inst->term,1,lineNo);
}

int
Widget::colDiff(int width) const
{
    int colDiffPixel=width-_inst->font_width*_inst->term->cols;
    int colDiff=colDiffPixel/_inst->font_width;
    if(colDiffPixel%_inst->font_width<0)
    {
        colDiff-=1;
    }
    return colDiff;
}

int
Widget::lineDiff(int height) const
{
    int lineDiffPixel=height-_inst->font_height*_inst->term->rows;
    int lineDiff=lineDiffPixel/_inst->font_height;
    if(lineDiffPixel%_inst->font_height<0)
    {
        lineDiff-=1;
    }
    return lineDiff;
}

void
Widget::resize(int width,int height)
{
    // it can happen that a resize comes faster then the post event is executed
    // and so a crash could be the result at ::paint for calc of line from drawing rect
    // because the rect is invalid, to avoid that a non executed post event will be removed
    QCoreApplication::removePostedEvents(this,QEvent::User);
    int yDiff=lineDiff(height);
    int xDiff=colDiff(width);
    if(yDiff==0 && xDiff==0) return;
    QCoreApplication::postEvent(this,new DiffEvent(xDiff,yDiff));
}

void
Widget::scale(int width,int height)
{
    _transform.reset();
    _transform.scale((qreal)width/_inst->font_width/_inst->term->cols,(qreal)height/_inst->font_height/_inst->term->rows);
    if(_transform.m11()!=1 || _transform.m22()!=1)
    {
        _inst->fonts[0]->setStyleStrategy(QFont::PreferAntialias);
    }
    else
    {
        _inst->fonts[0]->setStyleStrategy(QFont::NoAntialias);
    }
    setFont(*_inst->fonts[0]);
}

void*
Widget::realObject()
{
    return this;
}

QSize
Widget::sizeHint() const
{
    return QSize(_inst->font_width*_inst->term->cols,_inst->font_height*_inst->term->rows);
}


void 
Widget::copy()
{
    if(!canCopy()) return;
    QApplication::clipboard()->setText(_selectedText);
}

void 
Widget::paste()
{
    if(QApplication::clipboard()->mimeData(QClipboard::Selection))
    {
        sendText(QApplication::clipboard()->text(QClipboard::Selection));
    }
    else
    {
        sendText(QApplication::clipboard()->text());
    }
}

void 
Widget::copyAll()
{
    term_copyall(_inst->term);
    QApplication::clipboard()->setText(_selectedText);
}

void 
Widget::clearScrollback()
{
    term_clrsb(_inst->term);
}

void 
Widget::reset()
{
    term_pwron(_inst->term,TRUE);
    if (_inst->ldisc) ldisc_send(_inst->ldisc,NULL,0,0);
}

void 
Widget::restart()
{
    if(_inst->back)
    {
        term_pwron(_inst->term,FALSE);
        start_backend(_inst);
        _inst->exited=FALSE;
    }
}

void 
Widget::mouseMoveEvent(QMouseEvent* e)
{
    puttyTermMouse(e,MA_DRAG);
}

void 
Widget::mousePressEvent(QMouseEvent* e)
{
    if((e->button() & Qt::RightButton) && (conf_get_int(_inst->conf, CONF_mouse_is_xterm)==2)) return;
    if((!(e->button() & Qt::RightButton)) || (!(e->modifiers() & Qt::ControlModifier)))
    {
        puttyTermMouse(e,MA_CLICK);
    }
}

void 
Widget::mouseReleaseEvent(QMouseEvent* e)
{
    if((e->button() & Qt::RightButton) && (conf_get_int(_inst->conf, CONF_mouse_is_xterm)==2)) return;
    if((!(e->button() & Qt::RightButton)) || (!(e->modifiers() & Qt::ControlModifier)))
    {
        puttyTermMouse(e,MA_RELEASE);
    }
    QWidget::mouseReleaseEvent(e);
}

void 
Widget::mouseDoubleClickEvent(QMouseEvent* e)
{
    puttyTermMouse(e,MA_2CLK);
}

void 
Widget::contextMenuEvent(QContextMenuEvent* e)
{
    e->setAccepted(false);
}

void 
Widget::keyPressEvent(QKeyEvent* e) 
{
    switch(e->key())
    {
#ifdef Q_OS_WIN
        case Qt::Key_Return:
            if(e->modifiers()==Qt::AltModifier && conf_get_int(_inst->conf, CONF_fullscreenonaltenter))
            {
                QWidget* p=static_cast<QWidget*>(parent());
                if(p->isFullScreen())
                {
                    p->showNormal();
                }
                else
                {
                    p->showFullScreen();
                }
            }
            else
            {
                goto goAhead;
            }
            break;
#endif
        case Qt::Key_Insert:
            if(e->modifiers()==Qt::ShiftModifier)
            {
                paste();
            }
            break;
        case Qt::Key_PageUp:
            if(e->modifiers()==Qt::ShiftModifier)
            {
                scrollTermBy(-_inst->term->rows/2);
            }
            else if(e->modifiers()==Qt::ControlModifier)
            {
                scrollTermBy(-1);
            }
            break;
        case Qt::Key_PageDown:
            if(e->modifiers()==Qt::ShiftModifier)
            {
                scrollTermBy(_inst->term->rows/2);
            }                                        
            else if(e->modifiers()==Qt::ControlModifier)
            {
                scrollTermBy(1);
            }
            break;
        case Qt::Key_Tab:
            if(e->modifiers()==Qt::ShiftModifier)
            {
                sendKey('Z');
            }
            else
            {
                sendText(e->text());
            }
            break;
        case Qt::Key_Up:
            sendCursorKey('A',*e);
            break;
        case Qt::Key_Down:
            sendCursorKey('B',*e);
            break;
        case Qt::Key_Right:
            sendCursorKey('C',*e);
            break;
        case Qt::Key_Left:
            sendCursorKey('D',*e);
            break;
        case Qt::Key_Home:
            sendEditKey('1',*e);
            break;
        case Qt::Key_End:
            sendEditKey('4',*e);
            break;
        case Qt::Key_F1:
        case Qt::Key_F2:
        case Qt::Key_F3:
        case Qt::Key_F4:
        case Qt::Key_F5:
        case Qt::Key_F6:
        case Qt::Key_F7:
        case Qt::Key_F8:
        case Qt::Key_F9:
        case Qt::Key_F10:
        case Qt::Key_F11:
        case Qt::Key_F12:
        case Qt::Key_F13:
        case Qt::Key_F14:
        case Qt::Key_F15:
        case Qt::Key_F16:
        case Qt::Key_F17:
        case Qt::Key_F18:
        case Qt::Key_F19:
        case Qt::Key_F20:
            sendFunctionKey(e->key()-Qt::Key_F1,*e);
        default:
#ifdef Q_OS_WIN
goAhead:
#endif
#ifdef Q_OS_MAC
            //check for Ctrl keys on Mac-> MetaModifier was set
            if (e->modifiers()&Qt::MetaModifier && e->text().isEmpty()
                && e->key()>=Qt::Key_A && e->key()<=Qt::Key_Z)
            {
                sendText((QChar)(e->key()&0x1f));
            }
            else
#endif
            sendText(e->text());
    }
    term_seen_key_event(_inst->term);
    e->accept();
}

void 
Widget::wheelEvent(QWheelEvent* e)
{
    if(e->orientation()==Qt::Vertical)
    {
        int scrollLines=e->delta()/120*QApplication::wheelScrollLines();
        //Mighty Mouse, Magic Mouse and the trackpad has much higher granularity on the mouse wheel than standard miscs=120
        if(qAbs(e->delta())<QApplication::wheelScrollLines())
        {
            setMinDelta(qAbs(e->delta()));
            scrollLines=e->delta()/qAbs(e->delta());
        }
        else if(qAbs(e->delta())<120 || _minDelta<120)
        {
            setMinDelta(qAbs(e->delta()));
            scrollLines=e->delta()/QApplication::wheelScrollLines();
        }
        term_scroll(_inst->term,0,-scrollLines);
    }
    e->accept();
}

void
Widget::paintEvent(QPaintEvent* e)
{
    _painter.begin(this);
    _painter.scale(_transform.m11(),_transform.m22());
    if(!e->region().rects().isEmpty())
    {
        QVectorIterator<QRect> it(e->region().rects());
        while(it.hasNext())
        {
            paint(it.next());
        }
    }
    else
    {
        paint(e->rect());
    }
    _cursor.draw(_painter);
    _painter.end();
}

void
Widget::focusInEvent(QFocusEvent* )
{
    term_set_focus(_inst->term,TRUE);
    term_update(_inst->term);
}

void
Widget::focusOutEvent(QFocusEvent* )
{
    term_set_focus(_inst->term,FALSE);
    term_update(_inst->term);
}

void
Widget::customEvent(QEvent* e)
{
    DiffEvent* de=static_cast<DiffEvent*>(e);
    term_size(_inst->term,_inst->term->rows+de->yDiff(),_inst->term->cols+de->xDiff(),_inst->term->savelines);
    emit(termSizeChanged());
    updateGeometry();
}

bool 
Widget::focusNextPrevChild(bool)
{
    return false;
}

void 
Widget::sendKey(char k,char c) const
{
    sendText(QString("\033%1%2").arg(c).arg(k));
}

void 
Widget::sendFunctionKey(int i,const QKeyEvent& e) const
{
    if(conf_get_int(_inst->term->conf, CONF_funky_type) == FUNKY_XTERM && i < 4)
    {
        if(_inst->term->vt52_mode)
        {
            sendText(QString("\033%1").arg((QChar)(i+'P')));
        } 
        else
        {
            sendText(QString("\033O%1").arg((QChar)(i+'P')));
        }
        return;
    }
    if(conf_get_int(_inst->term->conf, CONF_funky_type) == FUNKY_SCO)
    {
        if(i<12)
        {
            static char const codes[] = "MNOPQRSTUVWX" "YZabcdefghij" "klmnopqrstuv" "wxyz@[\\]^_`{";
            if(e.modifiers()&Qt::ShiftModifier) i+=12;
            if(e.modifiers()&Qt::ControlModifier) i+=24;
            sendText(QString("\033[%1").arg((QChar)(codes[i])));
        }
        return;
    }
    if((e.modifiers()&Qt::ShiftModifier) && i<10) i+=10;
    if((_inst->term->vt52_mode || conf_get_int(_inst->term->conf, CONF_funky_type) == FUNKY_VT100P) && i < 12)
    {
        if(_inst->term->vt52_mode)
        {
            sendText(QString("\033%1").arg((QChar)('P'+i)));
        }
        else
        {
            sendText(QString("\033O%1").arg((QChar)('P'+i)));
        }
        return;
    }
    int code;
    switch(i/5)
    {
        case 0:
            if(conf_get_int(_inst->term->conf, CONF_funky_type) == FUNKY_LINUX)
            {
                sendText(QString("\033[[%1").arg((QChar)(i+'A')));
                return;
            }
            code=i+11;
            break;
        case 1:
            code=i+12;
            break;
        case 2:
            code=i+13;
            if(i%5==4) code+=1;
            break;
        case 3:
            code=i+15;
            if(i%5==0) code-=1;
            break;
        default:
            code=0;

    }
    sendText(QString("\033[%1~").arg(code));
}

void 
Widget::sendCursorKey(char k,const QKeyEvent& ) const
{
    sendKey(k,(_inst->term->app_cursor_keys&&!conf_get_int(_inst->conf, CONF_no_applic_c))?'O':'[');
}

void 
Widget::sendEditKey(char k,const QKeyEvent& e) const
{
    if(conf_get_int(_inst->term->conf, CONF_rxvt_homeend) && e.key()==Qt::Key_Home)
    {
        sendText("\033[H");
        return;
    }
    if(conf_get_int(_inst->term->conf, CONF_rxvt_homeend) && e.key()==Qt::Key_End)
    {
        sendText("\033Ow");
        return;
    }
    sendText(QString("\033[%1~").arg(k));
}

void
Widget::scrollTermBy(int lineCount)
{
    term_scroll(_inst->term,0,lineCount);
}

#ifdef Q_OS_WIN
QString 
Widget::decode(const QString& text) const
{
    QString s;
    for(int i=0;i<text.size();++i)
    {
        if((text[i].unicode()&CSET_MASK)==CSET_ACP)
        {
            s.append(_inst->term->ucsdata->unitab_font[text[i].cell()]);
        }
        else if((text[i].unicode()&CSET_MASK)==CSET_OEMCP)
        {
            s.append(_inst->term->ucsdata->unitab_oemcp[text[i].cell()]);
        }
        else if(text[i].unicode()>=0x23ba && text[i].unicode()<=0x23bd)
        {
            // still something to do
            s.append(_inst->term->ucsdata->unitab_xterm['q']);
        }
        else
        {
            s.append(text[i]);
        }
    }
    return s;
}
#else
const QString&
Widget::decode(const QString& text) const
{
    return text;
}
#endif

QFont
Widget::font(unsigned long attr) const
{
    QFont f(QWidget::font());
    f.setUnderline(attr&ATTR_UNDER);
    f.setWeight((attr&ATTR_BOLD)?QFont::Bold:QFont::Normal);
    return f;
}

const QColor& 
Widget::fgColor(unsigned long attr,bool cursor) const
{
    int index=(attr & ATTR_REVERSE)?(attr&ATTR_BGMASK)>>ATTR_BGSHIFT:(attr&ATTR_FGMASK)>>ATTR_FGSHIFT;
    if ((attr & TATTR_ACTCURS))
    {
        index=cursor?260:256;
    }
    else if(conf_get_int(_inst->conf, CONF_bold_style) && (attr & ATTR_BOLD))
    {
	if(index<16) 
        {
            index |= 8;
        }
	else if(index>=256)
        {
            index |= 1;
        }
    }
    return _inst->cols[index];
}

const QColor& 
Widget::bgColor(unsigned long attr,bool cursor) const
{
    int index=(attr & ATTR_REVERSE)?(attr&ATTR_FGMASK)>>ATTR_FGSHIFT:(attr&ATTR_BGMASK)>>ATTR_BGSHIFT;
    if ((attr & TATTR_ACTCURS))
    {
        index=cursor?261:258;
    }
    else if(conf_get_int(_inst->conf, CONF_bold_style) && (attr & ATTR_BLINK))
    {
	if(index<16)
        {
            index |= 8;
        }
	else if(index>=256) 
        {
            index |= 1;
        }
    }
    return _inst->cols[index];
}

Mouse_Button 
Widget::puttyMouseButton(Qt::MouseButton button) const
{
    switch(button)
    {
        case Qt::LeftButton:
            return MBT_LEFT;
        case Qt::RightButton:
            return MBT_RIGHT;
        case Qt::MidButton:
        default:
            return MBT_MIDDLE;
    }
}

Mouse_Button 
Widget::puttyMouseButtonTranslate(Qt::MouseButton button) const
{
    switch(button)
    {
        case Qt::LeftButton:
            return MBT_SELECT;
        case Qt::RightButton:
#ifdef Q_OS_WIN
            return conf_get_int(_inst->conf, CONF_mouse_is_xterm) == 1 ? MBT_EXTEND : MBT_PASTE;
#else
            return MBT_EXTEND;
#endif
        case Qt::MidButton:
        default:
#ifdef Q_OS_WIN
            return conf_get_int(_inst->conf, CONF_mouse_is_xterm) == 1 ? MBT_PASTE : MBT_EXTEND;
#else
            return MBT_PASTE;
#endif
    }
}

void 
Widget::puttyTermMouse(QMouseEvent* e,Mouse_Action action) const
{
    int col=(e->x()-conf_get_int(_inst->conf, CONF_window_border))/_inst->font_width;
    int line=(e->y()-conf_get_int(_inst->conf, CONF_window_border))/_inst->font_height;
    int shift=e->modifiers() & Qt::ShiftModifier;
    int ctrl=e->modifiers() & Qt::ControlModifier;
    int alt=e->modifiers() & Qt::AltModifier;
    Qt::MouseButton button=(e->button()!=Qt::NoButton)?e->button():mouseMoveButton(e->buttons());
    term_mouse(_inst->term
                ,puttyMouseButton(button)
                ,puttyMouseButtonTranslate(button)
                ,action
                ,col,line,shift,ctrl,alt);
}

Qt::MouseButton 
Widget::mouseMoveButton(Qt::MouseButtons buttons) const
{
    if(buttons & Qt::RightButton) 
    {
        return Qt::RightButton;
    }
    if(buttons & Qt::MidButton) 
    {
        return Qt::MidButton;
    }
    return Qt::LeftButton;
}

bool
Widget::canCopy() const
{
    return (!(_inst->term->selstart.x==0 && _inst->term->selstart.y==0 && _inst->term->selend.x==0 && _inst->term->selend.y==0) );
}

bool
Widget::canPaste() const
{
    const QMimeData* md=QApplication::clipboard()->mimeData();
    if(md)
    {
        return md->hasText();
    }
    return false;
}

void
Widget::paint(const QRect& re)
{
    QRect rect=_transform.inverted().mapRect(re);
    int sCol=rect.x()/_inst->font_width;
    int eCol=sCol+rect.width()/_inst->font_width;
    int sLine=rect.y()/_inst->font_height;
    int eLine=sLine+rect.height()/_inst->font_height;
    QString text;
    QRect r;
    for(int line=sLine;line<eLine;++line)
    {
        unsigned long attr=_inst->term->disptext[line]->chars[sCol].attr;
        text.clear();
        r.setY(line*_inst->font_height);
        r.setX(sCol*_inst->font_width);
        QChar lc;
        QChar c;
        bool flush=false;
        for(int col=sCol;col<eCol;++col)
        {
            c=((int)_inst->term->disptext[line]->chars[col].chr);
            if(_text.isLdc(c))
            {
                if(lc.isNull())
                {
                    lc=c;
                    flush=text.isEmpty()?false:true;
                }
                else if(lc==c)
                {
                    flush=false;
                }
                else
                {
                    lc=c;
                    flush=true;
                }
            }
            else
            {
                if(!lc.isNull())
                {
                    lc=0;
                    flush=true;
                }
                else
                {
                    flush=false;
                }
            }
            if(((_inst->term->disptext[line]->chars[col].attr & ~TATTR_COMBINING) == (attr & ~TATTR_COMBINING)) && !flush)
            {
                text.append(c);
            }
            else
            {
                r.setWidth(text.size()*_inst->font_width);
                r.setHeight(_inst->font_height);
                _text.draw(_painter,r,decode(text),attr);

                r.setX(col*_inst->font_width);
                text.clear();
                text.append(c);
                attr=_inst->term->disptext[line]->chars[col].attr;
            }
        }
        r.setWidth(text.size()*_inst->font_width);
        r.setHeight(_inst->font_height);
        _text.draw(_painter,r,decode(text),attr);
    }
}

bool
Widget::eventFilter(QObject* o,QEvent* e)
{
    switch(e->type())
    {
        /*
         * remove resize events for this widget because of resize this widget 
         * must be triggerd by the parent widget via resize(int,int) call.
         */
        case QEvent::Resize:
            return true;
        default:
            ;
    }
    if(conf_get_int(_inst->conf, CONF_hide_mouseptr))
    {
        switch(e->type())
        {
            case QEvent::KeyPress:
                if(cursor().shape()!=Qt::BlankCursor) QWidget::setCursor(QCursor(Qt::BlankCursor));
                break;
            case QEvent::FocusIn: 
            case QEvent::MouseButtonPress: 
            case QEvent::MouseButtonRelease: 
            case QEvent::MouseMove: 
                if(cursor().shape()!=Qt::IBeamCursor) QWidget::setCursor(QCursor(Qt::IBeamCursor));
                break;
            default:
                ;
        }
    }
    return QObject::eventFilter(o,e);
}

void
Widget::timeout()
{
    unsigned long next;
    if(run_timers(_nextNow,&next))
    {
        long ticks=next-GETTICKCOUNT();
        _nextNow=next;
        _timer.start(ticks>0?ticks:1);
    }
}

#ifndef Q_OS_WIN
void
Widget::fdReadInput(int fd)
{
     select_result(fd,1);
}

void
Widget::fdWriteInput(int fd)
{
     select_result(fd,2);
}

#ifndef Q_OS_MAC
void
Widget::fdExceptionInput(int fd)
{
     select_result(fd,4);
}
#endif
#endif

void
Widget::setMinDelta(int value)
{
    if(value<_minDelta) 
    {
        _minDelta=value;
    }
}
}

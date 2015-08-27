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
#ifndef _TERMINAL_WIDGET_H
#define _TERMINAL_WIDGET_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QHash>
#include <QSocketNotifier>
#include <QTransform>
#include "abstractTerminalWidget.h"
#include "structs.h"
#include "terminalCursor.h"
#include "terminalText.h"

extern "C" {
#include <putty.h>
}

class QPaintEvent;
namespace PTerminal
{
class Widget:public AbstractTerminalWidget
{
    Q_OBJECT
    public:
    Widget(struct ::gui_data* inst,QWidget* p=0);
    virtual ~Widget();
    virtual void init();
    virtual void setCursor(int col,int line,const QString& text,unsigned long attr);
    virtual void insertText(int col,int line,const QString& text,unsigned long attr);
    virtual void sendText(const QString& text) const;
    virtual void setSelectedText(const QString& text);
    virtual void requestPaste();
    virtual void contextMenu(QMenu* menu) const;
#ifndef Q_OS_WIN
    virtual int registerFd(int fd,int rwx);
    virtual void releaseFd(int id);
#endif
    virtual void timerChangeNotify(long ticks,long nextNow);
    virtual int colDiff(int width) const;
    virtual int lineDiff(int height) const;
    virtual void resize(int width,int height);
    virtual void scale(int width,int height);
    virtual void* realObject();

    virtual QSize sizeHint() const;

    signals:
    void termSizeChanged();

    public slots:
    virtual void copy();
    virtual void paste();
    virtual void copyAll();
    virtual void clearScrollback();
    virtual void reset();
    virtual void restart();
    virtual void scroll(int lines);
    virtual void scrollTermTo(int lineNo);

    protected:
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void mouseDoubleClickEvent(QMouseEvent* e);
    virtual void wheelEvent(QWheelEvent* e);
    virtual void contextMenuEvent(QContextMenuEvent* e);
    virtual void keyPressEvent(QKeyEvent* e);
    virtual void paintEvent(QPaintEvent *e);
    virtual void focusInEvent(QFocusEvent *e);
    virtual void focusOutEvent(QFocusEvent *e);
    virtual void customEvent(QEvent *e);
    virtual bool focusNextPrevChild(bool next);

    //helper
    virtual void sendKey(char k,char c='[') const;
    virtual void sendFunctionKey(int i,const QKeyEvent& e) const;
    virtual void sendCursorKey(char k,const QKeyEvent& e) const;
    virtual void sendEditKey(char k,const QKeyEvent& e) const;
    virtual void scrollTermBy(int lineCount);
#ifdef Q_OS_WIN
    virtual QString decode(const QString& text) const;
#else
    virtual const QString& decode(const QString& text) const;
#endif
    virtual QFont font(unsigned long attr) const;
    virtual const QColor& fgColor(unsigned long attr,bool cursor=false) const;
    virtual const QColor& bgColor(unsigned long attr,bool cursor=false) const;
    virtual Mouse_Button puttyMouseButton(Qt::MouseButton b) const;
    virtual Mouse_Button puttyMouseButtonTranslate(Qt::MouseButton b) const;
    virtual void puttyTermMouse(QMouseEvent* e,Mouse_Action a) const;
    virtual Qt::MouseButton mouseMoveButton(Qt::MouseButtons buttons) const;
    virtual bool canCopy() const;
    virtual bool canPaste() const;
    virtual void paint(const QRect& rect);
    virtual bool eventFilter(QObject* o,QEvent* e);

    protected slots:
    virtual void timeout();
#ifndef Q_OS_WIN
    virtual void fdReadInput(int);
    virtual void fdWriteInput(int);
#ifndef Q_OS_MAC
    virtual void fdExceptionInput(int);
#endif 
#endif

    private:
    void setMinDelta(int value);

    private:
    struct ::gui_data* _inst;
    //QPainter _painter;
    QString _selectedText;
    int _nextNow;
    QTimer _timer;
    QHash<int,QSocketNotifier*> _fdObservers;
    Cursor _cursor;
    Text _text;
    bool _repaintCursor;
    bool _repaintText;
    int _minDelta;
    friend class Cursor;
    friend class Text;
    QPainter _painter;
    QTransform _transform;
};
}

#endif

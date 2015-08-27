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
#ifndef _ABSTRACT_TERMINAL_WIDGET_H_
#define _ABSTRACT_TERMINAL_WIDGET_H_

#include <QWidget>

class QMenu;
class AbstractTerminalWidget:public QWidget
{
    public:
    AbstractTerminalWidget(QWidget* p=0):QWidget(p) {}
    virtual ~AbstractTerminalWidget() {}
    virtual void init()=0;
    virtual void scroll(int lines)=0;
    virtual void setCursor(int x,int y,const QString& text,unsigned long attr)=0;
    virtual void insertText(int x,int y,const QString& ,unsigned long attr)=0;
    virtual void sendText(const QString& text) const=0;
    virtual void setSelectedText(const QString& text)=0;
    virtual void requestPaste()=0;
    virtual void contextMenu(QMenu* menu) const=0;
#ifndef Q_OS_WIN
    virtual int registerFd(int fd,int rwx)=0;
    virtual void releaseFd(int id)=0;
#endif
    virtual void timerChangeNotify(long ticks,long nextNow)=0;
    virtual void scrollTermTo(int lineNo)=0;
    virtual int colDiff(int width) const=0;
    virtual int lineDiff(int height) const=0;
    virtual void resize(int width,int height)=0;
    virtual void scale(int width,int height)=0;
    virtual void* realObject()=0;
};

#endif

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
#ifndef _TERMINAL_TEXT_H_
#define _TERMINAL_TEXT_H_

#include <QObject>
#include <QColor>
#include <QFont>
#include <QRect>
#include <QHash>
#include <QPolygon>

class QPainter;
namespace PTerminal
{
class Widget;
class Text:public QObject
{
    Q_OBJECT
    Q_ENUMS(LDC)
    public:
    enum LDC {begin=0x2500,
              lh=0x2500,//hh=0x2501,
              lv=0x2502,//lh=0x2503,
              ldlr=0x250c,//-0x250F
              ldll=0x2510,//-0x2513
              lulr=0x2514,//-0x2517
              lull=0x2518,//-0x251B
              lvlr=0x251C,//-0x2523
              lvll=0x2524,//-0x252B
              lhld=0x252C,//-0x2533
              lhlu=0x2534,//-0x253b
              lhlv=0x253C,//-0x254b
              end=0x254B};

    Text(Widget* p=0);
    void init();
    bool isLdc(const QChar& c) const;
    void draw(QPainter& p,const QRect& r,const QString& text,unsigned long attr);

    protected:
    Widget* widget() const;

    public:
    QHash<LDC,QPolygon> _ldcMap;
    QPoint _fontSize;
};
}

#endif

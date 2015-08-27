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
#include "terminalText.h"
#include <QWidget>
#include <QPainter>
#include <QString>
#include <QColor>
#include <QFont>
#include <QMetaEnum>
#include "terminalWidget.h"

namespace PTerminal
{
Text::Text(Widget* p):QObject(p),_fontSize(0,0)
{
}

void
Text::init()
{
    _fontSize.setX((qreal)widget()->_inst->font_width);
    _fontSize.setY((qreal)widget()->_inst->font_height);

    _ldcMap.clear();

    QPolygon polygon;

    // -
    polygon << QPoint(0,_fontSize.y()/2) << QPoint(_fontSize.x(),_fontSize.y()/2);
    _ldcMap.insert(lh,polygon);

    // |
    polygon.clear();
    polygon << QPoint(_fontSize.x()/2,0) << QPoint(_fontSize.x()/2,_fontSize.y());
    _ldcMap.insert(lv,polygon);

    //  _
    // |
    polygon.clear();
    polygon << QPoint(_fontSize.x()/2,_fontSize.y()/2) << QPoint(_fontSize.x()/2,_fontSize.y());
    polygon << QPoint(_fontSize.x()/2,_fontSize.y()/2) << QPoint(_fontSize.x(),_fontSize.y()/2);
    _ldcMap.insert(ldlr,polygon);

    //  _
    //  | 
    polygon.clear();
    polygon << QPoint(_fontSize.x()/2,_fontSize.y()/2) << QPoint(_fontSize.x()/2,_fontSize.y());
    polygon << QPoint(0,_fontSize.y()/2) << QPoint(_fontSize.x()/2,_fontSize.y()/2);            
    _ldcMap.insert(ldll,polygon);

    // |_
    polygon.clear();
    polygon << QPoint(_fontSize.x()/2,0) << QPoint(_fontSize.x()/2,_fontSize.y()/2);
    polygon << QPoint(_fontSize.x()/2,_fontSize.y()/2) << QPoint(_fontSize.x(),_fontSize.y()/2);
    _ldcMap.insert(lulr,polygon);

    // _|
    polygon.clear();
    polygon << QPoint(_fontSize.x()/2,0) << QPoint(_fontSize.x()/2,_fontSize.y()/2);
    polygon << QPoint(0,_fontSize.y()/2) << QPoint(_fontSize.x()/2,_fontSize.y()/2);
    _ldcMap.insert(lull,polygon);

    // |-
    polygon.clear();
    polygon << QPoint(_fontSize.x()/2,0) << QPoint(_fontSize.x()/2,_fontSize.y());
    polygon << QPoint(_fontSize.x()/2,_fontSize.y()/2) << QPoint(_fontSize.x(),_fontSize.y()/2);
    _ldcMap.insert(lvlr,polygon);

    // -|
    polygon.clear();
    polygon << QPoint(_fontSize.x()/2,0) << QPoint(_fontSize.x()/2,_fontSize.y());
    polygon << QPoint(0,_fontSize.y()/2) << QPoint(_fontSize.x()/2,_fontSize.y()/2);
    _ldcMap.insert(lvll,polygon);

    // _
    // |
    polygon.clear();
    polygon << QPoint(0,_fontSize.y()/2) << QPoint(_fontSize.x(),_fontSize.y()/2);
    polygon << QPoint(_fontSize.x()/2,_fontSize.y()/2) << QPoint(_fontSize.x()/2,_fontSize.y());
    _ldcMap.insert(lhld,polygon);

    // |
    // - 
    polygon.clear();
    polygon << QPoint(0,_fontSize.y()/2) << QPoint(_fontSize.x(),_fontSize.y()/2);
    polygon << QPoint(_fontSize.x()/2,0) << QPoint(_fontSize.x()/2,_fontSize.y()/2);
    _ldcMap.insert(lhlu,polygon);

    // +
    polygon.clear();
    polygon << QPoint(0,_fontSize.y()/2) << QPoint(_fontSize.x(),_fontSize.y()/2);
    polygon << QPoint(_fontSize.x()/2,0) << QPoint(_fontSize.x()/2,_fontSize.y());
    _ldcMap.insert(lhlv,polygon);
}

bool
Text::isLdc(const QChar& c) const
{
    return (c.unicode()>=begin && c.unicode()<=end);
}

void
Text::draw(QPainter& p,const QRect& r,const QString& text,unsigned long attr)
{
    p.fillRect(r,widget()->bgColor(attr));
    p.setFont(widget()->font(attr));
    if(isLdc(text.at(0)))
    {
        p.setPen(QPen(widget()->fgColor(attr),(attr&ATTR_BOLD)?2:1));
        LDC key;
        QMetaEnum me=metaObject()->enumerator(metaObject()->indexOfEnumerator("LDC"));
        for(int i=2;i<me.keyCount();++i)
        {
            if(text.at(0).unicode()<me.value(i))
            {
                key=(LDC)me.value(i-1);
                break;
            }
        }
        if(text.at(0).unicode()==end)
        {
            key=lhlv;
        }
        const QPolygon& polygon=_ldcMap.value(key);
        for(int j=0;j<text.size();++j)
        {
            p.drawLines(polygon.translated(r.topLeft().x()+j*_fontSize.x(),r.topLeft().y()));
        }
    }
    else
    {
        p.setPen(widget()->fgColor(attr));
        QFontMetrics fm(widget()->font(attr));
        QRect br=fm.boundingRect(text);
        p.drawText(r.x(),r.y()-br.y(),text);
    }
}

Widget*
Text::widget() const
{
    return (Widget*)parent();
}

}

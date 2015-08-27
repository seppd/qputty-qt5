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
#include "terminalCursor.h"
#include <QWidget>
#include <QPainter>
#include "terminalWidget.h"

namespace PTerminal
{
Cursor::Cursor(Widget* parent):QObject(parent),_shape(Empty),_shapeWidth(2),_attr(0)
{
}

Cursor::~Cursor()
{
}

int
Cursor::col() const
{
    return widget()->_inst->term->dispcursx;
}

int
Cursor::line() const
{
    return widget()->_inst->term->dispcursy;
}

void
Cursor::setText(const QString& text,unsigned long attr)
{
    _text=text;
    _attr=attr;
    QRect r(col()*widget()->_inst->font_width,line()*widget()->_inst->font_height,widget()->_inst->font_width,widget()->_inst->font_height);
    switch(_shape)
    {
        case Block:
            _rect=r;
            break;
        case Underline:
            _rect=r.adjusted(0,r.height()-_shapeWidth,0,0);
            break;
        case Vertical:
            _rect=r.adjusted(0,0,-r.width()+_shapeWidth,0);
            break;
        default:
            _rect=r.adjusted(0,0,-r.width(),-r.height());
    }
    widget()->update(widget()->_transform.mapRect(_rect));
}

void
Cursor::setShape(int shape)
{
    if(shape==_shape) return;
    switch(shape)
    {
        case 0:
            _shape=Block;
            break;
        case 1:
            _shape=Underline;
            break;
        case 2:
            _shape=Vertical;
            break;
        default:
            _shape=Empty;
    }
}

void
Cursor::draw(QPainter& p)
{
    if(col()==-1 || line()==-1) return;
    p.setFont(((QWidget*)parent())->font());
    if(_attr&TATTR_PASCURS)
    {
        p.setPen(widget()->fgColor(_attr,true));
        p.drawRect(_rect.adjusted(0,0,-1,-1));
        p.setPen(widget()->fgColor(_attr,true));
        p.drawText(_rect,_text);
    }
    else if(_attr&TATTR_ACTCURS && widget()->_inst->term->cblinker)
    {
        p.fillRect(_rect,widget()->bgColor(_attr,true));
        p.setPen(widget()->fgColor(_attr,true));
        p.drawText(_rect,_text);
    }
}

Widget*
Cursor::widget() const
{
    return (Widget*)parent();
}
}

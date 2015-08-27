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
#include "puttyGridLayout.h"
#include <QWidget>
#include <QDebug>
extern "C" {
#include "putty.h"
#include "dialog.h"
}

// Putty's internal debug macro from misc.h breaks qDebug.
#if defined (debug) && !defined (QT_NO_DEBUG_OUTPUT)
#undef debug
#define debug debug
#endif

PuttyGridLayout::PuttyGridLayout(int colCount,int* colStretch):QGridLayout(),_colCount(colCount),_colNo(0),_rowNo(0)
{
    setContentsMargins(2,2,2,2);
    if(colStretch)
    {
        for(int i=0;i<colCount;++i)
        {
            setColumnStretch(i,colStretch[i]);
        }
    }
}

void 
PuttyGridLayout::add(QObject* x,int colNo)
{
    if(qobject_cast<QLayout*>(x))
    {
        addLayout((QLayout*)x,colNo);
    }
    else if(qobject_cast<QWidget*>(x))
    {
        addWidget((QWidget*)x,colNo);
    }
    else
    {
        qDebug() << "adding" << x << "not supported!";
    }
}

void 
PuttyGridLayout::addLayout(QLayout* l,int colNo)
{
    int validColNo=validateColNo(colNo);
    updateRowNo(validColNo);
    QLayoutItem* li=itemAtPosition(_rowNo,validColNo);
    if(li && li->layout())
    {
        PuttyGridLayout* theLi=(PuttyGridLayout*)li->layout();
        ((QGridLayout*)li->layout())->addLayout(l,theLi->rowCount(),0,1,COLUMN_SPAN(colNo));

    }
    else
    {
        QGridLayout* g=new PuttyGridLayout(1);
        QGridLayout::addLayout(g,_rowNo,validColNo,1,COLUMN_SPAN(colNo));
        g->addLayout(l,0,0);
    }
}

void 
PuttyGridLayout::addWidget(QWidget* w,int colNo)
{
    int validColNo=validateColNo(colNo);
    updateRowNo(validColNo);
    QLayoutItem* li=itemAtPosition(_rowNo,validColNo);
    if(li && li->layout())
    {
        PuttyGridLayout* theLi=(PuttyGridLayout*)li->layout();
        ((QGridLayout*)li->layout())->addWidget(w,theLi->rowCount(),0,1,COLUMN_SPAN(colNo));
    }
    else
    {
        QGridLayout* g=new PuttyGridLayout(1);
        QGridLayout::addLayout(g,_rowNo,validColNo,1,COLUMN_SPAN(colNo));
        g->addWidget(w,0,0);
    }
}

int
PuttyGridLayout::validateColNo(int colNo)
{
    if(colNo>0xffff)
    {
        return COLUMN_START(colNo);
    }
    if(colNo>=_colCount)
    {
        colNo%=_colCount;
    }
    return colNo;
}

void
PuttyGridLayout::updateRowNo(int colNo)
{
    if(_colNo>colNo)
    {
        ++_rowNo;
    }
    _colNo=colNo;
}

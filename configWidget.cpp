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
#include "configWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLayout>
#include <QGroupBox>
#include <QStringList>
#include <QLineEdit>
#include <QRadioButton>
#include <QPushButton>
#include <QCheckBox>
#include <QListWidget>
#include <QComboBox>
#include <QButtonGroup>
#include <QLabel>
#include <QFileDialog>
#include <QFontDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QFontDatabase>
#include <QDebug>

#include "puttyGridLayout.h"

extern "C" {
#include "dialog.h"
}

// Putty's internal debug macro from misc.h breaks qDebug.
#if defined (debug) && !defined (QT_NO_DEBUG_OUTPUT)
#undef debug
#define debug debug
#endif

extern void dlg_init(struct putty_dlgparam *dp);

bool
ConfigWidget::isFixedFont(const QFont& font)
{
    QFontDatabase fdb;
    return fdb.isFixedPitch(font.family(),fdb.styleString(font));
}

bool
ConfigWidget::isULDFont(const QFont& font)
{
    QFontMetrics fm(font);
    return (fm.averageCharWidth()==fm.width(0x2500));
}

bool
ConfigWidget::isValidFont(const QFont& font)
{
    return isFixedFont(font)&&isULDFont(font);
}

QFont 
ConfigWidget::findValidFont(const QString& text)
{
    QFontDatabase fdb;
    QStringList families=fdb.families();
    if(!text.isEmpty())
    {
        QFont f;
        f.fromString(text);
        if(isFixedFont(f) && families.contains(f.family())) return f;
    }
    QStringList fonts;
    fonts << "Courier New" << "Andale Mono" << "Monospace";
    for(int i=0;i<fonts.size();++i)
    {
        QFont f;
        f.fromString(fonts.at(i));
        if(families.contains(f.family())) return f;
    }
    return QFont();
}

ConfigWidget::ConfigWidget(bool buttonBox):QWidget(),_conf(0),_parent(0)
{
    _category.setColumnCount(1);
    _category.headerItem()->setText(0,tr("Category"));
    connect(&_category,SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),SLOT(changePage(QTreeWidgetItem*,QTreeWidgetItem*)));

    QHBoxLayout *hBox=new QHBoxLayout();
    hBox->setContentsMargins(QMargins());
    hBox->addWidget(&_category);
    hBox->addWidget(&_pages,1);

    QFrame* hLine=new QFrame();
    hLine->setFrameShape(QFrame::HLine);

    QVBoxLayout *vBox=new QVBoxLayout();
    vBox->setContentsMargins(QMargins());
    vBox->addLayout(hBox);
    vBox->addStretch(1);
    if(buttonBox)
    {
        vBox->addWidget(hLine);
        vBox->addWidget(&_buttonBox);
    }
    setLayout(vBox);

    connect(&_fileSm,SIGNAL(mapped(QWidget*)),SLOT(selectFile(QWidget*)));
    connect(&_fontSm,SIGNAL(mapped(QWidget*)),SLOT(selectFont(QWidget*)));
#ifdef Q_OS_WIN
    _excludes.insert("Window/Behaviour//Window closes on ALT-F4");
    _excludes.insert("Window/Behaviour//System menu appears on ALT-Space");
    _excludes.insert("Window/Behaviour//System menu appears on ALT alone");
    _excludes.insert("Terminal/Bell/Set the style of bell/Action to happen when a bell occurs:->Beep using the PC speaker");
    _excludes.insert("Terminal/Bell/Set the style of bell/Action to happen when a bell occurs:->Play a custom sound file");
    _excludes.insert("Terminal/Bell/Set the style of bell/Custom sound file to play as a bell:");
    _excludes.insert("Terminal/Bell/Set the style of bell/Taskbar/caption indication on bell:");
#endif
}

ConfigWidget::~ConfigWidget()
{
    while(!_privData.isEmpty())
    {
        delete[] _privData.begin().value();
        _privData.erase(_privData.begin());
    }
}

void
ConfigWidget::fill(Conf* conf,QDialog* parent,int midSession,int protCfgInfo)
{
    _conf=conf;
    if(!conf) return;
    struct controlbox* ctrlbox=ctrl_new_box();
    setup_config_box(ctrlbox,midSession,conf_get_int(_conf, CONF_protocol),protCfgInfo);
#ifdef Q_OS_WIN
    WId w=winId();
    win_setup_config_box(ctrlbox,&w,has_help(),midSession,conf_get_int(_conf, CONF_protocol));
#endif
#ifdef Q_OS_UNIX
    unix_setup_config_box(ctrlbox,midSession,conf_get_int(_conf, CONF_protocol));
    gtk_setup_config_box(ctrlbox,midSession,0);
#endif

    fillButtonBox(parent,ctrlbox->ctrlsets[0]);
    int longest=0;
    for(int index=1;index<ctrlbox->nctrlsets;++index) 
    //for(int index=73;index<74;++index) 
    {
        struct controlset *c = ctrlbox->ctrlsets[index];
        //qDebug() << "-------------------" << c->pathname << "-" << c->boxtitle << "-" << c->boxname;
        if(!_stringToItem.contains(c->pathname))
        {
            createTreeItemAndWidgetForString(c->pathname);
            int l=strlen(c->pathname);
            if(l>longest)
            {
                longest=l;
            }
        }
        fillPage(c);
    }
    QListIterator<QWidget*> it(_itemToPage.values());
    while(it.hasNext())
    {
        QGridLayout* g=(QGridLayout*)it.next()->layout();
        g->addItem(new QSpacerItem(0,-1,QSizePolicy::Minimum,QSizePolicy::Expanding),g->rowCount(),0);
    }
    _category.expandAll();
    _category.setMinimumWidth(_category.fontMetrics().averageCharWidth()*longest);
    _category.setMaximumWidth(_category.minimumWidth());
}

QString
ConfigWidget::toShortCutString(const char* label,char shortcut) const
{
    QString s(label);
    s.insert(s.indexOf(shortcut,0,Qt::CaseInsensitive),'&');
    return s;
}

void 
ConfigWidget::setCheckBox(union control* ctrl,int checked)
{
    ((QCheckBox*)_controlToWidget.values(ctrl)[0])->setChecked(checked);
}

int 
ConfigWidget::checkBox(union control* ctrl) const
{
     return ((QCheckBox*)_controlToWidget.values(ctrl)[0])->isChecked();
}

void 
ConfigWidget::setRadioButton(union control* ctrl,int which)
{
    int size=_controlToWidget.values(ctrl).size();
    ((QRadioButton*)_controlToWidget.values(ctrl)[size-which-1])->setChecked(true);
}

int 
ConfigWidget::radioButton(union control* ctrl) const
{
    QList<QWidget*> rb=_controlToWidget.values(ctrl);
    for(int i=0;i<rb.size();++i)
    {
        if(((QRadioButton*)rb[i])->isChecked()) return rb.size()-i-1;
    }
    return 0;
}
void 
ConfigWidget::setEditBox(union control* ctrl,const char* text)
{
    if(ctrl->editbox.has_list)
    {
        QComboBox* c=(QComboBox*)_controlToWidget.values(ctrl)[0];
        c->setCurrentIndex(c->findText(text,Qt::MatchFixedString));
    }
    else
    {
        ((QLineEdit*)_controlToWidget.values(ctrl)[0])->setText(text);
    }
}

char*
ConfigWidget::editBox(union control* ctrl) const
{
    QByteArray a;
    if(ctrl->editbox.has_list)
    {
        a=((QComboBox*)_controlToWidget.values(ctrl)[0])->currentText().toLocal8Bit();
    }
    else
    {
        a=((QLineEdit*)_controlToWidget.values(ctrl)[0])->text().toLocal8Bit();
    }
    return strdup(a.data());
}

void 
ConfigWidget::setFileBox(union control* ctrl,const char* text)
{
    ((QLineEdit*)_controlToWidget.values(ctrl)[0])->setText(text);
}

char*
ConfigWidget::fileBox(union control* ctrl) const
{
    QByteArray a=((QLineEdit*)_controlToWidget.values(ctrl)[0])->text().toLocal8Bit();
    return a.data();
}

void
ConfigWidget::listBoxClear(union control* ctrl)
{
    if(ctrl->listbox.height==0 || ctrl->generic.type==CTRL_EDITBOX)
    {
        ((QComboBox*)_controlToWidget.values(ctrl)[0])->clear();
    }
    else
    {
        ((QListWidget*)_controlToWidget.values(ctrl)[0])->clear();
    }
}

void 
ConfigWidget::listBoxDel(union control* ctrl,int index)
{
    if(ctrl->listbox.height==0 || ctrl->generic.type==CTRL_EDITBOX)
    {
        ((QComboBox*)_controlToWidget.values(ctrl)[0])->removeItem(index);
    }
    else
    {
        delete(((QListWidget*)_controlToWidget.values(ctrl)[0])->takeItem(index));
    }
}

void 
ConfigWidget::listBoxAdd(union control* ctrl,const char* text)
{
    if(ctrl->listbox.height==0 || ctrl->generic.type==CTRL_EDITBOX)
    {
        ((QComboBox*)_controlToWidget.values(ctrl)[0])->addItem(text);
    }
    else
    {
        ((QListWidget*)_controlToWidget.values(ctrl)[0])->addItem(text);
    }
}

void 
ConfigWidget::listBoxAddWithId(union control* ctrl,const char* text,int id)
{
    if(ctrl->listbox.height==0 || ctrl->generic.type==CTRL_EDITBOX)
    {
        ((QComboBox*)_controlToWidget.values(ctrl)[0])->addItem(text,id);
    }
    else
    {
        QListWidgetItem* item=new QListWidgetItem(text);
        item->setData(Qt::UserRole,id);
        ((QListWidget*)_controlToWidget.values(ctrl)[0])->addItem(item);
    }
}

int
ConfigWidget::listBoxGetId(union control* ctrl,int index)
{
    if(ctrl->listbox.height==0 || ctrl->generic.type==CTRL_EDITBOX)
    {
        return ((QComboBox*)_controlToWidget.values(ctrl)[0])->itemData(index).toInt();
    }
    else
    {
        return ((QListWidget*)_controlToWidget.values(ctrl)[0])->item(index)->data(Qt::UserRole).toInt();
    }
    return 0;
}

int
ConfigWidget::listBoxIndex(union control* ctrl)
{
    if(ctrl->listbox.height==0 || ctrl->generic.type==CTRL_EDITBOX)
    {
        return ((QComboBox*)_controlToWidget.values(ctrl)[0])->currentIndex();
    }
    else
    {
        return ((QListWidget*)_controlToWidget.values(ctrl)[0])->currentRow();
    }
    return 0;
}

void
ConfigWidget::listBoxSelect(union control* ctrl,int index)
{
    if(ctrl->listbox.height==0 || ctrl->generic.type==CTRL_EDITBOX)
    {
        return ((QComboBox*)_controlToWidget.values(ctrl)[0])->setCurrentIndex(index);
    }
    else
    {
        return ((QListWidget*)_controlToWidget.values(ctrl)[0])->setCurrentRow(index);
    }
}

void
ConfigWidget::setLabel(union control* ctrl,const char* text)
{
    if(_reversBuddy.contains(_controlToWidget.values(ctrl)[0]))
    {
        char shortCut=0;
        switch (ctrl->generic.type) 
        {
            case CTRL_BUTTON:
                shortCut=ctrl->button.shortcut;
                break;
            case CTRL_CHECKBOX:
                shortCut=ctrl->checkbox.shortcut;
                break;
            case CTRL_RADIO:
                shortCut=ctrl->radio.shortcut;
                break;
            case CTRL_EDITBOX:
                shortCut=ctrl->editbox.shortcut;
                break;
            case CTRL_FILESELECT:
                shortCut=ctrl->fileselect.shortcut;
                break;
            case CTRL_FONTSELECT:
                shortCut=ctrl->fontselect.shortcut;
                break;
            case CTRL_LISTBOX:
                shortCut=ctrl->listbox.shortcut;
                break;
            default:
                qDebug() << "This shouldn't happen";
                break;
        }
        _reversBuddy.value(_controlToWidget.values(ctrl)[0])->setText(toShortCutString(text,shortCut));
    }
    else
    {
        qDebug() << "no buddy relation!";
    }
}

void
ConfigWidget::refresh(union control* ctrl)
{
    if(ctrl)
    {
        refreshControl(ctrl);
    }
    else
    {
        QList<union control*> controls=_controlToWidget.uniqueKeys();
        for(int i=0;i<controls.size();++i)
        {
            refreshControl(controls[i]);
        }
    }
}
char* 
ConfigWidget::privData(union control* ctrl) const
{
    return _privData.value(ctrl);
}

char* 
ConfigWidget::setPrivData(union control* ctrl,size_t size)
{
    char* d=new char[size];
    _privData[ctrl]=d;
    return d;
}

void 
ConfigWidget::selectColor(union control* ctrl,int r,int g,int b)
{
    QColorDialog d(QColor(r,g,b),this);
    if(d.exec())
    {
        _selectedColor=d.selectedColor();
        callControlHandler(ctrl,EVENT_CALLBACK);
    }
}

void 
ConfigWidget::selectedColor(union control* ,int* r,int* g,int* b)
{
    *r=_selectedColor.red();
    *g=_selectedColor.green();
    *b=_selectedColor.blue();
}

void 
ConfigWidget::errorMessage(const char* text)
{
    QMessageBox::critical(this,"Error",text);
}

void 
ConfigWidget::accept() const
{
    if(_parent)
    {
        _parent->accept();
    }
}

void 
ConfigWidget::reject() const
{
    if(_parent)
    {
        _parent->reject();
    }
}

void
ConfigWidget::createTreeItemAndWidgetForString(const QString& p)
{
    if(_excludes.contains(p)) return; 
    QTreeWidgetItem* item;
    QStringList path=p.split('/');
    if(path.size()==1)
    {
        item=new QTreeWidgetItem(&_category);
        item->setText(0,p);
    }
    else
    {
        QStringList parentPath(path);
        parentPath.removeLast();
        QString pp=parentPath.join("/");
        if(!_stringToItem.contains(pp))
        {
            createTreeItemAndWidgetForString(pp);
        }
        item=new QTreeWidgetItem(_stringToItem[pp]);
        item->setText(0,path.last());
    }
    _stringToItem[p]=item;
    QWidget* w=new QWidget();
    w->setLayout(new PuttyGridLayout(1));
    _itemToPage[item]=w;
    _pageToItem[w]=item;
    _pages.addWidget(w);
}

void
ConfigWidget::fillButtonBox(QDialog* parent,struct controlset* cs)
{
    if(!parent) return;
    _parent=parent;
    for(int i=0;i<cs->ncontrols;++i)
    {
        union control *ctrl=cs->ctrls[i];
        switch (ctrl->generic.type) 
        {
            case CTRL_BUTTON:
                {
                QPushButton* b;
                if(ctrl->button.iscancel)
                {
                    b=_buttonBox.addButton(toShortCutString(ctrl->generic.label,ctrl->button.shortcut),QDialogButtonBox::RejectRole);
                    connect(&_buttonBox,SIGNAL(rejected()),parent,SLOT(reject()));
                }
                else if(QString(ctrl->generic.label)=="About")
                {
                    b=_buttonBox.addButton(toShortCutString(ctrl->generic.label,ctrl->button.shortcut),QDialogButtonBox::HelpRole);
                    connect(&_buttonBox,SIGNAL(clicked(QAbstractButton*)),this,SLOT(buttonClicked(QAbstractButton*)));
                }
                else
                {
                    b=_buttonBox.addButton(toShortCutString(ctrl->generic.label,ctrl->button.shortcut),QDialogButtonBox::AcceptRole);
                    connect(&_buttonBox,SIGNAL(accepted()),parent,SLOT(accept()));
                }
                if(ctrl->button.isdefault)
                {
                    //b->setDefault(true);
                }
                addRelation(b,ctrl);
                }
                break;
            case CTRL_COLUMNS:
                //qDebug() << "oops columns" << ctrl->columns.ncols;
                break;
            default:
                qDebug() << "oops ctrl not supported inside buttonbox!" << ctrl->generic.label << ctrl->generic.type;
        }
    }
}

void
ConfigWidget::fillPage(struct controlset* cs)
{
    if(_excludes.contains(cs->pathname)) return;
    QString excludeKey=QString("%1/%2").arg(cs->pathname).arg(cs->boxtitle);
    if(_excludes.contains(excludeKey)) return;
    QWidget* parent=_itemToPage[_stringToItem[cs->pathname]];
    if(!cs->boxname && cs->boxtitle)
    {
        ((PuttyGridLayout*)parent->layout())->add(new QLabel(cs->boxtitle),0);
    }
    else 
    {
        QGroupBox* g;
        if(cs->boxtitle)
        {
            g=new QGroupBox(cs->boxtitle);
        }
        else
        {
            g=new QGroupBox();
        }
        g->setLayout(new PuttyGridLayout(1));
        ((PuttyGridLayout*)parent->layout())->add(g,0);
        parent=g;
    }

    QStack<PuttyGridLayout*> layoutStack;
    layoutStack.push((PuttyGridLayout*)parent->layout());
    for(int i=0;i<cs->ncontrols;++i)
    {
        insertControl(cs->ctrls[i],layoutStack,excludeKey);
    }
    for(int i=0;i<cs->ncontrols;++i)
    {
        refreshControl(cs->ctrls[i],excludeKey);
    }
}

void 
ConfigWidget::addRelation(QWidget* w,union control* ctrl)
{
    _widgetToControl.insert(w,ctrl);
    _controlToWidget.insert(ctrl,w);
}

QLayout* 
ConfigWidget::buddyLayout(QLabel* l,QObject* o,int widgetWidth) const
{
    PuttyGridLayout* grid;
    if(widgetWidth==100)
    {
        grid=new PuttyGridLayout(1);
    }
    else
    {
        int p[2]={100-widgetWidth,widgetWidth};
        grid=new PuttyGridLayout(2,p);
    }
    grid->add(l,0);
    grid->add(o,1);
    return grid;
}

void 
ConfigWidget::insertControl(union control* ctrl,QStack<PuttyGridLayout*>& layoutStack,const QString& excludeKey)
{
    if(_excludes.contains(excludeKey+"/"+ctrl->generic.label)) return;
    switch (ctrl->generic.type) 
    {
        case CTRL_COLUMNS:
            if(ctrl->columns.ncols==1)
            {
                layoutStack.pop();
            }
            else
            {
                PuttyGridLayout* g=new PuttyGridLayout(ctrl->columns.ncols,ctrl->columns.percentages);
                layoutStack.top()->add(g,0);
                layoutStack.push(g);
            }
            break;
        case CTRL_FILESELECT:
            selectDialog(layoutStack.top(),ctrl,_fileSm,"Browse...");
            break;
        case CTRL_FONTSELECT:
            selectDialog(layoutStack.top(),ctrl,_fontSm,"Change...");
            break;
        case CTRL_TABDELAY:
            qDebug() << "tabdelay not implemented yet!";
            break;
        case CTRL_RADIO:
        {
            QVBoxLayout* vBox=new QVBoxLayout();
            QLabel* l;
            if(ctrl->radio.shortcut==NO_SHORTCUT)
            {
                l=new QLabel(ctrl->generic.label);
            }
            else
            {
                l=new QLabel(toShortCutString(ctrl->generic.label,ctrl->radio.shortcut));
            }
            vBox->addWidget(l);
            QBoxLayout* layout=vBox;
            if(ctrl->radio.ncolumns>ctrl->radio.nbuttons)
            {
                ctrl->radio.ncolumns=ctrl->radio.nbuttons;
            }
            QButtonGroup* bg=new QButtonGroup(vBox);
            for(int i=0;i<ctrl->radio.nbuttons;++i) 
            {
                if(_excludes.contains(excludeKey+"/"+ctrl->generic.label+"->"+ctrl->radio.buttons[i])) continue;
                if(i%ctrl->radio.ncolumns==0)
                {
                    layout=new QHBoxLayout();
                    vBox->addLayout(layout);
                }
                QRadioButton* b;
                if(ctrl->radio.shortcuts)
                {
                    b=new QRadioButton(toShortCutString(ctrl->radio.buttons[i],ctrl->radio.shortcuts[i]));
                }
                else
                {
                    b=new QRadioButton(ctrl->radio.buttons[i]);
                }
                addRelation(b,ctrl);
                bg->addButton(b);
                bg->setId(b,i);
                if(i==0 && ctrl->radio.shortcut!=NO_SHORTCUT)
                {
                    setBuddy(l,b);
                }
                layout->addWidget(b);
                connect(b,SIGNAL(toggled(bool)),this,SLOT(buttonToggle(bool)));
            }
            layoutStack.top()->add(vBox,ctrl->generic.column);
            break;
        }
        default:
            QWidget* w=0;
            QObject* o=0;
            switch (ctrl->generic.type) 
            {
                case CTRL_TEXT:
                    w=new QLabel(ctrl->generic.label);
                    ((QLabel*)w)->setWordWrap(true);
                    break;
                case CTRL_CHECKBOX:
                    w=new QCheckBox(toShortCutString(ctrl->generic.label,ctrl->checkbox.shortcut));
                    connect(w,SIGNAL(toggled(bool)),this,SLOT(buttonToggle(bool)));
                    break;
                case CTRL_BUTTON:
                {
                    QPushButton* pb=new QPushButton(toShortCutString(ctrl->generic.label,ctrl->button.shortcut));
                    if(ctrl->button.isdefault)
                    {
                        qDebug() << "--set default" << ctrl->generic.label;
                        pb->setDefault(true);
                    }
                    connect(pb,SIGNAL(clicked()),this,SLOT(buttonClicked()));
                    w=pb;
                    break;
                }
                default:
                    int width=0;
                    switch (ctrl->generic.type) 
                    {
                        case CTRL_EDITBOX:
                            if(ctrl->editbox.has_list)
                            {
                                QComboBox* c=new QComboBox();
                                c->setEditable(true);
                                w=c;
                                connect(w,SIGNAL(activated(const QString&)),this,SLOT(textChanged(const QString&)));
                            }
                            else
                            {
                                QLineEdit* e=new QLineEdit();
                                connect(e,SIGNAL(textChanged(const QString&)),this,SLOT(textChanged(const QString&)));
                                if(ctrl->editbox.password)
                                {
                                    e->setEchoMode(QLineEdit::Password);
                                }
                                w=e;
                            }
                            width=ctrl->editbox.percentwidth;
                            break;
                        case CTRL_LISTBOX:
                            if(ctrl->listbox.height==0)
                            {
                                w=new QComboBox();
                                connect(w,SIGNAL(activated(int)),this,SLOT(selectionChange()));
                            }
                            else
                            {
                                QListWidget* lw=new QListWidget();
                                if(ctrl->listbox.ncols==0)
                                {
                                    connect(lw,SIGNAL(itemSelectionChanged()),this,SLOT(selectionChange()));
                                    connect(lw,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(doubleClicked()));
                                }
                                else
                                {
                                    lw->setSelectionMode(QAbstractItemView::MultiSelection);
                                    //connect(w,SIGNAL(itemSelectionChanged()),this,SLOT(selectionChange()));
                                }
                                if(ctrl->listbox.draglist)
                                {
                                    lw->setMovement(QListView::Snap);
                                    lw->setDragDropMode(QAbstractItemView::InternalMove);
                                }
                                w=lw;
                            }
                            width=ctrl->listbox.percentwidth;
                    }
                    if(ctrl->generic.label)
                    {
                        QLabel* l=new QLabel(toShortCutString(ctrl->generic.label,ctrl->listbox.shortcut));
                        setBuddy(l,w);
                        o=buddyLayout(l,w,width);
                    }
            }
            addRelation(w,ctrl);
            layoutStack.top()->add(o==0?w:o,ctrl->generic.column);
    }
}

void
ConfigWidget::callControlHandler(union control* ctrl,int eventType)
{
    if(ctrl && ctrl->generic.handler) ctrl->generic.handler(ctrl,this,_conf,eventType);
}

void 
ConfigWidget::refreshControl(union control* ctrl)
{
    callControlHandler(ctrl,EVENT_REFRESH);
}

void 
ConfigWidget::refreshControl(union control* ctrl,const QString& excludeKey)
{
    if(_excludes.contains(excludeKey+"/"+ctrl->generic.label)) return;
    refreshControl(ctrl);
}

void
ConfigWidget::valueChanged(QWidget* sender)
{
    callControlHandler(_widgetToControl.value(sender),EVENT_VALCHANGE);
}

void
ConfigWidget::startAction(QWidget* sender)
{
    callControlHandler(_widgetToControl.value(sender),EVENT_ACTION);
}

void
ConfigWidget::selectionChanged(QWidget* sender)
{
    callControlHandler(_widgetToControl.value(sender),EVENT_SELCHANGE);
}

void 
ConfigWidget::selectDialog(PuttyGridLayout* layout,union control* ctrl,QSignalMapper& sm,const QString& buttonText)
{
    QLineEdit* e=new QLineEdit();
    connect(e,SIGNAL(textChanged(const QString&)),this,SLOT(textChanged(const QString&)));
    addRelation(e,ctrl);

    QPushButton* button=new QPushButton(buttonText);
    connect(button,SIGNAL(clicked()),&sm,SLOT(map()));
    sm.setMapping(button,e);

    QHBoxLayout* hBox=new QHBoxLayout();
    hBox->addWidget(e);
    hBox->addWidget(button);

    if(ctrl->generic.label)
    {
        QLabel* l=new QLabel(toShortCutString(ctrl->generic.label,ctrl->editbox.shortcut));
        setBuddy(l,e);
        layout->add(buddyLayout(l,hBox),ctrl->generic.column);
    }
    else
    {
        layout->add(hBox,ctrl->generic.column);
    }
}

void 
ConfigWidget::setBuddy(QLabel* l,QWidget* w)
{
    l->setBuddy(w);
    _reversBuddy[w]=l;
}

void 
ConfigWidget::changePage(QTreeWidgetItem* item,QTreeWidgetItem* last)
{
    if(last!=item)
    {
        _pages.setCurrentWidget(_itemToPage[item]);
    }
}

void 
ConfigWidget::buttonToggle(bool)
{
    valueChanged((QWidget*)sender());
}

void 
ConfigWidget::buttonClicked()
{
    startAction((QWidget*)sender());
}

void 
ConfigWidget::buttonClicked(QAbstractButton* b)
{
    startAction(b);
}

void 
ConfigWidget::doubleClicked()
{
    startAction((QWidget*)sender());
}

void 
ConfigWidget::textChanged(const QString&)
{
    valueChanged((QWidget*)sender());
}

void 
ConfigWidget::selectionChange()
{
    selectionChanged((QWidget*)sender());
}

void 
ConfigWidget::selectFile(QWidget* editLine)
{
    QFileDialog d(this);
    if(d.exec())
    {
        ((QLineEdit*)editLine)->setText(d.selectedFiles().at(0));
    }
}

void 
ConfigWidget::selectFont(QWidget* editLine)
{
    bool ok;
    QFont f=QFontDialog::getFont(&ok,findValidFont(((QLineEdit*)editLine)->text()),this);
    if(ok)
    {
        ((QLineEdit*)editLine)->setText(f.toString());
        if(!isFixedFont(f))
        {
            QMessageBox::warning(this,"Font Selection","The selected font isn't a fixed font.\nWill switch to poormode.");
        }
        else if(!isULDFont(f))
        {
            QMessageBox::warning(this,"Unicode Box Drawing","The selected font can't draw unicode line draw characters correct.\nWill switch to poormode.");
        }
    }
}

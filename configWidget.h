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
#ifndef _CONFIG_WIDGET_H_
#define _CONFIG_WIDGET_H_

#include <QWidget>
#include <QAbstractListModel>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QDialogButtonBox>
#include <QSignalMapper>
#include <QStack>
#include <QSet>

extern "C" {
#include "putty.h"
}

class QTreeWidgetItem;
class PuttyGridLayout;
class QLayout;
class QLabel;
class ConfigWidget:public QWidget
{
    Q_OBJECT
    public:
    static bool isFixedFont(const QFont& font);
    static bool isULDFont(const QFont& font);
    static bool isValidFont(const QFont& font);
    static QFont findValidFont(const QString& text);

    ConfigWidget(bool buttonBox=true);
    ~ConfigWidget();
    virtual void fill(Conf* conf,QDialog* parent=0,int midSession=0,int protCfgInfo=0);
    QString toShortCutString(const char* label,char shortcut) const;

    void setCheckBox(union control* ctrl,int checked);
    int checkBox(union control* ctrl) const;
    void setRadioButton(union control* ctrl,int which);
    int radioButton(union control* ctrl) const;
    void setEditBox(union control* ctrl,const char* text);
    char* editBox(union control* ctrl) const;
    void setFileBox(union control* ctrl,const char* text);
    char* fileBox(union control* ctrl) const;
    void listBoxClear(union control* ctrl);
    void listBoxDel(union control* ctrl,int index);
    void listBoxAdd(union control* ctrl,const char* text);
    void listBoxAddWithId(union control* ctrl,const char* text,int id);
    int listBoxGetId(union control* ctrl,int index);
    int listBoxIndex(union control* ctrl);
    void listBoxSelect(union control* ctrl,int index);
    void setLabel(union control* ctrl,const char* text);
    void refresh(union control* ctrl);
    char* privData(union control* ctrl) const;
    char* setPrivData(union control* ctrl,size_t size);
    void selectColor(union control* ctrl,int r,int g,int b);
    void selectedColor(union control* ctrl,int* r,int* g,int* b);
    void errorMessage(const char* text);
    virtual void accept() const;
    virtual void reject() const;

    protected:
    void createTreeItemAndWidgetForString(const QString& path);
    void fillPage(struct controlset* cs);
    void fillButtonBox(QDialog* parent,struct controlset* cs);
    void addRelation(QWidget* w,union control* ctrl);
    QLayout* buddyLayout(QLabel* l,QObject* o,int widgetWidth=100) const;
    void insertControl(union control* ctrl,QStack<PuttyGridLayout*>& layoutStack,const QString& excludeKey);
    void callControlHandler(union control* ctrl,int eventType);
    void refreshControl(union control* ctrl);
    void refreshControl(union control* ctrl,const QString& excludeKey);
    void valueChanged(QWidget* sender);
    void startAction(QWidget* sender);
    void selectionChanged(QWidget* sender);
    void selectDialog(PuttyGridLayout* layout,union control* ctrl,QSignalMapper& sm,const QString& buttonText);
    void setBuddy(QLabel* l,QWidget* w);

    protected slots:
    void changePage(QTreeWidgetItem* item,QTreeWidgetItem* last);
    void buttonToggle(bool checked);
    void buttonClicked();
    void buttonClicked(QAbstractButton* button);
    void doubleClicked();
    void textChanged(const QString&);
    void selectionChange();
    void selectFile(QWidget*);
    void selectFont(QWidget*);
    
    protected:
    Conf* _conf;
    QMap<QTreeWidgetItem*,QWidget*> _itemToPage;
    QMap<QWidget*,QTreeWidgetItem*> _pageToItem;
    QHash<QString,QTreeWidgetItem*> _stringToItem;
    QMultiHash<QWidget*,union control*> _widgetToControl;
    QMultiHash<union control*,QWidget*> _controlToWidget;
    QMap<union control*,char*> _privData;
    QMap<QWidget*,QLabel*> _reversBuddy;

    QTreeWidget _category;
    QStackedWidget _pages;
    QDialogButtonBox _buttonBox;
    QSignalMapper _fileSm;
    QSignalMapper _fontSm;
    QColor _selectedColor;

    QSet<QString> _excludes;
    QDialog* _parent;
};

#endif

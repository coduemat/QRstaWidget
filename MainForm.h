/* 
 * File:   MainForm.h
 * Author: Taran
 *
 * Created on 6 Март 2015 г., 23:39
 */

#ifndef _MAINFORM_H
#define	_MAINFORM_H

#include "ui_MainForm.h"
#include "QRstaWidget.h"

class MainForm : public QWidget {
    Q_OBJECT
public:
    MainForm();
    virtual ~MainForm();
private:
    Ui::Form widget;
    QRstaWidget wogl;
    private slots:    
    void test();
};

#endif	/* _MAINFORM_H */

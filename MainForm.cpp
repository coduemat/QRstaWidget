/*
 * File:   MainForm.cpp
 * Author: Taran
 *
 * Created on 6 Март 2015 г., 23:39
 */

#include "MainForm.h"
#include <QMessageBox>

MainForm::MainForm() {
    widget.setupUi(this);
    QHBoxLayout *layout = new QHBoxLayout(widget.frameOgl);
    layout->addWidget(&wogl);
    connect(widget.pushButton, SIGNAL(clicked()), this, SLOT(test()));
}

MainForm::~MainForm() {
}

void MainForm::test() {
//    QMessageBox::information(this, "ololo", "start");
    widget.pushButton->setEnabled(false);
    wogl.load();
    widget.pushButton->setEnabled(true);
//    QMessageBox::information(this, "ololo", "ololo");
}

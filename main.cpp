/*
 * File:   main.cpp
 * Author: Taran
 *
 * Created on 6 Март 2015 г., 22:05
 */

#include <QApplication>

#include "MainForm.h"

int main(int argc, char *argv[]) {
    // initialize resources, if needed
    // Q_INIT_RESOURCE(resfile);

    QApplication app(argc, argv);

    // create and show your widgets here
    MainForm mf;
    mf.show();
    return app.exec();
}

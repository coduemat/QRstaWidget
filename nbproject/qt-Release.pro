# This file is generated automatically. Do not edit.
# Use project properties -> Build -> Qt -> Expert -> Custom Definitions.
TEMPLATE = app
DESTDIR = dist/Release/MinGW_1-Windows
TARGET = QtRstaPlot
VERSION = 1.0.0
CONFIG -= debug_and_release app_bundle lib_bundle
CONFIG += release 
PKGCONFIG +=
QT = core gui widgets opengl
SOURCES += ClRsta.cpp MainForm.cpp QRstaWidget.cpp main.cpp
HEADERS += ClRsta.h MainForm.h QRstaWidget.h
FORMS += MainForm.ui
RESOURCES +=
TRANSLATIONS +=
OBJECTS_DIR = build/Release/MinGW_1-Windows
MOC_DIR = 
RCC_DIR = 
UI_DIR = 
QMAKE_CC = gcc
QMAKE_CXX = g++
DEFINES += 
INCLUDEPATH += ../../../usr/qt/5.4.0/5.4/mingw491_32/include/qwt ../../../usr/share/clfft/include ../../../usr/share/opencl/include 
LIBS += ../../../usr/qt/5.4.0/5.4/mingw491_32/lib/libqwtd.a ../../../usr/share/clfft/lib/import/libclFFT.dll.a ../../../usr/share/opencl/lib/libOpenCL.a  

/* 
 * File:   QRstaWidget.cpp
 * Author: Taran
 * 
 * Created on 30 Март 2015 г., 22:58
 */

#include <stdio.h>
#include <stdlib.h>
#include <qguiapplication.h>
#include <math.h>

#include "QRstaWidget.h"

QRstaWidget::QRstaWidget() {
    setFftLength(FftLength);
    setFftOverlap(FftOverlap);
    setRstaHeight(RstaHeight);
    setWaterfallHeight(WaterfallHeight);
    setFftLevel(FftLevel);
    setFftScale(FftScale);
    setRstaWeight(RstaWeight);
    setRstaDecay(RstaDecay);
    
    texture[0] = 0;
    texture[1] = 0;
    buffer = 0;
    clrsta = NULL;
}

QRstaWidget::~QRstaWidget() {
    delete clrsta;
}

void QRstaWidget::initializeGL() {
    initializeGLFunctions(context());
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-0.5, 0.5, 0.5, -0.5, -1000, 1000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);     

    uint32_t *drsta = new uint32_t[fftLength * rstaHeight];
    memset(drsta, 0, fftLength * rstaHeight * sizeof(uint32_t));
    
    uint32_t *dwtrfl = new uint32_t[fftLength * waterfallHeight];
    memset(dwtrfl, 0, fftLength * waterfallHeight * sizeof(uint32_t));
        
    glGenTextures(2, texture);
    printf("ids %d %d \n", texture[0], texture[1]);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fftLength, 
            rstaHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, drsta);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glBindTexture(GL_TEXTURE_2D, texture[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fftLength, 
            waterfallHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, dwtrfl);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    
    float *line = new float[fftLength * 2];
    for (int i = 0; i < fftLength * 2; i += 2) {
        line[i] = (float)i / float(fftLength * 2);
        line[i + 1] = -0.0f;
    }

    makeCurrent();
    glGenBuffers(1, &buffer);
    printf("ids %d \n", buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glVertexPointer(2, GL_FLOAT, 0, 0);
    glBufferData(GL_ARRAY_BUFFER, fftLength * 2 * sizeof(cl_float),
            line, GL_STATIC_DRAW);
    
    clrsta = new ClRsta(
            fftLength, 
            rstaHeight, 
            waterfallHeight, 
            fftLevel, 
            fftScale, 
            rstaWeight, 
            rstaDecay
        );
    delete drsta;
    delete dwtrfl;
    delete line;
    fflush(stdout);
}

void QRstaWidget::paintGL() {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    qglColor(Qt::black);

    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND); 

    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(-0.5, 1.0); glVertex2f(-0.5, -0.5);
    glTexCoord2f( 0.5, 1.0); glVertex2f( 0.5, -0.5);
    glTexCoord2f( 0.5, 0.0); glVertex2f( 0.5,  0.0);
    glTexCoord2f(-0.5, 0.0); glVertex2f(-0.5,  0.0);
    glEnd();
    
    float offset = (float)clrsta->getWline() / (float)waterfallHeight;
    
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    glBegin(GL_QUADS);
    glTexCoord2f(-0.5, 1.0 + offset); glVertex2f(-0.5,  0.0);
    glTexCoord2f( 0.5, 1.0 + offset); glVertex2f( 0.5,  0.0);
    glTexCoord2f( 0.5, 0.0 + offset); glVertex2f( 0.5,  0.5);
    glTexCoord2f(-0.5, 0.0 + offset); glVertex2f(-0.5,  0.5);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    
    glScalef(1.0f, 0.5f, 1.0f);
    glTranslatef(-0.5f, 0.0f, 0.0f);

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(2.0f);
    glColor4f(1.0f, 0.0f, 0.0f, 0.75f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_LINE_STRIP, 0, fftLength);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void QRstaWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h); 
}

void QRstaWidget::load() {
    int i, n, count;
    cl_int err;
    FILE *fin;

    cl_float *re = new cl_float[fftLength];
    cl_float *im = new cl_float[fftLength];
    
    fin = fopen("d:/music/sin.csv", "r");

    count = 0;
    do {
        i = 0;
        for (i = 0; i < fftOverlap; i++) {
            re[i] = re[i + fftLength - fftOverlap];
            im[i] = im[i + fftLength - fftOverlap];
        }
        do {
            n = fscanf(fin, "%f\t%f\n", &re[i], &im[i]);
            i++;
        } while ((i < fftLength) && (n > 0));

        if (n > 0) {
            err = clrsta->add(re, im);
            if (err != CL_SUCCESS) {
                printf("error add data %d\n", err);
            }
            if (count == 10) {
                err = clrsta->add2(re, im);
                cl_float2 *data = clrsta->getDout();
                FILE *fout = fopen("d:/tmp/d.csv", "w");
                for (int k = 0; k < fftLength; k++) {
                    fprintf(fout, "%f\t%f\n", data[k].x, data[k].y);
                }
                fclose(fout);
            }
        }
        else {
            printf("n = %d\n", n);
        }
        count++;
        repaint();
        qApp->processEvents();
    } while (!feof(fin) && (n > 0) && (err == CL_SUCCESS)/* && (count < 1024)*/);
    

    fclose(fin);
    delete re;
    delete im;
}

void QRstaWidget::setFftLength(int value) {
    if (value > FftLengthMin) {
        if (value < FftLengthMax) {
            fftLength = 1;

            while (value >>= 1) {
                fftLength <<= 1;
            }
        }
        else {
            fftLength = FftLengthMax;
        }
    }
    else {
        fftLength = FftLengthMin;
    }
}

void QRstaWidget::setFftOverlap(int value) {
    if (value < 0) {
        fftOverlap = 0;
    } 
    else {
        if (value >= fftLength) {
            fftOverlap = fftLength - 1;
        }
        else {
            fftOverlap = value;
        }
    }
}

void QRstaWidget::setRstaHeight(int value) {
    rstaHeight = value;
}

void QRstaWidget::setWaterfallHeight(int value) {
    waterfallHeight = value;
}

void QRstaWidget::setRstaDecay(float value) {
    rstaDecay = value;
}

void QRstaWidget::setRstaWeight(float value) {
    rstaWeight = value;
}

void QRstaWidget::setFftScale(float value) {
    fftScale = value;
}

void QRstaWidget::setFftLevel(float value) {
    fftLevel = value;
}

int QRstaWidget::getFftLength() const {
    return fftLength;
}

int QRstaWidget::getFftOverlap() const {
    return fftOverlap;
}

int QRstaWidget::getRstaHeight() const {
    return rstaHeight;
}

int QRstaWidget::getWaterfallHeight() const {
    return waterfallHeight;
}

float QRstaWidget::getRstaDecay() const {
    return rstaDecay;
}

float QRstaWidget::getRstaWeight() const {
    return rstaWeight;
}

float QRstaWidget::getFftScale() const {
    return fftScale;
}

float QRstaWidget::getFftLevel() const {
    return fftLevel;
}



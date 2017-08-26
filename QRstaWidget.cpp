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
#include <time.h>

#include "QRstaWidget.h"
#include <GL/glx.h>

QRstaWidget::QRstaWidget(size_t size, int height, int height_waterfall, 
        float level, float scale, float weight, float decay) : 
        ClRsta (size, height, height_waterfall, level, scale, weight, decay){
    
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
}

QRstaWidget::~QRstaWidget() {

}

void QRstaWidget::initializeGL() {

    initializeOpenGLFunctions();

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
    qWarning("ids %d %d \n", texture[0], texture[1]);
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

//    makeCurrent(); // TODO see doc initializeGL, is no need
    
    glGenBuffers(1, &buffer);
    qWarning("ids %d \n", buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glVertexPointer(2, GL_FLOAT, 0, 0);
    glBufferData(GL_ARRAY_BUFFER, fftLength * 2 * sizeof(cl_float),
            line, GL_STATIC_DRAW);
    
    delete drsta;
    delete dwtrfl;
    delete line;

    this->init((cl_context_properties)glXGetCurrentContext(), (cl_context_properties)glXGetCurrentDisplay());
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
    
    float offset = (float)this->getWline() / (float)waterfallHeight;
    
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
    char line[80];
    float tmp;
    n = 0;
    cl_float2 *cplx = this->getDin();
    
    fin = fopen("/home/taran/Downloads/intro.txt", "r");
    if (!fin) {
        qWarning("can't open file\n");
        return;
    }
    int32_t t = time(0);
    
    count = 0;
    do {
        i = 0;
        for (i = 0; i < fftOverlap; i++) {
            cplx[i].x = cplx[i + fftLength - fftOverlap].x;
            cplx[i].y = cplx[i + fftLength - fftOverlap].y;
        }
        do {
            if (fgets(line, sizeof(line), fin)) {
                if (line[0] != ';') {
                    n = sscanf(line, "%f%f%f", &tmp, &cplx[i].x, &cplx[i].y);
                    if (n >= 3) {
                        i++;
                    }
                }
            }
            else {
                break;
            }
//            n = fscanf(fin, "%f\t%f\n", &cplx[i].x, &cplx[i].y);
//            i++;
        } while ((i < fftLength) && !feof(fin));

        if (n > 0) {
            err = this->run();
            if (err != CL_SUCCESS) {
                qWarning("error add data %d\n", err);
            }
        }
        else {
            qWarning("n = %d\n", n);
        }
        count++;
        repaint();
        qApp->processEvents();
    } while (!feof(fin) && (n > 0) && (err == CL_SUCCESS)/* && (count < 1024)*/);
    
    qWarning("time: %ld\n", time(0) - t);
    fclose(fin);
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



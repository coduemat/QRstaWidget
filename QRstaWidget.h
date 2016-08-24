/* 
 * File:   QRstaWidget.h
 * Author: Taran
 *
 * Created on 30 Март 2015 г., 22:58
 */

#ifndef QRSTAWIDGET_H
#define	QRSTAWIDGET_H

#include <QGLWidget>
#include <QGLFunctions>

#include "ClRsta.h"

class QRstaWidget : public QGLWidget, /*protected QGLFunctions, */private ClRsta{
    Q_OBJECT
    Q_PROPERTY ( int FftLength READ getFftLength WRITE setFftLength )
    Q_PROPERTY ( int FftOverlap READ getFftOverlap WRITE setFftOverlap )
    Q_PROPERTY ( int RstaHeight READ getRstaHeight WRITE setRstaHeight )
    Q_PROPERTY ( int WaterfallHeight READ getWaterfallHeight WRITE setWaterfallHeight )
    
public:
    QRstaWidget(
            size_t size = FftLength, 
            int height = RstaHeight, 
            int height_waterfall = WaterfallHeight, 
            float level = FftLevel, 
            float scale = FftScale, 
            float weight = RstaWeight, 
            float decay = RstaDecay
        );
    
    virtual ~QRstaWidget();
    void load();
    
    void setFftLength(int value);
    void setFftOverlap(int value);
    void setRstaHeight(int value);
    void setWaterfallHeight(int value);
    void setRstaDecay(float rstaDecay);
    void setRstaWeight(float rstaWeight);
    void setFftScale(float fftScale);
    void setFftLevel(float fftLevel);
    
    int getFftLength() const;
    int getFftOverlap() const;
    int getRstaHeight() const;
    int getWaterfallHeight() const;
    float getRstaDecay() const;
    float getRstaWeight() const;
    float getFftScale() const;
    float getFftLevel() const;
    
private:
    static const int FftLength          = 1024;
    static const int FftOverlap         = 0;
    static const int RstaHeight         = 256;
    static const int WaterfallHeight    = 256;
    static const float FftLevel         = 45.0f;
    static const float FftScale         = 4.0f;
    static const float RstaWeight       = 0.1f;
    static const float RstaDecay        = 0.97f;
    
    static const int FftLengthMin       = 8;
    static const int FftLengthMax       = 32768;
    
    GLuint texture[2];
    GLuint buffer;
    
    int fftLength;
    int fftOverlap;
    int rstaHeight;
    int waterfallHeight;
    float fftLevel;
    float fftScale;
    float rstaWeight;
    float rstaDecay;
    
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);
};

#endif	/* QRSTAWIDGET_H */


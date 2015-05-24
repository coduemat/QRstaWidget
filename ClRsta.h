/* 
 * File:   ClRsta.h
 * Author: Taran
 *
 * Created on 1 Апрель 2015 г., 23:22
 */

#ifndef CLRSTA_H
#define	CLRSTA_H
#include <cl/cl.h>
#include <clFFT.h>

class ClRsta {
public:
    ClRsta(size_t size, int height, int height_waterfall, 
        float level, float scale, float weight, float decay);
    virtual ~ClRsta();
    cl_int add(cl_float* re, cl_float* im);
    cl_int add2(cl_float* re, cl_float* im);
    cl_uint getWline() const;
    cl_float2* getDout() const;    
private:
    cl_int err;
    cl_context ctx;
    cl_command_queue queue;
    cl_program program;
    cl_kernel k_veclog10;
    cl_kernel k_vv2mul;
    cl_kernel k_mag2mtx;
    cl_kernel k_img2tex;
    cl_kernel k_fft;
    cl_mem b_fft_in;
    cl_mem b_fft_out;
    cl_mem buffersIn[2];
    cl_mem buffersOut[2];
    cl_mem bufferMag;
    cl_mem bufferWnd;
    cl_mem bufferFrame;
    cl_mem bufferSum;
    cl_mem tmpBuffer;
    cl_mem image2d;
    cl_mem image_waterfall;
    cl_mem spectrum;
    size_t size;
    clfftPlanHandle planHandle;
    cl_float* re_in;
    cl_float* im_in;
    cl_float* re_out;
    cl_float* im_out;
    cl_float* mag;
    cl_float* wnd;
    cl_float* sum;
    cl_float* frame;
    cl_float2* din;
    cl_float2* dout;
    cl_uint height;
    cl_uint height_waterfall;
    cl_uint wline;
    cl_uint fft_p;
    cl_float level;
    cl_float scale;
    cl_float weight;
    cl_float decay;
    
    void init();
    cl_int initFft();
    cl_int initPrograms();
    cl_int initImage();
    void blackmanharris(cl_float *buffer, size_t size);
    cl_int fft2();
    cl_int vv2mul();
    cl_int veclog10();
    cl_int mag2img();
    cl_int img2tex();
    cl_int fft();
    void printMemInfo(cl_mem mem);
    void printImageInfo(cl_mem image);
    void printObjInfo(cl_mem mem);
    void checkError(cl_int error, const char str[], int is_critical);
};

#endif	/* CLRSTA_H */


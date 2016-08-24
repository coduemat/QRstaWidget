/* 
 * File:   ClRsta.cpp
 * Author: Taran
 * 
 * Created on 1 Апрель 2015 г., 23:22
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <cl/cl_gl.h>
//#include <qopenglext.h>
#include "cl_routines.h"
#include "ClRsta.h"

ClRsta::ClRsta(size_t size, int height, int height_waterfall, 
        float level, float scale, float weight, float decay) {
    
    this->size              = size;
    this->height            = height;
    this->height_waterfall  = height_waterfall;
    this->level             = level;
    this->scale             = scale;
    this->weight            = weight;
    this->decay             = decay;

    wline  = 0;
//    init();
}

ClRsta::~ClRsta() {
    err = clEnqueueReleaseGLObjects(queue, 1, &image2d, 0, NULL, NULL);
    checkError(err, "clEnqueueReleaseGLObjects", 0);
    err = clEnqueueReleaseGLObjects(queue, 1, &image_waterfall, 0, NULL, NULL);
    checkError(err, "clEnqueueReleaseGLObjects", 0);
    err = clEnqueueReleaseGLObjects(queue, 1, &spectrum, 0, NULL, NULL);
    checkError(err, "clEnqueueReleaseGLObjects", 0);
        
    err = 0;
    err |= clReleaseMemObject(bufferMag);
    err |= clReleaseMemObject(bufferWnd);
    err |= clReleaseMemObject(bufferFrame);
    err |= clReleaseMemObject(bufferSum);
    err |= clReleaseMemObject(image2d);
    err |= clReleaseMemObject(image_waterfall);
    err |= clReleaseMemObject(spectrum);
    err |= clReleaseMemObject(b_fft_in);
    err |= clReleaseMemObject(b_fft_out);
    if (err != CL_SUCCESS) {
        printf("Error clReleaseMemObject %d\n", err);
    }

    err = 0;
    err |= clReleaseKernel(k_cplx2db);
    err |= clReleaseKernel(k_cplxmulv);
    err |= clReleaseKernel(k_mag2mtx);
    err |= clReleaseKernel(k_fft);
    err |= clReleaseProgram(program);
    if (err != CL_SUCCESS) {
        printf("Error clReleaseKernel or clReleaseProgram\n");
    }

    err = 0;
    err |= clReleaseCommandQueue(queue);
    err |= clReleaseContext(ctx);
    if (err != CL_SUCCESS) {
        printf("Error clReleaseCommandQueue or clReleaseContext\n");
    }

    delete din   ;
    delete dout  ;
    delete mag;
    delete wnd;
    delete sum;
    delete frame;
}

void ClRsta::init() {
    err = 0;
    ctx = 0;
    queue = 0;
    fft_p = 0;
    
    cl_platform_id platforms[32];
    size_t psize = 0;
    err = clGetPlatformIDs(32, platforms, &psize);
    if (err != CL_SUCCESS) {
        printf("Error clGetPlatformIDs: %d\n", err);
        exit(-1);
    }
    else {
        char info[1024];
        info[1023] = 0;
        for (int i = 0; i < psize; i++) {
            printf("clGetPlatformIDs 0x%08X\n", platforms[i]);
            clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 1024, info, NULL);
            printf("CL_PLATFORM_NAME: %s\n", info);
            clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, 1024, info, NULL);
            printf("CL_PLATFORM_VENDOR: %s\n", info);
            clGetPlatformInfo(platforms[i], CL_PLATFORM_PROFILE, 1024, info, NULL);
            printf("CL_PLATFORM_PROFILE: %s\n", info);
//            clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, 1024, info, NULL);
//            printf("CL_PLATFORM_EXTENSIONS: %s\n", info);
        }
    }

    cl_device_id devices[32]; 
    size_t nsize = 0;
    err = clGetDeviceIDs(platforms[1], CL_DEVICES_FOR_GL_CONTEXT_KHR, 32, devices, &nsize);
    if (err != CL_SUCCESS) {
        printf("Error clGetDeviceIDs: %d\n", err);
        exit(-1);
    }
    else {
        for (int i = 0; i < nsize; i++) {
            printf("device[%d]: 0x%08X\n", i, devices[i]);
        }
    }

//    char str[1024];
//    str[1023] = 0;
//    err = clGetDeviceInfo(devices[0], CL_DEVICE_EXTENSIONS, 1024, str, NULL);
//    if (err != CL_SUCCESS) {
//        printf("Error clGetDeviceInfo: %d\n", err);
//        exit(-1);
//    }
//    else {
//        printf("ext: %s\n", str);
//    }

    cl_context_properties props[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[1], 
        CL_GL_CONTEXT_KHR  , (cl_context_properties)wglGetCurrentContext(),
        CL_WGL_HDC_KHR     , (cl_context_properties)wglGetCurrentDC(),
        0
    };
    printf("wglGetCurrentContext: 0x%08X\n", (uint32_t)wglGetCurrentContext());
    printf("wglGetCurrentDC: 0x%08X\n", (uint32_t)wglGetCurrentDC());
    
    err = 0;
    ctx = clCreateContext(props, 1, &devices[0], NULL, NULL, &err);
    if (err != CL_SUCCESS || ctx == NULL) {
        printf("error clCreateContext %d\n", err);
        exit(-1);
    }

    queue = clCreateCommandQueue(ctx, devices[0], 0, &err);
    
    initImage();
    initPrograms();
}

cl_int ClRsta::initPrograms() {
    const char *txt = (const char *)routines_cl;
    
    din  = new cl_float2[size];
    dout = new cl_float2[size];
    mag = new cl_float[size];
    wnd = new cl_float[size];
    sum = new cl_float[size * height];
    frame = new cl_float[size * height];
    
    if (mag == NULL || wnd == NULL || sum == NULL || frame == NULL || 
            din == NULL || dout == NULL) {
        printf("error new operator in initPrograms\n");
    }
    
    memset(dout , 0, size * sizeof(*dout));
    memset(mag  , 0, size * sizeof(*mag));
    memset(wnd  , 0, size * sizeof(*wnd));
    memset(sum  , 0, size * height * sizeof(*sum  ));
    memset(frame, 0, size * height * sizeof(*frame));

    blackmanharris(wnd, size);
    
    program = clCreateProgramWithSource(ctx, 1, &txt, 
            &routines_cl_len, &err);
    
    if (err != CL_SUCCESS) {
        printf("Error clCreateProgramWithSource: %d\n", err);
    }
    
    err = clBuildProgram(program, 0, NULL, "", NULL, NULL);

    if (err != CL_SUCCESS) {
        printf("Error clBuildProgram: %d\n", err);
    }

    b_fft_in = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
            size * sizeof(cl_float2), din, &err);
    checkError(err, "clCreateBuffer b_fft_in", 1);
    
    b_fft_out = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
            size * sizeof(cl_float2), dout, &err);
    checkError(err, "clCreateBuffer b_fft_out", 1);

    err = clEnqueueWriteBuffer(queue, b_fft_out, CL_TRUE, 0, 
            size * sizeof(*dout), dout, 0, NULL, NULL);
    checkError(err, "clEnqueueWriteBuffer b_fft_out", 1);
        
    bufferMag = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
            size * sizeof(*mag), mag, &err);
    if (err != CL_SUCCESS) {
        printf("Error clCreateBuffer bufferMag: %d\n", err);
    }
        
    bufferWnd = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
            size * sizeof(*wnd), wnd, &err);
    if (err != CL_SUCCESS) {
        printf("Error clCreateBuffer bufferWnd: %d\n", err);
    }
    
    err = clEnqueueWriteBuffer(queue, bufferWnd, CL_TRUE, 0, 
            size * sizeof(*wnd), wnd, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Error with bufferWnd clEnqueueWriteBuffer: %d\n", err);
    }
    
    k_cplx2db = clCreateKernel(program, "cplx2db", &err);
    if (err != CL_SUCCESS) {
        printf("Error clCreateKernel: %d\n", err);
    }
    
    err = 0;
    err |= clSetKernelArg(k_cplx2db, 0, sizeof(b_fft_out), &b_fft_out);
    err |= clSetKernelArg(k_cplx2db, 1, sizeof(bufferMag), &bufferMag);
    if (err != CL_SUCCESS) {
        printf("Error clSetKernelArg %d\n", err);
    }
    
    k_cplxmulv = clCreateKernel(program, "cplxmulv", &err);
    if (err != CL_SUCCESS) {
        printf("Error clCreateKernel: %d\n", err);
    }
    
    err = 0;
    err |= clSetKernelArg(k_cplxmulv, 0, sizeof(b_fft_in ), &b_fft_in );
    err |= clSetKernelArg(k_cplxmulv, 1, sizeof(bufferWnd), &bufferWnd);
    if (err != CL_SUCCESS) {
        printf("Error clSetKernelArg %d\n", err);
    }
    
    bufferSum = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
            size * height * sizeof(*sum), sum, &err);
    if (err != CL_SUCCESS) {
        printf("Error clCreateBuffer bufferImg: %d\n", err);
    }

    err = clEnqueueWriteBuffer(queue, bufferSum, CL_TRUE, 0, 
            size * height * sizeof(*sum), sum, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Error with bufferSum clEnqueueWriteBuffer: %d\n", err);
    }
    
    bufferFrame = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
            size * height * sizeof(cl_float), frame, &err);
    if (err != CL_SUCCESS) {
        printf("Error clCreateBuffer bufferFrame: %d\n", err);
    }

    err = clEnqueueWriteBuffer(queue, bufferFrame, CL_TRUE, 0, 
            size * height * sizeof(*frame), frame, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Error with bufferFrame clEnqueueWriteBuffer: %d\n", err);
    }

    k_mag2mtx = clCreateKernel(program, "mag2frame", &err);
    if (err != CL_SUCCESS) {
        printf("Error clCreateKernel: %d\n", err);
    }
    
    err = 0;
    err |= clSetKernelArg(k_mag2mtx, 0, sizeof(bufferMag  ), &bufferMag  );
    err |= clSetKernelArg(k_mag2mtx, 1, sizeof(bufferFrame), &bufferFrame);
    err |= clSetKernelArg(k_mag2mtx, 2, sizeof(image_waterfall), 
            &image_waterfall);
    err |= clSetKernelArg(k_mag2mtx, 3, sizeof(spectrum), &spectrum);
    err |= clSetKernelArg(k_mag2mtx, 4, sizeof(height  ), &height  );
    err |= clSetKernelArg(k_mag2mtx, 5, sizeof(wline   ), &wline   );
    err |= clSetKernelArg(k_mag2mtx, 6, sizeof(level   ), &level   );
    err |= clSetKernelArg(k_mag2mtx, 7, sizeof(scale   ), &scale   );
    err |= clSetKernelArg(k_mag2mtx, 8, sizeof(weight  ), &weight  );
    if (err != CL_SUCCESS) {
        printf("Error clSetKernelArg %d\n", err);
    }

    k_img2tex = clCreateKernel(program, "frame2tex", &err);
    if (err != CL_SUCCESS) {
        printf("Error clCreateKernel: %d\n", err);
    }
    err = 0;
    err |= clSetKernelArg(k_img2tex, 0, sizeof(bufferFrame), &bufferFrame);
    err |= clSetKernelArg(k_img2tex, 1, sizeof(bufferSum  ), &bufferSum  );
    err |= clSetKernelArg(k_img2tex, 2, sizeof(image2d    ), &image2d    );
    err |= clSetKernelArg(k_img2tex, 3, sizeof(decay      ), &decay      );
    if (err != CL_SUCCESS) {
        printf("Error clSetKernelArg %d\n", err);
    }

    k_fft = clCreateKernel(program, "fftRadix2Kernel", &err);
    checkError(err, "fftRadix2Kernel", 1);
    err = 0;
    err |= clSetKernelArg(k_fft, 0, sizeof(b_fft_in ), &b_fft_in );
    err |= clSetKernelArg(k_fft, 1, sizeof(b_fft_out), &b_fft_out);
    err |= clSetKernelArg(k_fft, 2, sizeof(fft_p    ), &fft_p    );
    checkError(err, "clSetKernelArg k_fft", 1);
    
    return err;
}

void ClRsta::printImageInfo(cl_mem image) {
    size_t p = 0;
    err = clGetImageInfo(image, CL_IMAGE_WIDTH, sizeof(p), &p, NULL);
    if (err != CL_SUCCESS) {
        printf("error clGetImageInfo %d\n", err);
    }
    else {
        printf("CL_IMAGE_WIDTH %d\n", p);
    }

    err = clGetImageInfo(image, CL_IMAGE_HEIGHT, sizeof(p), &p, NULL);
    if (err != CL_SUCCESS) {
        printf("error clGetImageInfo %d\n", err);
    }
    else {
        printf("CL_IMAGE_HEIGHT %d\n", p);
    }

    err = clGetImageInfo(image, CL_IMAGE_DEPTH, sizeof(p), &p, NULL);
    if (err != CL_SUCCESS) {
        printf("error clGetImageInfo %d\n", err);
    }
    else {
        printf("CL_IMAGE_DEPTH %d\n", p);
    }
    
    cl_image_format fmt;
    err = clGetImageInfo(image, CL_IMAGE_FORMAT, sizeof(cl_image_format), &fmt, NULL);
    if (err != CL_SUCCESS) {
        printf("error clGetImageInfo %d\n", err);
    }
    else {
        printf("image_channel_data_type 0x%04X\n", fmt.image_channel_data_type);
        printf("image_channel_order 0x%04X\n", fmt.image_channel_order);
    }
}

void ClRsta::checkError(cl_int error, const char str[], int is_critical) {
    if (error != CL_SUCCESS) {
        printf("Error [%s]: %d\n", str, error);
        if (is_critical) {
            exit(-1);
        }
    }
}

cl_int ClRsta::initImage() {
    GLuint texId[] = {1, 2};
    
    image2d = clCreateFromGLTexture2D(ctx, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 
            0, texId[0], &err);
    checkError(err, "clCreateImage2D", 1);
//    printImageInfo(image2d);
//    printMemInfo(image2d);

    image_waterfall = clCreateFromGLTexture2D(ctx, CL_MEM_READ_WRITE, 
            GL_TEXTURE_2D, 0, texId[1], &err);
    checkError(err, "clCreateImage2D", 0);

    glFinish();
    spectrum = clCreateFromGLBuffer(ctx, CL_MEM_READ_WRITE, 1, &err);
    checkError(err, "clCreateImage2D spectrum", 0);
    
    cl_mem obj[] = {image2d, image_waterfall, spectrum};
    err = clEnqueueAcquireGLObjects(queue, 3, obj, 0, NULL, NULL);
    checkError(err, "clEnqueueAcquireGLObjects", 1);
    
    err = clFinish(queue);
    checkError(err, "clFinish", 1);
    return err;
}

void ClRsta::printObjInfo(cl_mem mem) {
    
}

void ClRsta::printMemInfo(cl_mem mem) {
    size_t p;
    err = clGetMemObjectInfo(mem, CL_MEM_SIZE, sizeof(p), &p, NULL);
    if (err != CL_SUCCESS) {
        printf("error clGetMemObjectInfo %d\n", err);
    }
    else {
        printf("CL_MEM_SIZE %d\n", p);
    }

    cl_mem_flags flg;
    err = clGetMemObjectInfo(mem, CL_MEM_FLAGS, sizeof(flg), &flg, NULL);
    if (err != CL_SUCCESS) {
        printf("error clGetMemObjectInfo %d\n", err);
    }
    else {
        printf("CL_MEM_FLAGS %lu\n", (unsigned long)flg);
    }
    cl_mem_object_type type;
    err = clGetMemObjectInfo(mem, CL_MEM_TYPE, sizeof(type), &type, NULL);
    if (err != CL_SUCCESS) {
        printf("error clGetMemObjectInfo %d\n", err);
    }
    else {
        printf("CL_MEM_TYPE %d\n", type);
    }

}

void ClRsta::blackmanharris(cl_float *buffer, size_t size) {
    static const cl_float a[4] = {0.35875f, 0.48829f, 0.14128f, 0.01168f};
    
    cl_float p[3] = {
        2.0f * (cl_float)M_PI / (cl_float)(size - 1), 
        4.0f * (cl_float)M_PI / (cl_float)(size - 1), 
        6.0f * (cl_float)M_PI / (cl_float)(size - 1), 
    };
    
    for (size_t i = 0; i < size; i++) {
        buffer[i] = a[0] - a[1] * cosf(p[0] * (cl_float)i) + 
                a[2] * cosf(p[1] * (cl_float)i) - 
                a[3] * cosf(p[2] * (cl_float)i);
    }
}

cl_int ClRsta::fft() {
    size_t s = size / 2;
    
    for (fft_p = 1; fft_p <= s; fft_p <<= 1) {
        err = clSetKernelArg(k_fft, 2, sizeof(fft_p), &fft_p);
        checkError(err, "clSetKernelArg k_fft", 1);

        err = clEnqueueNDRangeKernel(queue, k_fft, 1, NULL, &s, NULL, 0, 
                NULL, NULL);
        checkError(err, "fft clEnqueueNDRangeKernel", 1);

        err = clFinish(queue);
        checkError(err, "fft clFinish", 1);
        
        if (fft_p != s) {
            err = clEnqueueCopyBuffer(queue, b_fft_out, b_fft_in, 0, 0, 
                    size * sizeof(*dout), 0, NULL, NULL);
            checkError(err, "fft clEnqueueCopyBuffer", 1);
            
            err = clFinish(queue);
            checkError(err, "fft clFinish", 1);
        }
    }
    
//    err = clEnqueueReadBuffer(queue, b_fft_out, CL_TRUE, 0, 
//            size * sizeof(*dout), dout, 0, NULL, NULL);
//    checkError(err, "fft clEnqueueReadBuffer", 1);

    return err;
}

cl_int ClRsta::cplxmulv() {
    err = clEnqueueWriteBuffer(queue, b_fft_in, CL_TRUE, 0, 
            size * sizeof(*din), din, 0, NULL, NULL);
    checkError(err, "fft clEnqueueWriteBuffer", 1);

    err = clEnqueueNDRangeKernel(queue, k_cplxmulv, 1, NULL, &size, NULL, 0, 
            NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Error number %d\n", err);
    }

    err = clFinish(queue);
    if (err != CL_SUCCESS) {
        printf("Error number %d\n", err);
    }
    
    return err;
}

cl_int ClRsta::cplx2db() {
    err = clEnqueueNDRangeKernel(queue, k_cplx2db, 1, NULL, &size, NULL, 0, 
            NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Error clEnqueueNDRangeKernel %d\n", err);
    }

    err = clFinish(queue);
    if (err != CL_SUCCESS) {
        printf("Error clFinish %d\n", err);
    }

    return err;
}

cl_int ClRsta::mag2img() {
    size_t dim = size;
    err = clEnqueueNDRangeKernel(queue, k_mag2mtx, 1, NULL, &dim, NULL, 0, 
            NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Error clEnqueueNDRangeKernel %d\n", err);
    }

    err = clFinish(queue);
    if (err != CL_SUCCESS) {
        printf("Error number %d\n", err);
    }
    wline = (wline >= height_waterfall - 1) ? 0 : wline + 1;
    err |= clSetKernelArg(k_mag2mtx, 5, sizeof(wline), &wline);
    if (err != CL_SUCCESS) {
        printf("Error mag2img clSetKernelArg %d\n", err);
    }
    return err;
}

cl_int ClRsta::img2tex() {
    size_t dims[] = {size, height, 0};
    err = clEnqueueNDRangeKernel(queue, k_img2tex, 2, NULL, dims, NULL, 0, 
            NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Error clEnqueueNDRangeKernel %d\n", err);
    }

    err = clFinish(queue);
    if (err != CL_SUCCESS) {
        printf("Error clFinish %d\n", err);
    }
    
    return err;
}

cl_uint ClRsta::getWline() const {
    return wline;
}

cl_float2* ClRsta::getDout() const {
    return dout;
}

cl_float2* ClRsta::getDin() const {
    return din;
}

cl_int ClRsta::run() {

    err = 0;
    err |= cplxmulv();
    err |= fft();
    err |= cplx2db();
    err |= mag2img();
    err |= img2tex();
    
    if (err != CL_SUCCESS) {
        printf("error ClRsta::add %d\n", err);
    }
    return err;
}

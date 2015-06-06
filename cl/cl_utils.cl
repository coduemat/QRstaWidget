
inline float interpolate(float val, float y0, float x0, float y1, float x1) {
    return (val - x0) * (y1 - y0) * native_recip(x1 - x0) + y0;
}


inline float base(float val) {
    if (val <= -0.75f) return 0.0f;
    else if (val <= -0.25f) return interpolate(val, 0.0f, -0.75f, 1.0f, -0.25f);
    else if (val <=  0.25f) return 1.0f;
    else if (val <=  0.75f) return interpolate(val, 1.0f, 0.25f, 0.0f, 0.75f);
    else return 0.0f;
}

inline float4 gray2color(float gray) {
    float4 color;
    color.w = native_powr(gray, 0.5f);
    gray = (gray - 0.5f) * 2.0f;
    color.x = base(gray - 0.5f);
    color.y = base(gray       );
    color.z = base(gray + 0.5f);
    return color;
}

kernel void cplx2db(global float2* c, global float* m) {
    const int idx = get_global_id(0);
    m[idx] = log10(hypot(c[idx].x, c[idx].y)) * 10.0f;
}

kernel void cplxmulv(global float2* c, global float* v) {
    const int idx = get_global_id(0);
    c[idx].x *= v[idx];
    c[idx].y *= v[idx];
}

void lines(global float* img, uint width, uint height, int2 coord, float color) {

    if (coord.y >= height) {
        coord.y = -1;
    }
    const int range = 64;
    int b = (coord.y - range < 0) ? 0 : coord.y - range;
    for (int i = b; i <= coord.y; i++) {
        img[coord.x + i * width] +=  color;
    }

}

kernel void mag2frame(global float* mag, global float* frame,  
        write_only image2d_t waterfall, global float2 *spectrum, 
        uint height, uint wline, float level, float scale, float weight) {

    const size_t index = get_global_id(0);
    const size_t width = get_global_size(0);

    const int y   = (int)((mag[index] + level) * scale);
    const int pos = index + y * width;
    
    for (int j = 0; j < height; j++) {
        frame[index + j * width] = 0.0f;
    }

    lines(frame, width, height, (int2)(index, y), weight);
    // if (y >= 0 && y < height) {
    //     frame[pos] = weight;
    // }

    float color = (float)(y) * native_recip((float)height);
    color = clamp(color, 0.0f, 1.0f);
    write_imagef(waterfall, (int2)(index, wline), gray2color(color));
    
    int i = (2 * index < width) ? index + width / 2 : index - width / 2;
    spectrum[i].x = (float)i / (float)width;

    float m = mag[index];

    m = -((m + level) * scale) / (float)(height);
    m = clamp(m, -1.0f, 0.0f);

    if (m < spectrum[i].y) {
        spectrum[i].y = m * 0.97f + spectrum[i].y * 0.03f;
    }
    else {
        spectrum[i].y = m * 0.03f + spectrum[i].y * 0.97f;
    }
}

kernel void frame2tex(global float* frame, global float* sum, 
        write_only image2d_t tex, float decay) {
    const size_t x = get_global_id(0);
    const size_t y = get_global_id(1);
    const int width  = get_global_size(0);
    const int height = get_global_size(1);
    const int index = x + y * width;
    float4 color;
    float gray;

    sum[index] = sum[index] * (decay - frame[index]) + frame[index];
    sum[index] = clamp(sum[index], 0.0f, 1.0f);

    gray = (sum[index] - 0.5f) * 2.0f;
    gray = clamp(gray, -1.0f, 1.0f);
    color.x = base(gray - 0.5f);
    color.y = base(gray       );
    color.z = base(gray + 0.5f);
    // color.w = clamp(pow(sum[index], 0.5f), 0.0f, 1.0f);
    color.w = native_powr(sum[index], 0.5f);
    write_imagef(tex, (int2)(x, y), color);
}

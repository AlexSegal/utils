#pragma once
#include <vector>
#include <cstdint>
#include <Imath/half.h>
#include <libraw/libraw.h>

struct HalfImage {
    int width = 0, height = 0;
    std::vector<half> data; // RGB

    HalfImage() = default;
    HalfImage(int w, int h) : width(w), height(h), data(w*h*3) {}

    inline half* pixel(int x,int y){ return &data[(y*width+x)*3]; }
    inline const half* pixel(int x,int y) const { return &data[(y*width+x)*3]; }
};

// Convert LibRaw 16-bit to half-float
inline HalfImage convertLibRaw16ToHalf(const libraw_processed_image_t* memimg){
    if (!memimg || !memimg->data)
        throw std::runtime_error("Null image or data in convertLibRaw16ToHalf");
    if (memimg->type != LIBRAW_IMAGE_BITMAP || 
                        memimg->bits != 16 || 
                        memimg->colors != 3) {
        fprintf(stderr, "LibRaw image type: %d\n", memimg->type);
        fprintf(stderr, "LibRaw image bits: %d\n", memimg->bits);
        fprintf(stderr, "LibRaw image colors: %d\n", memimg->colors);
        throw std::runtime_error("Unsupported image format in convertLibRaw16ToHalf");
    }

    HalfImage out(memimg->width, memimg->height);
    const uint16_t* src = reinterpret_cast<const uint16_t*>(memimg->data);
    float inv = 1.0f / (float)0xFFFF;
    

    for (size_t i = 0; i<out.data.size(); ++i) {
        out.data[i] = half(src[i]*inv);
    }

    return out;
}

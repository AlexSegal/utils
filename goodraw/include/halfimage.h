#pragma once
#include <vector>
#include <cstdint>
#include <Imath/half.h>
#include <libraw/libraw.h>

/**
 * @brief Half-precision float RGB image container for high dynamic range
 * 
 * Stores image data as RGB triplets using half-precision floats for memory
 * efficiency while preserving HDR precision. Used throughout the pipeline
 * for color-managed image processing.
 */
struct HalfImage {
    int width = 0, height = 0;            ///< Image dimensions in pixels
    std::vector<half> data;               ///< Interleaved RGB data (3 * width * height)

    /// Default constructor for empty image
    HalfImage() = default;
    
    /**
     * @brief Construct image with specified dimensions
     * @param w Image width in pixels
     * @param h Image height in pixels
     */
    HalfImage(int w, int h) : width(w), height(h), data(w*h*3) {}

    /**
     * @brief Get mutable pointer to RGB pixel data
     * @param x Pixel X coordinate (0 to width-1)
     * @param y Pixel Y coordinate (0 to height-1)
     * @return Pointer to 3-element half array [R,G,B]
     */
    inline half* pixel(int x,int y){ return &data[(y*width+x)*3]; }
    
    /**
     * @brief Get const pointer to RGB pixel data
     * @param x Pixel X coordinate (0 to width-1)
     * @param y Pixel Y coordinate (0 to height-1)
     * @return Const pointer to 3-element half array [R,G,B]
     */
    inline const half* pixel(int x,int y) const { return &data[(y*width+x)*3]; }
};

/**
 * @brief Convert LibRaw 16-bit integer output to half-precision float
 * 
 * Converts LibRaw's processed 16-bit RGB image to normalized half-float
 * values in [0,1] range. Essential for maintaining precision in HDR workflow.
 * 
 * @param memimg LibRaw processed image (must be 16-bit RGB bitmap)
 * @return HalfImage with normalized RGB values
 * @throws std::runtime_error if image format is unsupported
 */
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

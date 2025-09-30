/**
 * @file rawdecoder.cpp
 * @brief LibRaw integration for RAW file processing
 * 
 * Implements RAW image loading using LibRaw library with 16-bit output
 * for maximum precision in the ACES color pipeline.
 */

#include "rawdecoder.h"
#include <stdexcept>
#include <memory>

/**
 * @brief Load and process RAW image file using LibRaw
 * 
 * Opens RAW file, applies basic processing (demosaicing, color space conversion),
 * and returns 16-bit RGB image with embedded color metadata for ACES workflow.
 * 
 * @param path Absolute path to RAW image file (.cr2, .nef, .arw, etc.)
 * @return RawImageResult containing processed image and color data
 * @throws std::runtime_error if file cannot be opened or processed
 */
RawImageResult loadRawImage(const std::string& path){
    // Use heap allocation instead of stack to avoid stack overflow
    std::unique_ptr<LibRaw> raw_ptr = std::make_unique<LibRaw>();
    LibRaw& raw = *raw_ptr;
    raw.imgdata.params.output_bps = 16; // Request 16 bits per channel output
        
    if(raw.open_file(path.c_str()) != LIBRAW_SUCCESS) {
            throw std::runtime_error("Failed to open RAW");
        }
    
    if(raw.unpack() != LIBRAW_SUCCESS) {
        throw std::runtime_error("Failed to unpack RAW");
    }
    
    if(raw.dcraw_process() != LIBRAW_SUCCESS) {
        throw std::runtime_error("Failed to process RAW");
    }

    libraw_processed_image_t* img = raw.dcraw_make_mem_image();
    libraw_colordata_t color = raw.imgdata.color;
    
    return RawImageResult{img, color};
}

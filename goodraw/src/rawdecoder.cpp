// rawdecoder.cpp
#include "rawdecoder.h"
#include <stdexcept>

RawImageResult loadRawImage(const std::string& path){
    LibRaw raw;
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

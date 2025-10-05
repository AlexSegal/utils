/**
 * @file rawdecoder.cpp
 * @brief LibRaw integration for RAW file processing
 * 
 * Implements RAW image loading using LibRaw library with 16-bit linear sRGB output
 * for accurate color reproduction.
 */

#include "rawdecoder.h"
#include <stdexcept>
#include <memory>
#include <QDebug>

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
    
    // Configure LibRaw to output linear XYZ color space
    raw.imgdata.params.output_bps = 16;        // 16 bits per channel
    raw.imgdata.params.gamm[0] = 1.0;          // Linear gamma (no gamma correction)
    raw.imgdata.params.gamm[1] = 1.0;          // Linear gamma (no gamma correction)
    raw.imgdata.params.no_auto_bright = 0;     // Enable auto-brightness
    raw.imgdata.params.output_color = 5;       // XYZ color space (D65 illuminant)
        
    raw.imgdata.params.use_camera_wb = 1;
    raw.imgdata.params.use_camera_matrix = 1;
    
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
    
    // Extract exposure compensation from multiple possible sources
    float exposureComp = 0.0f;
    
    // Try common metadata first (most universal)
    if (raw.imgdata.makernotes.common.ExposureCalibrationShift != 0.0f) {
        exposureComp = raw.imgdata.makernotes.common.ExposureCalibrationShift;
        qDebug() << "Using ExposureCalibrationShift:" << exposureComp << "EV";
    }
    // Fallback to Fuji-specific field
    else if (raw.imgdata.makernotes.fuji.BrightnessCompensation != 0.0f) {
        exposureComp = raw.imgdata.makernotes.fuji.BrightnessCompensation;
        qDebug() << "Using Fuji BrightnessCompensation:" << exposureComp << "EV";
    }
    else {
        qDebug() << "No exposure compensation found in checked fields";
    }
    
    // Comprehensive debug output for EXIF exploration
    qDebug() << "=== EXIF Exposure Compensation Analysis ===";
    qDebug() << "Final exposure compensation:" << exposureComp << "EV";
    qDebug() << "Common.ExposureCalibrationShift:" << raw.imgdata.makernotes.common.ExposureCalibrationShift;
    qDebug() << "Fuji.BrightnessCompensation:" << raw.imgdata.makernotes.fuji.BrightnessCompensation;
    qDebug() << "Common.FlashEC:" << raw.imgdata.makernotes.common.FlashEC;
    qDebug() << "DNG.baseline_exposure:" << raw.imgdata.color.dng_levels.baseline_exposure;
    qDebug() << "Canon.ExposureMode:" << raw.imgdata.makernotes.canon.ExposureMode;
    qDebug() << "=== Basic EXIF Data ===";
    qDebug() << "Aperture:" << raw.imgdata.other.aperture;
    qDebug() << "Shutter speed:" << raw.imgdata.other.shutter;
    qDebug() << "ISO speed:" << raw.imgdata.other.iso_speed;
    qDebug() << "Focal length:" << raw.imgdata.other.focal_len;
    
    return RawImageResult{img, color, exposureComp};
}

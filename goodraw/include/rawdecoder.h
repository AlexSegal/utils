// rawdecoder.h
#pragma once
#include <libraw/libraw.h>
#include <memory>

/**
 * @brief Container for LibRaw processing results with color metadata
 * 
 * Bundles the processed image data with camera color information needed
 * for ACES color space transformations and white balance calculations.
 */
struct RawImageResult {
	libraw_processed_image_t* image;      ///< Processed 16-bit RGB image data
	libraw_colordata_t color;             ///< Camera color matrices and white balance
};

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
RawImageResult loadRawImage(const std::string& path);

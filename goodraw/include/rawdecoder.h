// rawdecoder.h
#pragma once
#include <libraw/libraw.h>
#include <libraw/libraw.h>

struct RawImageResult {
	libraw_processed_image_t* image;
	libraw_colordata_t color;
};

RawImageResult loadRawImage(const std::string& path);

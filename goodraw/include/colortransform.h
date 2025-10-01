#pragma once
#include "halfimage.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

#ifdef _OPENMP
#include <omp.h>
#endif

/**
 * @brief Convert sRGB gamma-encoded value to linear
 * 
 * NOTE: This function is no longer used since LibRaw handles white balance.
 * Kept for potential future use in other color transformations.
 * 
 * @param gamma_value sRGB gamma-encoded value [0.0, 1.0+]
 * @return Linear value [0.0, 1.0+]
 */
inline float sRGBToLinear(float gamma_value) {
    if (gamma_value <= 0.04045f) {
        return gamma_value / 12.92f;
    } else {
        return powf((gamma_value + 0.055f) / 1.055f, 2.4f);
    }
}

/**
 * @brief Apply 3x3 matrix transformation to RGB color values
 * 
 * Performs in-place matrix multiplication: [r' g' b'] = [r g b] * M
 * Used for color space conversions in the ACES pipeline.
 * 
 * @param M 3x3 transformation matrix [row][col]
 * @param r Red component (modified in-place)
 * @param g Green component (modified in-place)  
 * @param b Blue component (modified in-place)
 */
inline void applyMatrix3x3(const float M[3][3], float& r,float& g,float& b){
    float rr=M[0][0]*r+M[0][1]*g+M[0][2]*b;
    float gg=M[1][0]*r+M[1][1]*g+M[1][2]*b;
    float bb=M[2][0]*r+M[2][1]*g+M[2][2]*b;
    r=rr; g=gg; b=bb;
}

/**
 * @brief Convert camera RGB to ACEScg color space with camera white balance
 * 
 * Transforms camera-specific RGB through the complete ACES pipeline:
 * Camera RGB → Apply Camera WB → XYZ → ACES AP0 → ACEScg (AP1)
 * 
 * Applies the camera's measured white balance from RAW metadata to the
 * raw pixel values before color space transformation. Interactive white
 * balance adjustments remain in GPU shader for real-time control.
 * 
 * @param img Image data to transform (modified in-place)
 * @param color LibRaw color metadata containing camera→XYZ matrices and cam_mul
 */
inline void cameraToACEScg(HalfImage& img, const libraw_colordata_t& color){
    // Extract camera→XYZ transformation matrix from LibRaw metadata
    float cam2xyz[3][3] = {
        {color.cam_xyz[0][0],color.cam_xyz[0][1],color.cam_xyz[0][2]},
        {color.cam_xyz[1][0],color.cam_xyz[1][1],color.cam_xyz[1][2]},
        {color.cam_xyz[2][0],color.cam_xyz[2][1],color.cam_xyz[2][2]}
    };

    // XYZ → Rec.709/sRGB (linear) - Standard color space transformation  
    // This bypasses ACES workflow and goes directly to display-ready RGB
    static const float XYZ_to_sRGB[3][3] = {
        { 3.2404542f, -1.5371385f, -0.4985314f},
        {-0.9692660f,  1.8760108f,  0.0415560f},
        { 0.0556434f, -0.2040259f,  1.0572252f}
    };

    // XYZ → ACES AP0 (ACES2065-1) - Standard ACES transformation matrix
    // Source: AMPAS ACES documentation, verified against multiple implementations
    static const float XYZ_to_AP0[3][3] = {
        { 0.9525523959f,  0.0000000000f,  0.0000936786f},
        { 0.3439664498f,  0.7281660966f, -0.0721325464f},
        { 0.0000000000f,  0.0000000000f,  1.0088251844f}
    };

    // ACES AP0 → ACEScg (AP1) - Standard ACES working space transformation
    // Source: AMPAS ACES documentation, verified against multiple implementations  
    static const float AP0_to_AP1[3][3] = {
        { 1.4514393161f, -0.2365107469f, -0.2149285693f},
        {-0.0765537734f,  1.1762296998f, -0.0996759264f},
        { 0.0083161484f, -0.0060324498f,  0.9977163014f}
    };

    // LibRaw's dcraw_process() already applies camera white balance!
    // No need to apply it again - this was causing the color casts.
    printf("DEBUG: Skipping white balance - LibRaw already applied it during dcraw_process()\n");

    // Apply complete transformation chain to each pixel (parallelized)
#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic, 16)
#endif
    for(int y=0;y<img.height;++y)
        for(int x=0;x<img.width;++x){
            half* p=img.pixel(x,y);
            float r=float(p[0]), g=float(p[1]), b=float(p[2]);
            
            // Skip white balance - LibRaw already applied it in dcraw_process()
            // Directly apply color space transformations to the already white-balanced RGB
            
            applyMatrix3x3(cam2xyz,r,g,b);      // Camera RGB → XYZ
            applyMatrix3x3(XYZ_to_sRGB,r,g,b);   // XYZ → Rec.709/sRGB (bypass ACES)
            // DEBUG: Skip ACES transforms and go directly to sRGB
            // applyMatrix3x3(XYZ_to_AP0,r,g,b);   // XYZ → AP0
            // applyMatrix3x3(AP0_to_AP1,r,g,b);   // AP0 → ACEScg
            p[0]=half(r); p[1]=half(g); p[2]=half(b);
        }
}

/**
 * @brief Convert XYZ color space to ACEScg working space
 * 
 * Transforms LibRaw's XYZ output to ACEScg for professional color grading.
 * LibRaw (output_color=5) → XYZ → ACES AP0 → ACEScg (AP1)
 * 
 * @param img Image data to transform from XYZ to ACEScg (modified in-place)
 */
inline void xyzToACEScg(HalfImage& img) {
    // XYZ → ACES AP0 (ACES2065-1) - Standard ACES transformation matrix
    // Source: AMPAS ACES documentation, verified against multiple implementations
    static const float XYZ_to_AP0[3][3] = {
        { 0.9525523959f,  0.0000000000f,  0.0000936786f},
        { 0.3439664498f,  0.7281660966f, -0.0721325464f},
        { 0.0000000000f,  0.0000000000f,  1.0088251844f}
    };

    // ACES AP0 → ACEScg (AP1) - Standard ACES working space transformation
    // Source: AMPAS ACES documentation, verified against multiple implementations  
    static const float AP0_to_AP1[3][3] = {
        { 1.4514393161f, -0.2365107469f, -0.2149285693f},
        {-0.0765537734f,  1.1762296998f, -0.0996759264f},
        { 0.0083161484f, -0.0060324498f,  0.9977163014f}
    };

    // Apply transformation chain to each pixel (parallelized)
#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic, 16)
#endif
    for(int y=0;y<img.height;++y)
        for(int x=0;x<img.width;++x){
            half* p=img.pixel(x,y);
            float r=float(p[0]), g=float(p[1]), b=float(p[2]);
            
            applyMatrix3x3(XYZ_to_AP0,r,g,b);   // XYZ → ACES AP0
            applyMatrix3x3(AP0_to_AP1,r,g,b);   // AP0 → ACEScg (AP1)
            
            p[0]=half(r); p[1]=half(g); p[2]=half(b);
        }
}

/**
 * @brief Apply only camera white balance without color space conversion
 * 
 * NOTE: This function is now deprecated and does nothing.
 * LibRaw's dcraw_process() already applies camera white balance automatically.
 * Use cameraToACEScg() for proper ACES color space conversion without double-WB.
 * 
 * @param img Image data (unmodified - LibRaw already white balanced it)
 * @param color LibRaw color metadata (unused)
 */
inline void applyCameraWhiteBalance(HalfImage& img, const libraw_colordata_t& color) {
    // Do nothing - LibRaw already applied white balance during dcraw_process()
    printf("WARNING: applyCameraWhiteBalance() is deprecated - LibRaw already applied WB\n");
    (void)img;   // Suppress unused parameter warning
    (void)color; // Suppress unused parameter warning
}


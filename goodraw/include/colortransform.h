#pragma once
#include "halfimage.h"

#ifdef _OPENMP
#include <omp.h>
#endif

// Forward declaration for OCIO display transform (implemented in ocio_transform.cpp)
void acesCgToDisplay(HalfImage& img);

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
 * @brief Convert camera RGB to ACEScg color space for color grading
 * 
 * Transforms camera-specific RGB through the complete ACES pipeline:
 * Camera RGB → XYZ → ACES AP0 → ACEScg (AP1)
 * 
 * This ensures accurate color reproduction and enables professional
 * color grading workflows compatible with film/TV standards.
 * 
 * @param img Image data to transform (modified in-place)
 * @param color LibRaw color metadata containing camera→XYZ matrices
 */
inline void cameraToACEScg(HalfImage& img, const libraw_colordata_t& color){
    // Extract camera→XYZ transformation matrix from LibRaw metadata
    float cam2xyz[3][3] = {
        {color.cam_xyz[0][0],color.cam_xyz[0][1],color.cam_xyz[0][2]},
        {color.cam_xyz[1][0],color.cam_xyz[1][1],color.cam_xyz[1][2]},
        {color.cam_xyz[2][0],color.cam_xyz[2][1],color.cam_xyz[2][2]}
    };

    // XYZ → ACES AP0 (Academy Color Encoding System primaries)
    static const float XYZ_to_AP0[3][3] = {
        {0.9525523959f,0.0f,0.0000936786f},
        {0.3439664498f,0.7281660966f,-0.0721325464f},
        {0.0f,0.0f,1.0088251844f}
    };

    // ACES AP0 → ACEScg (working space optimized for CG)
    static const float AP0_to_AP1[3][3] = {
        {1.4514393161f,-0.2365107469f,-0.2149285693f},
        {-0.0765537733f,1.1762296998f,-0.0996759265f},
        {0.0083161484f,-0.0060324498f,0.9977163014f}
    };

    // Apply complete transformation chain to each pixel (parallelized)
#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic, 16)
#endif
    for(int y=0;y<img.height;++y)
        for(int x=0;x<img.width;++x){
            half* p=img.pixel(x,y);
            float r=float(p[0]), g=float(p[1]), b=float(p[2]);
            applyMatrix3x3(cam2xyz,r,g,b);      // Camera RGB → XYZ
            applyMatrix3x3(XYZ_to_AP0,r,g,b);   // XYZ → AP0
            applyMatrix3x3(AP0_to_AP1,r,g,b);   // AP0 → ACEScg
            p[0]=half(r); p[1]=half(g); p[2]=half(b);
        }
}



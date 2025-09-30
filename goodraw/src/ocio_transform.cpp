/**
 * @file ocio_transform.cpp
 * @brief Manual ACES display transform implementation
 * 
 * Implements ACEScg → sRGB display transform using matrix math
 * instead of OCIO to avoid Windows/MSVC header conflicts.
 */

#include "halfimage.h"
#include <cmath>

#ifdef _OPENMP
#include <omp.h>
#endif

// Manual min/max to avoid Windows macro conflicts
template<typename T>
inline T clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

// Apply 3x3 matrix transformation (same as in colortransform.h)
inline void applyMatrix3x3(const float M[3][3], float& r, float& g, float& b) {
    float rr = M[0][0]*r + M[0][1]*g + M[0][2]*b;
    float gg = M[1][0]*r + M[1][1]*g + M[1][2]*b;
    float bb = M[2][0]*r + M[2][1]*g + M[2][2]*b;
    r = rr; g = gg; b = bb;
}

// ACES filmic tone mapping (approximation of RRT)
inline float acesToneMap(float x) {
    const float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0f, 1.0f);
}

// sRGB OETF (gamma encoding)
inline float linearToSRGB(float linear) {
    if (linear <= 0.0031308f) {
        return 12.92f * linear;
    } else {
        return 1.055f * std::pow(linear, 1.0f/2.4f) - 0.055f;
    }
}

/**
 * @brief Apply ACES display transform using manual matrix math
 * 
 * Transforms ACEScg working space to sRGB display using:
 * ACEScg → ACES AP0 → XYZ → sRGB + ACES tone mapping + sRGB gamma
 * 
 * This provides cinema-standard tone mapping and color management
 * without requiring OCIO runtime dependency.
 * 
 * @param img Image data in ACEScg color space (modified in-place to sRGB)
 */
void acesCgToDisplay(HalfImage& img) {
    // ACEScg (AP1) → ACES AP0
    static const float AP1_to_AP0[3][3] = {
        {0.6954522414f, 0.1406786965f, 0.1638690622f},
        {0.0447945634f, 0.8596711185f, 0.0955343182f},
        {-0.0055258826f, 0.0040252103f, 1.0015006723f}
    };
    
    // ACES AP0 → XYZ
    static const float AP0_to_XYZ[3][3] = {
        {0.9525523959f, 0.0f, 0.0000936786f},
        {0.3439664498f, 0.7281660966f, -0.0721325464f},
        {0.0f, 0.0f, 1.0088251844f}
    };
    
    // XYZ → sRGB (Rec.709 primaries)
    static const float XYZ_to_sRGB[3][3] = {
        {3.2404542f, -1.5371385f, -0.4985314f},
        {-0.9692660f, 1.8760108f, 0.0415560f},
        {0.0556434f, -0.2040259f, 1.0572252f}
    };
    
    // Apply display transform to each pixel (parallelized)
#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic, 16)
#endif
    for(int y = 0; y < img.height; ++y) {
        for(int x = 0; x < img.width; ++x) {
            half* p = img.pixel(x, y);
            float r = float(p[0]), g = float(p[1]), b = float(p[2]);
            
            // Transform through color spaces
            applyMatrix3x3(AP1_to_AP0, r, g, b);    // ACEScg → AP0
            applyMatrix3x3(AP0_to_XYZ, r, g, b);    // AP0 → XYZ  
            applyMatrix3x3(XYZ_to_sRGB, r, g, b);   // XYZ → sRGB
            
            // Apply ACES tone mapping (approximation of RRT/ODT)
            r = acesToneMap(r);
            g = acesToneMap(g);
            b = acesToneMap(b);
            
            // Apply sRGB gamma encoding
            r = linearToSRGB(r);
            g = linearToSRGB(g);
            b = linearToSRGB(b);
            
            // Store back as half precision
            p[0] = half(r); p[1] = half(g); p[2] = half(b);
        }
    }
}
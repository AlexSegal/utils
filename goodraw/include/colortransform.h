#pragma once
#include "halfimage.h"

inline void applyMatrix3x3(const float M[3][3], float& r,float& g,float& b){
    float rr=M[0][0]*r+M[0][1]*g+M[0][2]*b;
    float gg=M[1][0]*r+M[1][1]*g+M[1][2]*b;
    float bb=M[2][0]*r+M[2][1]*g+M[2][2]*b;
    r=rr; g=gg; b=bb;
}

// Camera RGB -> ACEScg
inline void cameraToACEScg(HalfImage& img,const libraw_colordata_t& color){
    float cam2xyz[3][3] = {
        {color.cam_xyz[0][0],color.cam_xyz[0][1],color.cam_xyz[0][2]},
        {color.cam_xyz[1][0],color.cam_xyz[1][1],color.cam_xyz[1][2]},
        {color.cam_xyz[2][0],color.cam_xyz[2][1],color.cam_xyz[2][2]}
    };

    static const float XYZ_to_AP0[3][3] = {
        {0.9525523959f,0.0f,0.0000936786f},
        {0.3439664498f,0.7281660966f,-0.0721325464f},
        {0.0f,0.0f,1.0088251844f}
    };

    static const float AP0_to_AP1[3][3] = {
        {1.4514393161f,-0.2365107469f,-0.2149285693f},
        {-0.0765537733f,1.1762296998f,-0.0996759265f},
        {0.0083161484f,-0.0060324498f,0.9977163014f}
    };

    for(int y=0;y<img.height;++y)
        for(int x=0;x<img.width;++x){
            half* p=img.pixel(x,y);
            float r=float(p[0]), g=float(p[1]), b=float(p[2]);
            applyMatrix3x3(cam2xyz,r,g,b);
            applyMatrix3x3(XYZ_to_AP0,r,g,b);
            applyMatrix3x3(AP0_to_AP1,r,g,b);
            p[0]=half(r); p[1]=half(g); p[2]=half(b);
        }
}

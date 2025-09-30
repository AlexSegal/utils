/**
 * @file glsl_shaders.h
 * @brief OpenGL 3.3 vertex and fragment shaders for real-time RAW processing
 * 
 * Contains GLSL shaders for rendering RAW images with real-time exposure,
 * white balance, contrast, and ACES tone mapping. Uses matrix-based
 * coordinate transformations for smooth navigation.
 */

/**
 * @brief Vertex shader for image quad rendering with matrix transformations
 * 
 * Applies 2D transformation matrix for zoom, pan, and rotation.
 * Transforms quad vertices from normalized coordinates to screen space.
 */
static const char* vertexShaderSource = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aTex;
out vec2 TexCoord;

uniform mat3 transform;  // 2D transformation matrix

void main(){
    // Apply transformation matrix to position
    vec3 pos = transform * vec3(aPos, 1.0);
    gl_Position = vec4(pos.xy, 0.0, 1.0);
    TexCoord = aTex;
}
)";

/**
 * @brief Fragment shader for RAW image processing and tone mapping
 * 
 * Applies exposure compensation, white balance, contrast adjustment,
 * and ACES filmic tone mapping for cinema-quality output. Handles
 * edge clamping to prevent texture bleeding during transformations.
 */
static const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D _m_tex;
uniform float exposure;
uniform vec3 wb;
uniform float contrast;
uniform bool showGrid;

// Note: ACES tone mapping now handled by OCIO display transform on CPU

void main(){
    // Clamp texture coordinates to prevent edge streaking
    vec2 clampedTexCoord = clamp(TexCoord, 0.0, 1.0);
    
    // If we're outside the valid texture area, render black
    if (TexCoord.x < 0.0 || TexCoord.x > 1.0 || TexCoord.y < 0.0 || TexCoord.y > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // Sample RGB image and apply processing pipeline
    // Note: Image is now in sRGB display space (via OCIO display transform)
    vec3 color = texture(_m_tex, clampedTexCoord).rgb;
    color *= wb;                                    // White balance correction
    color *= pow(2.0,exposure);                     // Exposure adjustment (stops)
    color = (color-0.5)*contrast + 0.5;           // Contrast around middle gray
    color = clamp(color, 0.0, 1.0);               // Clamp to display range

    // Optional rotation grid overlay for alignment
    if(showGrid){
        float gx = abs(fract(TexCoord.x*3.0-0.5)-0.5)*6.0;
        float gy = abs(fract(TexCoord.y*3.0-0.5)-0.5)*6.0;
        if(gx < 0.02 || gy < 0.02) color = vec3(1.0);
    }

    FragColor = vec4(color,1.0);
}
)";
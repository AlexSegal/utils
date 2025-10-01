/**
 * @file glsl_shaders.h
 * @brief OpenGL 3.3 vertex and fragment shaders for real-time RAW processing
 * 
 * Contains GLSL shaders for rendering linear ACEScg images with real-time 
 * color corrections, ACES display transform, and tone mapping. All processing
 * happens in GPU for maximum performance with proper linear color science.
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
 * @brief Fragment shader for real-time ACEScg processing and display transform
 * 
 * Input: Linear ACEScg color space data from CPU-side RAW processing
 * Processing: Color corrections (exposure, white balance, contrast) in linear ACEScg space
 * Display Transform: Direct ACEScg→sRGB conversion with ACES filmic tone mapping
 * Output: sRGB display-ready pixels with proper gamma encoding
 */
static const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D _m_tex;
uniform float exposure;
//uniform vec3 wb;
uniform float contrast;
uniform bool showGrid;

// DEBUG: Let's bypass the problematic ACEScg→sRGB matrix and go back to 
// treating the data as linear sRGB to isolate the color space issue
// If this fixes the green tint, we know the matrix is wrong

// ACEScg → sRGB transformation matrix (currently suspected to be incorrect)
const mat3 ACEScg_to_sRGB = transpose(mat3(
    1.7050515, -0.6217923, -0.0832592,
    -0.1302597, 1.1408027, -0.0105430,
    -0.0240003, -0.1289687, 1.1529690
));

// ACES filmic tone mapping (approximation of RRT/ODT)
vec3 acesToneMap(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

// sRGB OETF (gamma encoding)
vec3 linearToSRGB(vec3 linear) {
    return mix(
        12.92 * linear,
        1.055 * pow(linear, vec3(1.0/2.4)) - 0.055,
        step(0.0031308, linear)
    );
}

void main(){
    // Clamp texture coordinates to prevent edge streaking
    vec2 clampedTexCoord = clamp(TexCoord, 0.0, 1.0);
    
    // If we're outside the valid texture area, render black
    if (TexCoord.x < 0.0 || TexCoord.x > 1.0 || TexCoord.y < 0.0 || TexCoord.y > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // Sample linear ACEScg image data
    vec3 acesCg = texture(_m_tex, clampedTexCoord).rgb;
    
    // Apply color corrections in linear ACEScg space (proper color science!)
    //acesCg *= wb;                              // Interactive white balance (ACEScg space)
    acesCg *= pow(2.0, exposure);              // Exposure adjustment (stops)
    acesCg = max(vec3(0.0), (acesCg - 0.18) * contrast + 0.18); // Contrast around 18% gray, clamped to prevent negative values
    
    // DEBUG: Skip ACEScg→sRGB conversion to isolate the matrix issue
    // If this fixes green tint, the problem is in our color space pipeline
    vec3 srgb_linear = acesCg;  // Treat ACEScg as linear sRGB (temporary debug)
    
    // Apply ACES filmic tone mapping for cinematic look
    srgb_linear = acesToneMap(srgb_linear);
    
    // Apply sRGB gamma encoding for display
    vec3 srgb_display = linearToSRGB(srgb_linear);

    // Optional rotation grid overlay for alignment
    if(showGrid){
        float gx = abs(fract(TexCoord.x*3.0-0.5)-0.5)*6.0;
        float gy = abs(fract(TexCoord.y*3.0-0.5)-0.5)*6.0;
        if(gx < 0.02 || gy < 0.02) srgb_display = vec3(1.0);
    }

    FragColor = vec4(srgb_display, 1.0);
}
)";
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


static const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D _m_tex;
uniform float exposure;
uniform vec3 wb;
uniform float contrast;
uniform bool showGrid;

vec3 ACESFilm(vec3 x){
    const float a=2.51,b=0.03,c=2.43,d=0.59,e=0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e),0.0,1.0);
}

void main(){
    // Clamp texture coordinates to prevent edge streaking
    vec2 clampedTexCoord = clamp(TexCoord, 0.0, 1.0);
    
    // If we're outside the valid texture area, render black
    if (TexCoord.x < 0.0 || TexCoord.x > 1.0 || TexCoord.y < 0.0 || TexCoord.y > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    vec3 color = texture(_m_tex, clampedTexCoord).rgb;
    color *= wb;
    color *= pow(2.0,exposure);
    color = (color-0.5)*contrast + 0.5;
    color = ACESFilm(color);

    if(showGrid){
        float gx = abs(fract(TexCoord.x*3.0-0.5)-0.5)*6.0;
        float gy = abs(fract(TexCoord.y*3.0-0.5)-0.5)*6.0;
        if(gx < 0.02 || gy < 0.02) color = vec3(1.0);
    }

    FragColor = vec4(color,1.0);
}
)";

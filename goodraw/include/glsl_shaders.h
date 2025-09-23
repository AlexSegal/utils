static const char* vertexShaderSource = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aTex;
out vec2 TexCoord;

uniform float zoom;
uniform vec2 pan;
uniform vec2 cropCenter;
uniform vec2 cropSize;
uniform float rotation;

void main(){
    vec2 pos = (aPos-vec2(0.5))*cropSize + cropCenter;
    float s = sin(rotation), c = cos(rotation);
    pos = vec2(
        c*(pos.x-cropCenter.x) - s*(pos.y-cropCenter.y) + cropCenter.x,
        s*(pos.x-cropCenter.x) + c*(pos.y-cropCenter.y) + cropCenter.y
    );
    pos = pos*zoom + pan;
    gl_Position = vec4(pos*2.0-1.0,0.0,1.0);
    TexCoord = aTex*cropSize + cropCenter - cropSize*0.5;
}
)";


static const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D tex;
uniform float exposure;
uniform vec3 wb;
uniform float contrast;
uniform bool showGrid;

vec3 ACESFilm(vec3 x){
    const float a=2.51,b=0.03,c=2.43,d=0.59,e=0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e),0.0,1.0);
}

void main(){
    vec3 color = texture(tex, TexCoord).rgb;
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

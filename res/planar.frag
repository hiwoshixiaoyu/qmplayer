#ifdef GL_ES
precision highp int;
precision highp float;
#endif


uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;

//uniform float tex_format = 1.0;
//uniform float alpha = 1.0;
//uniform float tex_offset = 0;
//uniform float imageWidth = 0;
//uniform float imageHeight = 0;
//uniform bool enableHDR = false;
//uniform bool enableGaussianBlur = false;


in vec2 oTexCoord;

void main()
{
    vec3 yuv;
    vec3 rgb;

    yuv.x = texture2D(tex_y, oTexCoord).r;
    yuv.y = texture2D(tex_u, oTexCoord).r - 0.5;
    yuv.z = texture2D(tex_v, oTexCoord).r - 0.5;
    rgb = mat3(1.0, 1.0, 1.0,
        0.0, -0.39465, 2.03211,
        1.13983, -0.58060, 0.0) * yuv;
    gl_FragColor = vec4(rgb, 1.0);
}

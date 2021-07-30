#ifdef GL_ES
precision highp int;
precision highp float;
#endif

//定义高精度 顶点坐标
attribute highp vec4 vPosition;

//纹理坐标
attribute highp vec2 vTexture;


//CPU->GPU内存的矩阵
uniform highp mat4 Mmatrix;
uniform highp mat4 Vmatrix;
uniform highp mat4 Pmatrix;


//输出纹理坐标
varying vec2 oTexCoord;

void main(void)
{
    //顶点输出位置
    gl_Position = Mmatrix *vPosition;
    oTexCoord = vTexture;
}

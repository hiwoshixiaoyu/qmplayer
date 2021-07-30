#ifndef QMPLAYER_H
#define QMPLAYER_H
#include <QMutex>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QTime>

#include <libavutil/frame.h>

struct VideoFormat{
    float width;
    float height;
    float rotate;
    int format;

    AVFrame *renderFrame;
    QMutex *renderFrameMutex;
};

class RenderParams{
public :
    RenderParams(AVPixelFormat f,AVRational yw,AVRational uw,AVRational vw,AVRational yh,AVRational uh,AVRational vh,AVRational y,AVRational u,AVRational v,GLint yf1,GLenum yf2,GLint uf1,GLenum uf2,GLint vf1,GLenum vf2,int f3,bool planar,GLenum dataType = GL_UNSIGNED_BYTE)
        : format(f)
        , yWidth(yw)
        , uWidth(uw)
        , vWidth(vw)

        , yHeight(yh)
        , uHeight(uh)
        , vHeight(vh)

        , ySize(y)
        , uSize(u)
        , vSize(v)

        , yInternalformat(yf1)
        , uInternalformat(uf1)
        , vInternalformat(vf1)
        , yGlFormat(yf2)
        , uGlFormat(uf2)
        , vGlFormat(vf2)
        , textureFormat(f3)

        , isPlanar(planar)
        , dataType(dataType)
    {

        yuvsizes[0] = ySize;
        yuvsizes[1] = uSize;
        yuvsizes[2] = vSize;

        yuvwidths[0] = yWidth;
        yuvwidths[1] = uWidth;
        yuvwidths[2] = vWidth;

        yuvheights[0] = yHeight;
        yuvheights[1] = uHeight;
        yuvheights[2] = vHeight;

        yuvInternalformat[0] = yInternalformat;
        yuvInternalformat[1] = uInternalformat;
        yuvInternalformat[2] = vInternalformat;

        yuvGlFormat[0] = yGlFormat;
        yuvGlFormat[1] = uGlFormat;
        yuvGlFormat[2] = vGlFormat;
    }

    RenderParams(AVPixelFormat f,AVRational yw,AVRational uw,AVRational vw,AVRational yh,AVRational uh,AVRational vh,AVRational y,AVRational u,AVRational v,GLint f1,GLenum f2,int f3,bool planar,GLenum dataType = GL_UNSIGNED_BYTE): RenderParams(f,yw,uw,vw,yh,uh,vh,y,u,v,f1,f2,f1,f2,f1,f2,f3,planar,dataType){}
    RenderParams():RenderParams(AV_PIX_FMT_YUV420P,{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},0,0,0,false){}
    RenderParams (const RenderParams &r)
        : RenderParams(r.format,
                       r.yWidth,r.uWidth,r.vWidth,
                       r.yHeight,r.uHeight,r.vHeight,
                       r.ySize,r.uSize,r.vSize,
                       r.yInternalformat,r.yGlFormat,
                       r.uInternalformat,r.uGlFormat,
                       r.vInternalformat,r.vGlFormat,
                       r.textureFormat,r.isPlanar,
                       r.dataType)
    {
    }

    AVPixelFormat format;

    AVRational yWidth; //y的宽度(比率)
    AVRational uWidth; //u的宽度(比率)
    AVRational vWidth; //v的宽度(比率)

    AVRational yHeight; //y的高度(比率)
    AVRational uHeight; //u的高度(比率)
    AVRational vHeight; //v的高度(比率)

    AVRational ySize;// y的大小(比率)
    AVRational uSize;// u的大小(比率)
    AVRational vSize;// v的大小(比率)

    GLint yInternalformat; //数据位数
    GLint uInternalformat; //数据位数
    GLint vInternalformat; //数据位数

    GLenum yGlFormat; //
    GLenum uGlFormat; //
    GLenum vGlFormat; //

    int textureFormat; //自定义纹理格式，GL使用

    bool isPlanar; //是否是平面 true : 平面 | false : 打包

    AVRational yuvsizes[3];
    AVRational yuvwidths[3];
    AVRational yuvheights[3];
    GLint yuvInternalformat[3];
    GLenum yuvGlFormat[3];

    GLenum dataType;
};

class qmplayer:public QOpenGLWidget,public QOpenGLFunctions
{
public:
    qmplayer(QWidget *parent);

    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();


    void setVideoFormat(VideoFormat m_format);


     void init();
    static const int TEXTURE_NUMBER = 3;
    QOpenGLBuffer m_pbo[2][TEXTURE_NUMBER];

    //顶点数组对象
    QOpenGLVertexArrayObject *m_vao;
    //顶点数组缓冲对象
    QOpenGLBuffer *m_vbo;
    //索引数组缓冲对象
    QOpenGLBuffer *m_ibo;
    //
    QOpenGLFramebufferObject *m_renderFbo;

    //shader程序
    QOpenGLShaderProgram *m_program;
    QSize yuvSize[TEXTURE_NUMBER];
    qint64 yuvBufferSize[TEXTURE_NUMBER];
    GLuint textureId[TEXTURE_NUMBER];



    int matrix;
       int mVertexInLocaltion;
       int mTextureInLocaltion;
       int textureLocaltion[TEXTURE_NUMBER];
       int mAlpha;
       int mTextureFormat;
       int mTextureFormatValue;
       int mTextureOffset;
       int mImageWidthId;
       int mImageHeightId;
       int mEnableHDRId;
       int enableGaussianBlurId;


       int pboIndex;
       VideoFormat m_format;
       QMutex mDataMutex;


       QRect m_displayRect;

       QTime mTime;
       int mLastTime;
       int mFps;
       bool mIsInitPbo;
       bool mIsNeedNewUpdate;
       bool mIsInitTextures;

       bool mForceUpdate; //强制更新

       GLfloat mLastOffset; //上一次的偏移量
       RenderParams params;

};

#endif // QMPLAYER_H

#ifndef QGLWND_H
#define QGLWND_H



#include <QMatrix4x4>
#include <QMutex>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

#include <ffplay/Common.h>





class QGLWnd: public QOpenGLWidget, protected QOpenGLFunctions
{

    Q_OBJECT

public:
    QGLWnd(QWidget *parent = nullptr);
    virtual ~QGLWnd();


    void InitVideo(int width, int height);


    //opengl初始化
    void initializeGL() ;


    void initData();
    //绘制函数
    void paintGL() ;

    //Widget大小发生变化会调用次函数
    void resizeGL(int w, int h) ;



    //Model View Project 矩阵
    QMatrix4x4 mModelMatrix;
    QMatrix4x4 mViewMatrix;
    QMatrix4x4 mProjectionMatrix;

    //mvp矩阵句柄
    int mMMatrixHandle;
    int mVMatrixHandle;
    int mPMatrixHandle;

    //顶点坐标句柄
    int mVerticesHandle;

    //纹理坐标句柄
    int mTexCoordHandle;
    //shader程序
    QOpenGLShaderProgram *m_program;


    QVector<QVector3D> vertices;
    QVector<QVector2D> texcoords;




    //顶点缓冲区对象：存储顶点数据
    QOpenGLBuffer           *m_vbo = nullptr;

    //索引uancongqu对象：存储索引对象
    QOpenGLBuffer           *m_ibo = nullptr;

    QOpenGLVertexArrayObject *m_vao = nullptr;

    QOpenGLFramebufferObject *m_renderFbo;

    QOpenGLTexture *texture;

    //打印版本信息
    void printVersionInformation();
    void outputFunc(QOpenGLFunctions::OpenGLFeatures flag);
    void draw();
    void initTextures();

    void updateVideoFrame(VideoFormat *format);

    VideoFormat m_format;
    QMutex mux;
    //材质内存空间
    unsigned char *datas[3];
    GLuint texs[3];
    GLuint unis[3];


    int width = 960;
    int height = 540;
    bool isinit =false;
};

#endif // QGLWND_H

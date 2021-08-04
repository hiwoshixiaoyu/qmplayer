#include "qglwnd.h"

#include <QOpenGLTexture>


//顶点数组(物体表面坐标取值范围是-1到1,数组坐标：左下，右下，左上，右上)
//左上 -1.0f  -1.0f
//右上 1.0f  -1.0f
//左下 -1.0f  1.0f
//右下 1.0f  1.0f
static const GLfloat vertexVertices[] = {
   -1.0f, -1.0f, 0.0f,
   1.0f, -1.0f, 0.0f,
   1.0f,  1.0f, 0.0f,
   -1.0f,  1.0f, 0.0f
};






//像素，纹理数组(纹理坐标取值范围是0-1，坐标原点位于左下角,数组坐标：左上，右上，左下，右下,如果先左下，图像会倒过来)
static const GLfloat textureVertices[] = {
    0.0f,  0.0f,
    1.0f,  0.0f,
    1.0f,  1.0f,
    0.0f,  1.0f,
    0.0f,  0.0f,
    1.0f,  0.0f

};


QGLWnd::QGLWnd(QWidget *parent):QOpenGLWidget(parent)
{
    datas[0] =nullptr;
    datas[1] =nullptr;
    datas[2] =nullptr;
    unis[0]  =0;
    unis[1] =0;
    unis[2] =0;
    texs[0]=0;
    texs[1]=0;
    texs[2]=0;

}

QGLWnd::~QGLWnd()
{

}




void QGLWnd::updateVideoFrame(VideoFormat *format)
{

    if(format == nullptr)
    {
        return;
    }
    mux.lock();
    do
    {

        format->renderFrameMutex->lock();


        if(nullptr != datas[0])
        {
            delete datas[0];
            datas[0]=nullptr;
        }
        if(nullptr != datas[1] )
        {
            delete datas[1];
             datas[1]=nullptr;
        }
        if(nullptr != datas[2])
        {
            delete datas[2];
            datas[2]=nullptr;
        }


        datas[0] = new unsigned char[format->renderFrame->linesize[0]*height];		//Y
        datas[1] = new unsigned char[format->renderFrame->linesize[1]*height /2];	//U
        datas[2] = new unsigned char[format->renderFrame->linesize[2]*height /2];	//V

        for(int i = 0; i < height; i++) //Y
            memcpy(datas[0] + width*i, format->renderFrame->data[0] + format->renderFrame->linesize[0]*i, format->renderFrame->linesize[0]);
        for (int i = 0; i < height/2; i++) //U
            memcpy(datas[1] + width/2*i, format->renderFrame->data[1] + format->renderFrame->linesize[1] * i, format->renderFrame->linesize[1]);
        for (int i = 0; i < height/2; i++) //V
            memcpy(datas[2] + width/2*i, format->renderFrame->data[2] + format->renderFrame->linesize[2] * i, format->renderFrame->linesize[2]);

    }while(false);

    format->renderFrameMutex->unlock();
    mux.unlock();
    this->update();
}


void QGLWnd::initializeGL()
{
    initializeOpenGLFunctions();
    isinit =true;
    printVersionInformation();
    outputFunc(openGLFeatures());

    // Set global information
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//    glViewport(0,0,width(),height());

    // Create Shader (Do not release until VAO is created)
    initData();
   // initTextures();

}



void QGLWnd::initData()
{
    m_program = new QOpenGLShaderProgram();

    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/res/video.vert");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/res/planar.frag");
    m_program->link();
    m_program->bind();


    //设置为单位矩阵
    mModelMatrix.setToIdentity();
    mViewMatrix.setToIdentity();

   // mViewMatrix.translate(0,0,2);
    mProjectionMatrix.setToIdentity();


    mMMatrixHandle	= m_program->uniformLocation("Mmatrix");
    mVMatrixHandle	= m_program->uniformLocation("Vmatrix");
    mPMatrixHandle	= m_program->uniformLocation("Pmatrix");
    mVerticesHandle		= m_program->attributeLocation("vPosition");
    mTexCoordHandle		= m_program->attributeLocation("vTexture");
    unis[0] = m_program->uniformLocation("tex_y");
    unis[1] = m_program->uniformLocation("tex_u");
    unis[2] = m_program->uniformLocation("tex_v");

    if(!m_vbo){
        m_vbo = new QOpenGLBuffer;
        m_vbo->setUsagePattern( QOpenGLBuffer::StaticDraw );
        m_vbo->create( );
        m_vbo->bind( );
        m_vbo->allocate(vertexVertices,sizeof(vertexVertices));
        m_vbo->release();
    }


    if(!m_ibo){
        m_ibo = new QOpenGLBuffer;
        m_ibo->setUsagePattern( QOpenGLBuffer::StaticDraw );
        m_ibo->create( );
        m_ibo->bind( );
        m_ibo->allocate(textureVertices, sizeof( textureVertices ));
        m_ibo->release();
    }


    if(!m_vao){
        m_vao = new QOpenGLVertexArrayObject;
        m_vao->create();
        m_vao->bind();

        m_vbo->bind();
        m_program->enableAttributeArray(mVerticesHandle);
        m_program->setAttributeArray(mVerticesHandle, GL_FLOAT,0,3);
        m_vbo->release();

        m_ibo->bind();
        m_program->enableAttributeArray(mTexCoordHandle);
        m_program->setAttributeArray(mTexCoordHandle,GL_FLOAT,0,2);
        m_ibo->release();

        m_vao->release();

        m_program->disableAttributeArray(mVerticesHandle);
        m_program->disableAttributeArray(mTexCoordHandle);
      }


    //YUV纹理
    if (texs[0])
    {
        glDeleteTextures(3, texs);
    }
    glGenTextures(3, texs);

    //Y
    glBindTexture(GL_TEXTURE_2D, texs[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    //U
    glBindTexture(GL_TEXTURE_2D, texs[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    //V
    glBindTexture(GL_TEXTURE_2D, texs[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, 0);


//    m_renderFbo = new QOpenGLFramebufferObject(width,height,GL_TEXTURE_2D);
//    m_renderFbo->bindDefault();
}

void QGLWnd::paintGL()
{
    if(false == isinit)
    {
        return;
    }


    if(datas[0] == nullptr)
    {
        return;
    }

    mux.lock();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //m_renderFbo->bind();
    m_program->bind();
  m_vao->bind();

    draw();
   m_vao->release();
    m_program->release();

//   m_renderFbo->release();

    mux.unlock();

}


void QGLWnd::draw()
{

    //MVP矩阵
    m_program->setUniformValue(mMMatrixHandle, mModelMatrix);
    m_program->setUniformValue(mVMatrixHandle, mViewMatrix);
    m_program->setUniformValue(mPMatrixHandle, mProjectionMatrix);


    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texs[0]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, datas[0]);
    glUniform1i(unis[0], 0);


    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, texs[1]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_RED, GL_UNSIGNED_BYTE, datas[1]);
    glUniform1i(unis[1], 1);


    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, texs[2]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_RED, GL_UNSIGNED_BYTE, datas[2]);
    glUniform1i(unis[2], 2);

   //m_renderFbo->takeTexture();
    glDrawArrays(GL_TRIANGLE_FAN, 0,4);




    m_vao->release();
}


void QGLWnd::resizeGL(int w, int h)
{

}



void QGLWnd::printVersionInformation()
{
  QString glType;
  QString glVersion;
  QString glProfile;

  // Get Version Information
  glType = (context()->isOpenGLES()) ? "OpenGL ES" : "OpenGL";
  glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));

  // Get Profile Information
#define CASE(c) case QSurfaceFormat::c: glProfile = #c; break
  switch (format().profile())
  {
    CASE(NoProfile);
    CASE(CoreProfile);
    CASE(CompatibilityProfile);
  }
#undef CASE

  // qPrintable() will print our QString w/o quotes around it.
  qDebug() << qPrintable(glType) << qPrintable(glVersion) << "(" << qPrintable(glProfile) << ")";
}


void QGLWnd::outputFunc(QOpenGLFunctions::OpenGLFeatures flag)
{

   qDebug()<<"Qt包含opengl功能函数:"<<QString::number(flag,16);
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::Multitexture))
     qDebug() << "Multitexture";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::Shaders))
     qDebug() << "Shaders";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::Buffers))
     qDebug() << "Buffers";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::Framebuffers))
     qDebug() << "Framebuffers";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::BlendColor))
     qDebug() << "BlendColor";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::BlendEquation))
     qDebug() << "BlendEquation";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::BlendEquationSeparate))
     qDebug() << "BlendEquationSeparate";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::BlendSubtract))
     qDebug() << "BlendSubtract";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::CompressedTextures))
     qDebug() << "CompressedTextures";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::Multisample))
     qDebug() << "Multisample";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::StencilSeparate))
     qDebug() << "StencilSeparate";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::NPOTTextures))
     qDebug() << "NPOTTextures";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::NPOTTextureRepeat))
     qDebug() << "NPOTTextureRepeat";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::FixedFunctionPipeline))
     qDebug() << "FixedFunctionPipeline";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::TextureRGFormats))
     qDebug() << "TextureRGFormats";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::MultipleRenderTargets))
     qDebug() << "MultipleRenderTargets";
  if (flag.testFlag(QOpenGLFunctions::OpenGLFeature::BlendEquationAdvanced))
     qDebug() << "BlendEquationAdvanced";
  qDebug()<<"---------------------------";
}


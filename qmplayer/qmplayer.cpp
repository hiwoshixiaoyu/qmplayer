#include "qmplayer.h"
#include <QOpenGLFramebufferObjectFormat>

//顶点数组(物体表面坐标取值范围是-1到1,数组坐标：左下，右下，左上，右上)
//左上 -1.0f  -1.0f
//右上 1.0f  -1.0f
//左下 -1.0f  1.0f
//右下 1.0f  1.0f
static const GLfloat vertexVertices[] = {
   -1.0f, -1.0f,
   1.0f, -1.0f,
   -1.0f,  1.0f,
   1.0f,  1.0f,
};

//左上 0.0f  1.0f
//右上 1.0f  1.0f
//左下 0.0f  0.0f
//右下 1.0f  0.0f

//像素，纹理数组(纹理坐标取值范围是0-1，坐标原点位于左下角,数组坐标：左上，右上，左下，右下,如果先左下，图像会倒过来)
static const GLfloat textureVertices[] = {
    0.0f,  0.0f,
    1.0f,  0.0f,
    0.0f,  1.0f,
    1.0f,  1.0f,
};


qmplayer::qmplayer(QWidget *parent)
    : QOpenGLWidget(parent)
    ,m_program(nullptr)
    ,m_vbo(nullptr)
    ,m_ibo(nullptr)
    ,m_vao(nullptr)

{
   this->m_format.renderFrameMutex = new QMutex;
}
void qmplayer::initializeGL()
{
      initializeOpenGLFunctions();
}

void qmplayer::setVideoFormat(VideoFormat f)
{
    m_format.renderFrameMutex->lock();
    m_format.renderFrame = f.renderFrame;
    m_format.width = f.width;
    m_format.height =f.height;
    m_format.renderFrameMutex->unlock();

    this->update();
}

void qmplayer::resizeGL(int w, int h)
{

}

void qmplayer::paintGL()
{
    if(m_format.width <= 0 || m_format.height <= 0)
            return;
    this->init();
        //计算真实的渲染FPS
        if(!mTime.isValid())
            mTime.start();

        int elapsed = mTime.elapsed();
        if(elapsed - mLastTime >= 1000){ //1秒钟
            mLastTime = 0;
            mTime.restart();
           // m_output->setReallyFps(mFps);
            mFps = 0;
        }
        //end 计算真实的渲染FPS

//        if(!mForceUpdate){
//            return;
//        }

        ++mFps;

        mDataMutex.lock();
        m_format.renderFrameMutex->lock();

        if(m_format.renderFrame != NULL && m_format.renderFrame->hw_frames_ctx == NULL){ // hw_frames_ctx不为空则为硬解
            if(m_format.renderFrame == NULL ||
                    m_format.renderFrame->data[0] == NULL ||
                    m_format.renderFrame->width <= 0 ||
                    m_format.renderFrame->height <= 0){
                m_format.renderFrameMutex->unlock();
                mDataMutex.unlock();
                return;
            }
        }

        if(mIsNeedNewUpdate){
            if(m_program != NULL){
                m_program->deleteLater();
                m_program = NULL;
            }

            if(m_vbo != NULL){
                delete m_vbo;
                m_vbo = NULL;
            }

            if(m_ibo != NULL){
                delete m_ibo;
                m_ibo = NULL;
            }

            if(m_vao != NULL){
                m_vao->deleteLater();
                m_vao = NULL;
            }

            if(mIsInitTextures)
                glDeleteTextures(TEXTURE_NUMBER, textureId);
            mIsInitTextures = false;

            init();

            mIsNeedNewUpdate = false;
        }

    //    static int index = 0;
    //    ++index;
//        QOpenGLFramebufferObject *displayFbo = framebufferObject();

//        displayFbo->bind();
        m_program->bind();

        pboIndex = 0;

        GLfloat offset = 0;
        for(int j = 0;j < TEXTURE_NUMBER;j++){
            m_pbo[pboIndex][j].bind();
            glActiveTexture(GL_TEXTURE0 + j);
    //        glPixelStorei(GL_UNPACK_SWAP_BYTES,GL_TRUE);
            glBindTexture(GL_TEXTURE_2D, textureId[j]);

            int linesize = m_format.renderFrame->linesize[j];
            uint8_t * data = m_format.renderFrame->data[j];

            if(m_format.renderFrame->hw_frames_ctx != NULL){
                linesize = m_format.renderFrame->hw_frames_ctx->size;
                data = m_format.renderFrame->hw_frames_ctx->data;
                continue;
    //            qDebug() << linesize;
            }
    //        if(index == 1)
    //            qDebug() << "----------" << linesize << m_format.renderFrame->width << m_format.renderFrame->height << j;
            if(data != NULL && linesize != 0){
                qint64 textureSize = qAbs(linesize)*m_format.renderFrame->height;

                textureSize = textureSize * params.yuvsizes[j].num / params.yuvsizes[j].den;

                if(m_pbo[pboIndex][j].size() != textureSize)
                    m_pbo[pboIndex][j].allocate(textureSize);

                if(linesize < 0){
                    for(int i = 0;i < m_format.renderFrame->height;i++ ){
                        m_pbo[pboIndex][j].write(i * qAbs(linesize),data + i * linesize,qAbs(linesize));
                    }
                }else{
                    if(m_format.renderFrame->hw_frames_ctx != NULL){
                        void *p = m_pbo[pboIndex][j].map(QOpenGLBuffer::WriteOnly);
                        if(p){
                             memcpy(p, data , textureSize);
                             m_pbo[pboIndex][j].unmap();
                        }
                    }else{
                        m_pbo[pboIndex][j].write(0,data , textureSize);
                    }
                }
                linesize = qAbs(linesize);

                int width = linesize * params.yuvwidths[j].num / params.yuvwidths[j].den;
                int height = m_format.renderFrame->height * params.yuvheights[j].num / params.yuvheights[j].den;

                if(j == 0){// Y or R
                    offset = (GLfloat)((linesize % m_format.renderFrame->width) * 1.0 / m_format.renderFrame->width);
                    m_program->setUniformValue(mTextureOffset, offset); //偏移量

                    if(mLastOffset != offset){
                        GLfloat realTimeTextureVertices[] = {
                            0.0f,  0.0f,
                            1.0f - offset,  0.0f,
                            0.0f,  1.0f,
                            1.0f - offset,  1.0f,
                        };
                        m_ibo->bind( );
                        m_ibo->allocate( realTimeTextureVertices, sizeof( realTimeTextureVertices ));
                        m_ibo->release();
                    }
                    mLastOffset = offset;
                }
    //            qDebug() << width << height;
    //            glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
                glTexSubImage2D(GL_TEXTURE_2D,0,0,0,width,height, params.yuvGlFormat[j], params.dataType, NULL);
    //            glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
            }
            m_program->setUniformValue(textureLocaltion[j], j);
            m_pbo[pboIndex][j].release();
        }

        m_format.renderFrameMutex->unlock();
        mDataMutex.unlock();

//        int rotate = m_format.rotate;
//        switch (m_output->orientation()) {
//        case AVDefine::AVOrientation_LandscapeOrientation:rotate += 90;break;
//        case AVDefine::AVOrientation_InvertedLandscapeOrientation:rotate += 270;break;
//        case AVDefine::AVOrientation_InvertedPortraitOrientation:rotate += 180;break;
//        }
        //rotate %= 360;

        QMatrix4x4 modelview;
        //modelview.rotate(rotate, 0.0f, 0.0f, 1.0f);
        m_program->setUniformValue(matrix,modelview);
        m_program->setUniformValue(mTextureFormat, (GLfloat)params.textureFormat); //纹理格式(YUV,YUVJ,RGB)
        m_program->setUniformValue(mAlpha, (GLfloat)1.0); //透明度
       // m_program->setUniformValue(mEnableHDRId, m_output->HDR()); //HDR


//        m_displayRect = m_output->calculateGeometry(rotate == 90 || rotate == 270 ?
//                                                    m_format.height : m_format.width,
//                                                    rotate == 90 || rotate == 270 ?
//                                                    m_format.width : m_format.height);

        m_program->setUniformValue(mImageWidthId, m_format.width); //图像宽度
        m_program->setUniformValue(mImageHeightId, m_format.height); //图像高度

        QColor color;// = m_output->backgroundColor();

        glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_MULTISAMPLE);
        m_vao->bind();

//        if(m_output->useVideoBackground()){
//            m_program->setUniformValue(enableGaussianBlurId, true);
//            glViewport(0,0,m_output->width(),m_output->height());
//            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//            m_program->setUniformValue(enableGaussianBlurId, false);
//        }

    //    if(m_output->VR()){
    //        glViewport(m_displayRect.x(),m_displayRect.y(),m_displayRect.width() / 2,m_displayRect.height());
    //        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    //        glViewport(m_displayRect.width() / 2,m_displayRect.y(),m_displayRect.width() / 2,m_displayRect.height());
    //        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //    }else{
            glViewport(m_displayRect.x(),m_displayRect.y(),m_displayRect.width(),m_displayRect.height());
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //    }
        m_vao->release();
        m_program->release();
        //displayFbo->bindDefault();

       // m_output->window()->resetOpenGLState(); //重置opengl状态，不然界面上的文字会出现花屏

    //    QOpenGLFramebufferObject *displayFbo = framebufferObject();
    //    qSwap(m_renderFbo,displayFbo);

        mForceUpdate = false;
}

void qmplayer::init()
{

    if (!m_program)
    {
        m_program = new QOpenGLShaderProgram;


        m_program->addShaderFromSourceFile(QOpenGLShader::Vertex,":/res/video.vert");
        m_program->addShaderFromSourceFile(QOpenGLShader::Fragment,false ? ":/res/planar.frag" : ":/res/packed.frag");

        m_program->link();


       mVertexInLocaltion = m_program->attributeLocation("vertexIn");
       mTextureInLocaltion = m_program->attributeLocation("textureIn");

       matrix = m_program->uniformLocation("matrix");
       textureLocaltion[0] = m_program->uniformLocation("tex_y");
       textureLocaltion[1] = m_program->uniformLocation("tex_u");
       textureLocaltion[2] = m_program->uniformLocation("tex_v");

       mAlpha = m_program->uniformLocation("alpha");
       mTextureFormat = m_program->uniformLocation("tex_format");
       mTextureOffset = m_program->uniformLocation("tex_offset");
       mImageWidthId = m_program->uniformLocation("imageWidth");
       mImageHeightId = m_program->uniformLocation("imageHeight");
       mEnableHDRId = m_program->uniformLocation("enableHDR");
       enableGaussianBlurId = m_program->uniformLocation("enableGaussianBlur");
    }


    if(!mIsInitTextures){
            //初使化纹理
            glGenTextures(TEXTURE_NUMBER, textureId);
            for(int i = 0; i < TEXTURE_NUMBER; i++)
            {
                glBindTexture(GL_TEXTURE_2D, textureId[i]);
                int linesize = qAbs(m_format.renderFrame->linesize[i]);
                AVRational widthRational = params.yuvwidths[i];
                AVRational heightRational = params.yuvheights[i];
                int width = linesize * widthRational.num / widthRational.den;
                int height = m_format.renderFrame->height * heightRational.num / heightRational.den;
                //qDebug() << width << height;
                glTexImage2D ( GL_TEXTURE_2D, 0, params.yuvInternalformat[i],width ,height, 0, params.yuvGlFormat[i], params.dataType, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    //            glTexParameteri(GL_TEXTURE_2D, GL_UNPACK_SWAP_BYTES, GL_TRUE);
    //            glPixelStorei(GL_UNPACK_SWAP_BYTES,GL_TRUE );
            }
        }
        mIsInitTextures = true;

    //    GL_UNPACK_SWAP_BYTES
        if(!mIsInitPbo){
            for(int i = 0;i < 1;i++){
                for(int j = 0;j < TEXTURE_NUMBER;j++){
                    m_pbo[i][j] = QOpenGLBuffer(QOpenGLBuffer::PixelUnpackBuffer);
                    m_pbo[i][j].setUsagePattern(QOpenGLBuffer::StreamDraw);
                    m_pbo[i][j].create();
                }
            }
            mIsInitPbo = true;
        }

        if(!m_vbo){
            m_vbo = new QOpenGLBuffer;
            m_vbo->setUsagePattern( QOpenGLBuffer::StaticDraw );
            m_vbo->create( );
            m_vbo->bind( );
            m_vbo->allocate( vertexVertices, sizeof( vertexVertices ));
            m_vbo->release();
        }

        if(!m_ibo){
            m_ibo = new QOpenGLBuffer;
            m_ibo->setUsagePattern( QOpenGLBuffer::StaticDraw );
            m_ibo->create( );
            m_ibo->bind( );
            m_ibo->allocate( textureVertices, sizeof( textureVertices ));
            m_ibo->release();
        }

        if(!m_vao){
            m_vao = new QOpenGLVertexArrayObject;
            m_vao->create();
            m_vao->bind();

            m_vbo->bind();
            m_program->setAttributeBuffer( mVertexInLocaltion, GL_FLOAT, 0, 2 );
            m_program->enableAttributeArray(mVertexInLocaltion);
            m_vbo->release();

            m_ibo->bind();
            m_program->setAttributeBuffer( mTextureInLocaltion, GL_FLOAT, 0, 2 );
            m_program->enableAttributeArray(mTextureInLocaltion);
            m_ibo->release();

            m_vao->release();
        }
}


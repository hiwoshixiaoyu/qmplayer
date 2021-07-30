#include "qmplay.h"
#include "ui_qmplay.h"
static int display_disable;
double rdftspeed = 0.02;
static int64_t audio_callback_time;


QmPlay::QmPlay(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::QmPlay)
{
    ui->setupUi(this);


    connect(&m_timer,&QTimer::timeout,this,&QmPlay::RefreshVideo);
    auto *player =   ui->widget;

}

QmPlay::~QmPlay()
{
    delete ui;
}


//void QmPlay::on_btntester_clicked()
//{
//     t.init();

//}


void QmPlay::RefreshVideo()
{
     auto *player =   ui->widget;
     auto *video = m_read->getState();
     if(video != nullptr && video->video_st != nullptr)
     {
        Frame *vp = video->pictq.frame_queue_peek_last();

        VideoFormat f;
        f.renderFrameMutex = new std::mutex;
        f.width = vp->frame->width;
        f.height = vp->frame->height;
        f.renderFrame = vp->frame;
        player->updateVideoFrame(&f);


     }
}


void QmPlay::on_btnplay_clicked()
{

     auto *player =   ui->widget;
    //player->Init(960,540);
    m_read = new ReadThread;
    m_read->start();
    m_timer.start(100);

}

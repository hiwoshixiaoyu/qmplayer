#include "qmplay.h"
#include "ui_qmplay.h"

#include <ffplay/readthread.h>

#include <QThread>
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

    m_read = new ReadThread;
    m_read->start();


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
        if(nullptr != vp)
        {
            VideoFormat f;
            f.renderFrameMutex = new std::mutex;
            f.width = vp->frame->width;
            f.height = vp->frame->height;
            f.renderFrame = vp->frame;
            player->updateVideoFrame(&f);
        }
        video->pictq.frame_queue_next();
     }
}


void QmPlay::on_btnplay_clicked()
{

     auto *player =   ui->widget;
    //player->Init(960,540);
    m_timer.start(100);
    Task task;
    task.type = Task_File;
    task.filePath =  "E://a.mp4";;
    m_read->SendMsg(task);

    task.type =Task_Play;
    m_read->SendMsg(task);




}

void QmPlay::on_btnstop_clicked()
{
    m_timer.stop();

//    ui->widget->setRender(false);
    Task task;
    task.type = Task_Close;
    m_read->SendMsg(task);
}

void QmPlay::on_btnrender_clicked()
{
    // ui->widget->setRender(true);
}

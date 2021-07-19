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
}

QmPlay::~QmPlay()
{
    delete ui;
}


void QmPlay::on_btntester_clicked()
{
     t.init();
}

void QmPlay::on_btnread_clicked()
{

    m_read = new ReadThread;
    m_read->start();

    m_timer.start(10);
    connect(&m_timer,&QTimer::timeout,this,&QmPlay::RefreshVideo);

}

static inline int compute_mod(int a, int b)
{
    return a < 0 ? a%b + b : a%b;
}

unsigned char* y=nullptr;
unsigned char* u=nullptr;
unsigned char* v=nullptr;
void getYUV420P(AVFrame *aVFrame)
{
    int w = aVFrame->width;
    int h = aVFrame->height;
}
static void video_audio_display(VideoState *is)
{
    Frame *vp;
    Frame *sp = NULL;


    vp =is->pictq.frame_queue_peek_last();

    if (!vp->uploaded) {
           if (upload_texture(&is->vid_texture, vp->frame, &is->img_convert_ctx) < 0)
               return;
           vp->uploaded = 1;
           vp->flip_v = vp->frame->linesize[0] < 0;
    }



}
static void video_display(VideoState *is)
{
/*    if (!is->width)
        video_open(is)*/;

//    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
//    SDL_RenderClear(renderer);
    if (is->audio_st && is->show_mode != VideoState::SHOW_MODE_VIDEO)
        video_audio_display(is);
    else if (is->video_st)
        video_image_display(is);
//    SDL_RenderPresent(renderer);
}
void QmPlay::RefreshVideo()
{
     double remaining_time = 0.0;
    double time;
    Frame *sp, *sp2;
    VideoState *is = m_read->getState();

    if (!is->paused && is->get_master_sync_type() == AV_SYNC_EXTERNAL_CLOCK && is->realtime)
    {
        is->check_external_clock_speed();
    }

    if (!display_disable && is->show_mode != VideoState::SHOW_MODE_VIDEO && is->audio_st) {
            time = av_gettime_relative() / 1000000.0;
            if (is->force_refresh || is->last_vis_time + rdftspeed < time) {
                video_display(is);
                is->last_vis_time = time;
            }
            *remaining_time = FFMIN(*remaining_time, is->last_vis_time + rdftspeed - time);
    }


}

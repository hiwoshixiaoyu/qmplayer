#include "VideoState.h"
static int fast = 0;
static int lowres = 0;
#define AUDIO_DIFF_AVG_NB   20
static int framedrop = -1;

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10


#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001
void VideoState::destroy()
{
}
void VideoState::initDecoder()
{

//    viddec.decoder_init();
//    auddec;
//    subdec;

}

void VideoState::init()
{
    do
    {
        abort_request = 0;
        paused = 0;
        last_paused = 0;
        seek_req = 0;
        queue_attachments_req = 0;

        last_video_stream = video_stream = -1;
        last_audio_stream = audio_stream = -1;
        last_subtitle_stream = subtitle_stream = -1;

        //todo iformat

        if (pictq.frame_queue_init(&videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0)
        {
            break;
        }
        if (subpq.frame_queue_init(&subtitleq, SUBPICTURE_QUEUE_SIZE, 0) < 0)
        {
            break;
        }
        if (sampq.frame_queue_init(&audioq, SAMPLE_QUEUE_SIZE, 1) < 0)
        {
            break;
        }


        vidclk.init_clock(&videoq.serial);
        audclk.init_clock(&audioq.serial);
        extclk.init_clock(&extclk.serial);
        audio_clock_serial = -1;


        if (startup_volume < 0)
        {
            av_log(NULL, AV_LOG_WARNING, "-volume=%d < 0, setting to 0\n", startup_volume);
        }
       if (startup_volume > 100)
       {
           av_log(NULL, AV_LOG_WARNING, "-volume=%d > 100, setting to 100\n", startup_volume);
       }
       startup_volume = av_clip(startup_volume, 0, 100);
       audio_volume = startup_volume;
       muted = 0;
       this->av_sync_type = av_sync_type;
    }while(false);


}

bool VideoState::tickTask(AVPacket *pkt)
{
    bool ret = true;
    do
    {

        dealPaused();
        dealSeek_req();
        ret = dealqueue_attachments_req(pkt);
        if(ret)
        {
            break;
        }

        if(dealinfinite_buffer())
        {
            break;
        }

        ret = dealReadFrame(pkt);
        if(ret)
        {
            break;
        }

    }while(false);
    return ret;
}

int VideoState::stream_component_open(int stream_index)
{
    AVCodecContext *avctx;
    const AVCodec *codec;
    int sample_rate;
    int nb_channels;
    int64_t channel_layout;

    int stream_lowres = lowres;
    const char *forced_codec_name = nullptr;
    int ret = 0;
    bool isNeedFreeContext = false;
    bool isFailInit = false;
    do
    {
        if (stream_index < 0 || stream_index >= ic->nb_streams)
         {
            break;
         }

        avctx = avcodec_alloc_context3(nullptr);
        if (!avctx)
        {
            break;
        }
        ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
        if (ret < 0)
        {
            isNeedFreeContext = true;
            break;
        }

        avctx->pkt_timebase = ic->streams[stream_index]->time_base;
        codec = avcodec_find_decoder(avctx->codec_id);
//        codec = avcodec_find_decoder_by_name(forced_codec_name);

        if (nullptr == codec)
        {
            if (forced_codec_name)
            {
                av_log(NULL, AV_LOG_WARNING,"No codec could be found with name '%s'\n", forced_codec_name);
            }
            else
            {
                av_log(NULL, AV_LOG_WARNING,"No decoder could be found for codec %s\n", avcodec_get_name(avctx->codec_id));
            }
           avcodec_free_context(&avctx);
           break;
        }


        avctx->codec_id = codec->id;
        if (stream_lowres > codec->max_lowres)
        {
            av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",codec->max_lowres);
            stream_lowres = codec->max_lowres;
        }

        avctx->lowres = stream_lowres;

        if (fast)
        {
            avctx->flags2 |= AV_CODEC_FLAG2_FAST;
        }

        if ((ret = avcodec_open2(avctx, codec, nullptr)) < 0)
        {
            break;
        }

        eof = 0;
        ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;

        switch (avctx->codec_type) {
         case AVMEDIA_TYPE_AUDIO:

             sample_rate    = avctx->sample_rate;
             nb_channels    = avctx->channels;
             channel_layout = avctx->channel_layout;


             /* prepare audio output todo*/

             audio_hw_buf_size = ret;
             audio_src = audio_tgt;
             audio_buf_size  = 0;
             audio_buf_index = 0;

             /* init averaging filter */
             audio_diff_avg_coef  = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
             audio_diff_avg_count = 0;
             /* since we do not have a precise anough audio FIFO fullness,
                we correct audio sync only if larger than this threshold */
             audio_diff_threshold = (double)(audio_hw_buf_size) / audio_tgt.bytes_per_sec;

             audio_stream = stream_index;
             audio_st = ic->streams[stream_index];

             if ((ret = auddec.decoder_init( avctx, &audioq, &continue_read_thread)) < 0)
             {
                isFailInit = true;
                break;
             }

             if ((ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) && !ic->iformat->read_seek)
             {
                 auddec.start_pts = audio_st->start_time;
                 auddec.start_pts_tb = audio_st->time_base;
             }
             audioq.packet_queue_start();
             //todo  SDL_PauseAudioDevice(audio_dev, 0);
             break;
         case AVMEDIA_TYPE_VIDEO:
             video_stream = stream_index;
             video_st = ic->streams[stream_index];

             if ((ret = viddec.decoder_init(avctx, &videoq, &continue_read_thread)) < 0)
              {
                 isFailInit = true;
                 break;
             }

             queue_attachments_req = 1;
             videoq.packet_queue_start();
             break;
         case AVMEDIA_TYPE_SUBTITLE:
             subtitle_stream = stream_index;
             subtitle_st = ic->streams[stream_index];

             if ((ret = subdec.decoder_init(avctx, &subtitleq, &continue_read_thread)) < 0)
             {
                 isFailInit = true;
                 break;
             }
             subtitleq.packet_queue_start();
             break;
         default:
             break;
         }

    }while(false);

    if(isFailInit)
    {
        avcodec_free_context(&avctx);
    }


}

bool VideoState::dealPaused()
{
    if (paused != last_paused)
    {
      last_paused =paused;
       if (paused)
       {
           read_pause_return = av_read_pause(ic);
       }
       else
       {
          av_read_play(ic);
       }
    }
}


void VideoState::dealSeek_req()
{
    if (seek_req)
    {
         //seek请求
         int64_t seek_target = seek_pos;
         int64_t seek_min    = seek_rel > 0 ? seek_target - seek_rel + 2: INT64_MIN;
         int64_t seek_max    = seek_rel < 0 ? seek_target - seek_rel - 2: INT64_MAX;
         // FIXME the +-2 is due to rounding being not done in the correct direction in generation
         //      of the seek_pos/seek_rel variables

         int ret = avformat_seek_file(ic, -1, seek_min, seek_target, seek_max, seek_flags);
         if (ret < 0)
         {
               av_log(NULL, AV_LOG_ERROR,"%s: error while seeking\n", ic->url);
         }
         else
         {
               if(audio_stream >= 0)
               {
                   audioq.packet_queue_flush();
               }
               if(subtitle_stream >= 0)
               {
                   subtitleq.packet_queue_flush();
               }
               if(video_stream >= 0)
               {
                   videoq.packet_queue_flush();
               }
               if(seek_flags & AVSEEK_FLAG_BYTE)
               {
                   extclk.set_clock(NAN, 0);
               }
               else
               {
                  extclk.set_clock(seek_target / (double)AV_TIME_BASE, 0);
               }
           }
           seek_req = 0;
           queue_attachments_req = 1;
           eof = 0;
           if (paused)
              step_to_next_frame();
    }
}


int VideoState::dealqueue_attachments_req(AVPacket *pkt)
{
    int ret = 0;
    do
    {
        if (!queue_attachments_req)
        {
            break;
        }

        if (video_st && video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)
        {
            if (av_packet_ref(pkt, &video_st->attached_pic)< 0)
               {
                ret =1;
                break;
            }
            videoq.packet_queue_put(pkt);
            videoq.packet_queue_put_nullpacket(pkt, video_stream);
        }

        queue_attachments_req = 0;

    }while(false);
    return  ret;
}

int VideoState::dealReadFrame(AVPacket *pkt)
{
    int ret = av_read_frame(ic, pkt);
    bool isContinue = false;
    do
    {


        if (ret < 0)
        {
            if (ret == AVERROR_EOF || avio_feof(ic->pb) )
            {
               pushNullPack(pkt);
            }

            if (ic->pb && ic->pb->error)
            {
                ret = 1;
                break;
            }
            isContinue =true;
             wait();
             break;
          }
          else
          {
              eof = 0;
          }

        /* check if packet is in play range specified by user, then queue, otherwise discard */
        int64_t stream_start_time = ic->streams[pkt->stream_index]->start_time;
        int64_t pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
        int pkt_in_play_range = duration == AV_NOPTS_VALUE ||
              (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
              av_q2d(ic->streams[pkt->stream_index]->time_base) -
              (double)(start_time != AV_NOPTS_VALUE ? start_time : 0) / 1000000
              <= ((double)duration / 1000000);


        if(pkt_in_play_range)
        {
            packet_queue_put(pkt);
        }

    }while(false);

    return ret;
}


static int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue) {
    return stream_id < 0 ||
           queue->abort_request ||
           (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
           queue->nb_packets > MIN_FRAMES && (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);
}

bool VideoState::dealinfinite_buffer()
{
    if (infinite_buffer<1 &&
                 (audioq.size + videoq.size + subtitleq.size > MAX_QUEUE_SIZE
               || (stream_has_enough_packets(audio_st, audio_stream, &audioq) &&
                   stream_has_enough_packets(video_st, video_stream, &videoq) &&
                   stream_has_enough_packets(subtitle_st, subtitle_stream, &subtitleq)))) {
               /* wait 10 ms */
              wait();
   }
}

/* pause or resume the video */
 void VideoState::stream_toggle_pause()
{
    if (paused) {
        frame_timer += av_gettime_relative() / 1000000.0 - vidclk.last_updated;
        if (read_pause_return != AVERROR(ENOSYS)) {
            vidclk.paused = 0;
        }
        vidclk.set_clock(vidclk.get_clock(), vidclk.serial);
    }
    extclk.set_clock(extclk.get_clock(), extclk.serial);
    paused = audclk.paused = vidclk.paused = extclk.paused = !paused;
 }

 double VideoState::get_master_clock()
 {
     double val;
     switch (get_master_sync_type()) {
         case AV_SYNC_VIDEO_MASTER:
             val = vidclk.get_clock();
             break;
         case AV_SYNC_AUDIO_MASTER:
             val = audclk.get_clock();
             break;
         default:
             val = extclk.get_clock();
             break;
     }
     return val;
 }

 int VideoState::get_master_sync_type()
 {

     if (av_sync_type == AV_SYNC_VIDEO_MASTER)
     {
         if (video_st)
             return AV_SYNC_VIDEO_MASTER;
         else
             return AV_SYNC_AUDIO_MASTER;
     }
     else if (av_sync_type == AV_SYNC_AUDIO_MASTER)
     {
         if (audio_st)
             return AV_SYNC_AUDIO_MASTER;
         else
             return AV_SYNC_EXTERNAL_CLOCK;
     } else {
         return AV_SYNC_EXTERNAL_CLOCK;
     }

 }

void VideoState::step_to_next_frame()
{
    if (paused)
        stream_toggle_pause();
      step = 1;
}

void VideoState::pushNullPack(AVPacket *pkt)
{
    if(eof)
    {
        return;
    }
    if (video_stream >= 0)
    {
       videoq.packet_queue_put_nullpacket(pkt, video_stream);
    }
    if (audio_stream >= 0)
    {
        audioq.packet_queue_put_nullpacket(pkt, audio_stream);
    }
    if (subtitle_stream >= 0)
    {
        subtitleq.packet_queue_put_nullpacket(pkt, subtitle_stream);
    }
    eof = 1;
}

void VideoState::wait()
{
    std::unique_lock<std::mutex> tmutex(wait_mutex);
    continue_read_thread.wait_for(tmutex,std::chrono::milliseconds(10));

}

void VideoState::packet_queue_put(AVPacket *pkt)
{

    if (pkt->stream_index ==  audio_stream )
    {
        audioq.packet_queue_put( pkt);
    }
    else if (pkt->stream_index == video_stream
               && !(video_st->disposition & AV_DISPOSITION_ATTACHED_PIC))
    {
        videoq.packet_queue_put( pkt);
    } else if (pkt->stream_index == subtitle_stream )
    {
        subtitleq.packet_queue_put(pkt);
    } else {
        av_packet_unref(pkt);
    }
}



int VideoState::get_video_frame(AVFrame *frame)
{
    int got_picture;
    if ((got_picture = viddec.decoder_decode_frame(frame, NULL)) < 0)
        return -1;

    if (got_picture) {
        double dpts = NAN;

        if (frame->pts != AV_NOPTS_VALUE)
            dpts = av_q2d(video_st->time_base) * frame->pts;

        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(ic, video_st, frame);

        if (framedrop>0 || (framedrop && get_master_sync_type() != AV_SYNC_VIDEO_MASTER)) {
            if (frame->pts != AV_NOPTS_VALUE) {
                double diff = dpts - get_master_clock();
                if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
                    diff - frame_last_filter_delay < 0 &&
                    viddec.pkt_serial == vidclk.serial &&
                    videoq.nb_packets) {
                    frame_drops_early++;
                    av_frame_unref(frame);
                    got_picture = 0;
                }
            }
        }
    }

    return got_picture;
}

void VideoState::check_external_clock_speed()
{
    if (video_stream >= 0 && videoq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES ||
           audio_stream >= 0 && audioq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES) {
           extclk.set_clock_speed(FFMAX(EXTERNAL_CLOCK_SPEED_MIN, extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
       } else if ((video_stream < 0 || videoq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES) &&
                  (audio_stream < 0 || audioq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES)) {
          extclk.set_clock_speed( FFMIN(EXTERNAL_CLOCK_SPEED_MAX, extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
       } else {
           double speed = extclk.speed;
           if (speed != 1.0)
               extclk.set_clock_speed(speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
       }
}



int VideoState::queue_picture( AVFrame *src_frame, double pts, double duration, int64_t pos, int serial)
{
    Frame *vp;

    if (!(vp = pictq.frame_queue_peek_writable()))
        return -1;

    vp->sar = src_frame->sample_aspect_ratio;
    vp->uploaded = 0;

    vp->width = src_frame->width;
    vp->height = src_frame->height;
    vp->format = src_frame->format;

    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;

    //todo
    //set_default_window_size(vp->width, vp->height, vp->sar);

    av_frame_move_ref(vp->frame, src_frame);
    pictq.frame_queue_push();
    return 0;
}

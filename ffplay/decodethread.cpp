#include "DecodeThread.h"


DecodeThread::DecodeThread(VideoState *is)
{
    this->is = is;

}

int DecodeThread::RunAudio()
{
       return 1;
       AVFrame *frame = av_frame_alloc();
       Frame *af;

       int got_frame = 0;
       AVRational tb;
       int ret = 0;

       if (!frame)
           return AVERROR(ENOMEM);

       do {
           if ((got_frame = is->auddec.decoder_decode_frame( frame, NULL)) < 0)
               goto the_end;

           if (got_frame) {
                   tb = (AVRational){1, frame->sample_rate};


                   if (!(af = is->sampq.frame_queue_peek_writable()))
                       goto the_end;

                   af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
                   af->pos = frame->pkt_pos;
                   af->serial = is->auddec.pkt_serial;
                   af->duration = av_q2d((AVRational){frame->nb_samples, frame->sample_rate});

                   av_frame_move_ref(af->frame, frame);
                   is->sampq.frame_queue_push();


           }
       } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
    the_end:
       av_frame_free(&frame);
       return ret;
}






int DecodeThread::RunVideo()
{

      AVFrame *frame = av_frame_alloc();
      double pts;
      double duration;
      int ret;
      AVRational tb = is->video_st->time_base;
      AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);

      if (!frame)
          return AVERROR(ENOMEM);

      for (;;) {
          ret = is->get_video_frame( frame);
          if (ret < 0)
              goto the_end;
          if (!ret)
              continue;

              duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
              pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
              ret = is->queue_picture(frame, pts, duration, frame->pkt_pos, is->viddec.pkt_serial);
              av_frame_unref(frame);


          if (ret < 0)
              goto the_end;
      }
   the_end:
      av_frame_free(&frame);
      return 0;
}

int DecodeThread::RunSubtitle()
{

      Frame *sp;
      int got_subtitle;
      double pts;
      for (;;) {
          if (!(sp = is->subpq.frame_queue_peek_writable()))
              return 0;

          if ((got_subtitle = is->subdec.decoder_decode_frame(NULL, &sp->sub)) < 0)
              break;

          pts = 0;
          if (got_subtitle && sp->sub.format == 0) {
              if (sp->sub.pts != AV_NOPTS_VALUE)
                  pts = sp->sub.pts / (double)AV_TIME_BASE;
              sp->pts = pts;
              sp->serial = is->subdec.pkt_serial;
              sp->width = is->subdec.avctx->width;
              sp->height = is->subdec.avctx->height;
              sp->uploaded = 0;

              /* now we can update the picture count */
              is->subpq.frame_queue_push();
          } else if (got_subtitle) {
              avsubtitle_free(&sp->sub);
          }
      }
}


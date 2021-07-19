#include "decoder.h"


static int decoder_reorder_pts = -1;
int Decoder::decoder_init(AVCodecContext *avctx, PacketQueue *queue, std::condition_variable *empty_queue_cond)
{



    this->pkt =nullptr;
    this->queue = nullptr;
    this->avctx = nullptr;
    this->pkt_serial = 0;
    this->finished = 0;
    this->packet_pending = 0;
    this->empty_queue_cond=nullptr;

    this->start_pts = 0;
    this->start_pts_tb.den =0 ;
    this->start_pts_tb.num = 0;
    this->next_pts= 0;
    this->next_pts_tb.den= 0;
    this->next_pts_tb.num = 0;


    this->pkt = av_packet_alloc();
    if (!this->pkt)
      return AVERROR(ENOMEM);
    this->avctx = avctx;
    this->queue = queue;
    this->empty_queue_cond = empty_queue_cond;
    this->start_pts = AV_NOPTS_VALUE;
    this->pkt_serial = -1;
    return 0;
}

int Decoder::decoder_decode_frame(AVFrame *frame, AVSubtitle *sub)
{
    int ret = AVERROR(EAGAIN);

      for (;;) {
          if (queue->serial == pkt_serial) {
              do {
                  if (queue->abort_request)
                      return -1;

                  switch (avctx->codec_type) {
                      case AVMEDIA_TYPE_VIDEO:
                          ret = avcodec_receive_frame(avctx, frame);
                          if (ret >= 0) {
                              if (decoder_reorder_pts == -1) {
                                  frame->pts = frame->best_effort_timestamp;
                              } else if (!decoder_reorder_pts) {
                                  frame->pts = frame->pkt_dts;
                              }
                          }
                          break;
                      case AVMEDIA_TYPE_AUDIO:
                          ret = avcodec_receive_frame(avctx, frame);
                          if (ret >= 0) {
                              AVRational tb = (AVRational){1, frame->sample_rate};
                              if (frame->pts != AV_NOPTS_VALUE)
                                  frame->pts = av_rescale_q(frame->pts, avctx->pkt_timebase, tb);
                              else if (next_pts != AV_NOPTS_VALUE)
                                  frame->pts = av_rescale_q(next_pts, next_pts_tb, tb);
                              if (frame->pts != AV_NOPTS_VALUE) {
                                  next_pts = frame->pts + frame->nb_samples;
                                  next_pts_tb = tb;
                              }
                          }
                          break;
                  }
                  if (ret == AVERROR_EOF) {
                      finished = pkt_serial;
                      avcodec_flush_buffers(avctx);
                      return 0;
                  }
                  if (ret >= 0)
                      return 1;
              } while (ret != AVERROR(EAGAIN));
          }


          do {
              if (queue->nb_packets == 0)
              {
                   //todo
                  empty_queue_cond->notify_all();
              }
              if (packet_pending) {
                  packet_pending = 0;
              } else {
                  int old_serial = pkt_serial;
                  if (queue->packet_queue_get(pkt, 1, &pkt_serial) < 0)
                      return -1;
                  if (old_serial != pkt_serial) {
                      avcodec_flush_buffers(avctx);
                      finished = 0;
                      next_pts = start_pts;
                      next_pts_tb = start_pts_tb;
                  }
              }
              if (queue->serial == pkt_serial)
                  break;
              av_packet_unref(pkt);
          } while (1);

          if (avctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
              int got_frame = 0;
              ret = avcodec_decode_subtitle2(avctx, sub, &got_frame, pkt);
              if (ret < 0) {
                  ret = AVERROR(EAGAIN);
              } else {
                  if (got_frame && !pkt->data) {
                      packet_pending = 1;
                  }
                  ret = got_frame ? 0 : (pkt->data ? AVERROR(EAGAIN) : AVERROR_EOF);
              }
              av_packet_unref(pkt);
          } else {
              if (avcodec_send_packet(avctx, pkt) == AVERROR(EAGAIN)) {
                  av_log(avctx, AV_LOG_ERROR, "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
                  packet_pending = 1;
              } else {
                  av_packet_unref(pkt);
              }
          }
      }
}

void Decoder::decoder_destroy()
{
    av_packet_free(&pkt);
    avcodec_free_context(&avctx);
}

void Decoder::decoder_abort(FrameQueue *fq)
{
   queue->packet_queue_abort();
   fq->frame_queue_signal();

   //SDL_WaitThread(d->decoder_tid, NULL);
    //todo decoder_tid->

   queue->packet_queue_flush();
}

int Decoder::decoder_start()
{
    this->queue->packet_queue_start();

}

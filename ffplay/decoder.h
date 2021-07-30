#ifndef DECODER_H
#define DECODER_H

#include "framequeue.h"
#include "packetqueue.h"

class Decoder
{

public:
    int  decoder_init(AVCodecContext *avctx, PacketQueue *queue, std::condition_variable *empty_queue_cond);
    int  decoder_decode_frame(AVFrame *frame, AVSubtitle *sub) ;
    void decoder_destroy();
    void decoder_abort(FrameQueue *fq);
    int  decoder_start();


    AVPacket *pkt;
    PacketQueue *queue;
    AVCodecContext *avctx;
    int pkt_serial;
    int finished;
    int packet_pending;
    std::condition_variable *empty_queue_cond;

    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;

};

#endif // DECODER_H

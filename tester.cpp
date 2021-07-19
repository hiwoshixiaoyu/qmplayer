#include "tester.h"

Tester::Tester():
    isInit(false)
{

}


//std::thread a;
//std::thread b;
//

//q.packet_queue_start();

#include <iostream>
void readAVPacket(PacketQueue *q)
{
    int serial;
    AVPacket *pkt = av_packet_alloc();
    for(int i =0;i<20000;i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        printf("[read  avpacket]:%d\n",i);
        q->packet_queue_get(pkt,1,&serial);

    }
}


void writeAVPacket(PacketQueue *q)
{
    AVPacket *pkt= av_packet_alloc();
    for(int i =0;i<20000;i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(11));
        printf("[write frame avpacket]:%d\n",i);
        q->packet_queue_put(pkt);
    }
}




void readFrameQueue(FrameQueue *q)
{

    for(int i =0;i<200;i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        AVPacket *pkt= av_packet_alloc();
        auto *frame = q->frame_queue_peek_readable();
        if(nullptr != frame)
        {
            printf("[read frame queue]:%d pos:%d\n",i,frame->pos);
        }
        q->frame_queue_next();
    }

}


void writeFrameQueue(FrameQueue *q)
{

    int serial;
    for(int i =0;i<200;i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

         Frame *f =q->frame_queue_peek_writable();
         f->pos = i*10;
         printf("[write frame queue]:%d pos:%d\n",i,f->pos);
         q->frame_queue_push();


    }
}


void Tester::init()
{
    if(isInit)
    {
        return;
    }
    isInit = true;

    PacketQueue *qPQ = new PacketQueue;
    qPQ->packet_queue_start();

    FrameQueue *qFQ = new FrameQueue;
#if 0   //队列测试

    std::thread threadPtQ_w(writeAVPacket,qPQ);
    std::thread threadPtQ_r(readAVPacket,qPQ);
    threadPtQ_w.detach();
    threadPtQ_r.detach();
#endif

#if 1

    qFQ->frame_queue_init(qPQ,3,1);
    std::thread threadFQ_w(writeFrameQueue,qFQ);
    std::thread threadFQ_r(readFrameQueue,qFQ);
    threadFQ_w.detach();
    threadFQ_r.detach();
#endif
}

#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H

#include "Common.h"

//保存AVPacket
//解封装后的数据,解析数据的第一步
//链表实现的队列
class PacketQueue
{

public:
    PacketQueue();
    virtual ~PacketQueue();

    //清除队列内所有节点
    void packet_queue_flush();
    //中止
    void packet_queue_abort();
    //启用
    void packet_queue_start();


    /* 获取一个节点
    * return < 0 if aborted, 0 if no packet and > 0 if packet.
    * AVPacket:输出参数
    * block:1阻塞读模式  0非阻塞模式
    * serial:输出参数
    */
    int packet_queue_get(AVPacket *pkt, int block, int *serial);

    //存入一个空节点
    int packet_queue_put_nullpacket(AVPacket *pkt, int stream_index);
    //存入一个节点
    int packet_queue_put(AVPacket *pkt);

    //int decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, SDL_cond *empty_queue_cond);
private:
    int packet_queue_put_private(AVPacket *pkt);

public:
    //链表
    AVFifoBuffer *pkt_list;
    //队列中节点数
    int nb_packets;
    //队列所有节点字节总数
    int size;
    //队列所有节点合计时长
    int64_t duration;
    //是否中断队列,安全退出
    int abort_request;
    //序列号
    int serial;
    //线程安全锁
    std::mutex mutex;
    //信号量,读写线程互相通知
    std::condition_variable cond;

};

#endif // PACKETQUEUE_H

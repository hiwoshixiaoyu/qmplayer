#include "packetqueue.h"


PacketQueue::PacketQueue():
    pkt_list(nullptr),
    nb_packets(0),
    size(0),
    duration(0),
    abort_request(1),
    serial(0)
{

    pkt_list = av_fifo_alloc(sizeof(MyAVPacketList));

}

PacketQueue::~PacketQueue()
{
    packet_queue_flush();
    av_fifo_freep(&pkt_list);
}

void PacketQueue::packet_queue_flush()
{
    MyAVPacketList pkt1;
    mutex.lock();
    //链表是否有内存
    while (av_fifo_size(pkt_list) >= sizeof(pkt1))
    {
       av_fifo_generic_read(pkt_list, &pkt1, sizeof(pkt1), nullptr);
       av_packet_free(&pkt1.pkt);
    }
    nb_packets = 0;
    size = 0;
    duration = 0;
    serial++;
    mutex.unlock();
}

void PacketQueue::packet_queue_abort()
{
    mutex.lock();
    abort_request = 1;
    cond.notify_all();
    mutex.unlock();

}

void PacketQueue::packet_queue_start()
{
    mutex.lock();
    abort_request = 0;
    serial++;
    mutex.unlock();

}

int PacketQueue::packet_queue_get(AVPacket *pkt, int block, int *serial)
{
    MyAVPacketList pkt1;
    int ret;
    std::unique_lock<std::mutex> tmutex(mutex);
    for (;;)
    {
      if (abort_request)
      {
          ret = -1;
          break;
      }

      if (av_fifo_size(pkt_list) >= sizeof(pkt1))
      {
          av_fifo_generic_read(pkt_list, &pkt1, sizeof(pkt1), nullptr);
          nb_packets--;
          size -= pkt1.pkt->size + sizeof(pkt1);
          duration -= pkt1.pkt->duration;
          av_packet_move_ref(pkt, pkt1.pkt);
          if (serial)
          {
              *serial = pkt1.serial;
          }
          av_packet_free(&pkt1.pkt);
          ret = 1;
          break;
        }
        else if (!block)
        {
            //非阻塞就继续
            ret = 0;
            break;
        }
        else
        {
          //阻塞就等待别人通
          cond.wait(tmutex);

        }
      }

      return ret;
}

int PacketQueue::packet_queue_put_nullpacket(AVPacket *pkt, int stream_index)
{
    pkt->stream_index = stream_index;
     return packet_queue_put(pkt);
}

int PacketQueue::packet_queue_put(AVPacket *pkt)
{
    AVPacket *pkt1 = av_packet_alloc();
    int ret;
    if (!pkt1) {
        av_packet_unref(pkt);
        return -1;
    }
    av_packet_move_ref(pkt1, pkt);

    mutex.lock();
    ret = packet_queue_put_private(pkt1);
    mutex.unlock();

    if (ret < 0)
        av_packet_free(&pkt1);

    return ret;

}

int PacketQueue::packet_queue_put_private(AVPacket *pkt)
{
    MyAVPacketList pkt1;
    if (abort_request)
        return -1;

    if (av_fifo_space(pkt_list) < sizeof(pkt1))
    {
        if (av_fifo_grow(pkt_list, sizeof(pkt1)) < 0)
            return -1;
    }

    pkt1.pkt = pkt;
    pkt1.serial = serial;

    av_fifo_generic_write(pkt_list, &pkt1, sizeof(pkt1), NULL);
    nb_packets++;
    size += pkt1.pkt->size + sizeof(pkt1);
    duration += pkt1.pkt->duration;
    /* XXX: should duplicate packet data in DV case */
    cond.notify_one();
}


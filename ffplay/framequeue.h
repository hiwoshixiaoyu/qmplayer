#ifndef FRAMEQUEUE_H
#define FRAMEQUEUE_H



#include "Common.h"
#include "packetqueue.h"

//解复用的队列,使用环形数组实现
class FrameQueue
{

public:
    FrameQueue();
    ~FrameQueue();

    //初始化队列
    //pktq:指向的PacketQueue
    //max_size:队列大小
    //keep_last:是否保留最后一帧
    int frame_queue_init(PacketQueue *pktq, int max_size, int keep_last);


    //释放信号
    void frame_queue_signal();

    //获取当前节点指针
    Frame *frame_queue_peek();
    //获取下一节点指针
    Frame *frame_queue_peek_next();
    //获取上一节点指针
    Frame *frame_queue_peek_last();

    //获取写指针，如果队列满了就等待
    Frame *frame_queue_peek_writable();

    //获取读指针,如果为空则等待
    Frame *frame_queue_peek_readable();

    //更新写指针
    void frame_queue_push();
    //更新读指针(同时删除旧节点)
    void frame_queue_next();


    //todo
   // void decoder_abort(Decoder *d);

   int frame_queue_nb_remaining();
   int64_t frame_queue_last_pos();



private:
   //销毁函数
   void frame_queue_unref_item(Frame *vp);

   Frame queue[FRAME_QUEUE_SIZE];
   int rindex; //读索引
   int windex; //写索引
   int size;   //总帧数
   int max_size; //队列可以保存最大帧数
   int keep_last; //是否保留已播放的最后一帧标志
   int rindex_shown;//是否保留已播放的最后一帧实现手段

   std::mutex mutex;
   std::condition_variable cond;
   PacketQueue *pktq;
};

#endif // FRAMEQUEUE_H

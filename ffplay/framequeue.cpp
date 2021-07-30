#include "framequeue.h"


FrameQueue::FrameQueue():
    rindex(0),
    windex(0),
    size(0),
    max_size(0),
    keep_last(0),
    rindex_shown(0),
    pktq(nullptr)
{
    memset(queue,0x00,sizeof(queue));
}

FrameQueue::~FrameQueue()
{
    for (int i = 0; i < max_size; i++)
    {
        Frame *vp = &queue[i];
        frame_queue_unref_item(vp);
        av_frame_free(&vp->frame);
    }
}

int FrameQueue::frame_queue_init(PacketQueue *pktq, int max_size, int keep_last)
{
   this->pktq = pktq;
   this->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
   this->keep_last = !!keep_last;
   for (int i = 0; i < max_size; i++)
   {
       if (!(queue[i].frame = av_frame_alloc()))
        {
           return AVERROR(ENOMEM);
       }
   }
   return 0;
}


void FrameQueue::frame_queue_signal()
{
    mutex.lock();
    cond.notify_all();
    mutex.unlock();
}

Frame *FrameQueue::frame_queue_peek()
{
     return &(queue[(rindex + rindex_shown) % max_size]);
}

Frame *FrameQueue::frame_queue_peek_next()
{
    return &(queue[(rindex + rindex_shown + 1) % max_size]);
}

Frame *FrameQueue::frame_queue_peek_last()
{
    return &(queue[rindex]);
}

Frame *FrameQueue::frame_queue_peek_writable()
{
    /* wait until we have space to put a new frame */
    {
       std::unique_lock<std::mutex> t{mutex};
       while (size >= max_size &&
              !pktq->abort_request)
       {
           cond.wait(t);
       }
    }

    if (pktq->abort_request)
        return nullptr;

    return &(queue[windex]);
}


Frame *FrameQueue::frame_queue_peek_readable()
{
    /* wait until we have a readable a new frame */
    {
        std::unique_lock<std::mutex> t(mutex);
        while (size - rindex_shown <= 0 &&
               !pktq->abort_request)
        {

            cond.wait(t);
        }
    }

    if (pktq->abort_request)
        return nullptr;

    return &(queue[(rindex + rindex_shown) % max_size]);
}

void FrameQueue::frame_queue_push()
{
    if (++windex == max_size)
        windex = 0;

    mutex.lock();
    size++;
    cond.notify_all();
    mutex.unlock();
}

void FrameQueue::frame_queue_next()
{
    if (keep_last && !rindex_shown) {
        rindex_shown = 1;
        return;
    }
    frame_queue_unref_item(&queue[rindex]);
    if (++rindex == max_size)
        rindex = 0;
    mutex.lock();
    size--;
    cond.notify_all();
    mutex.unlock();
}

void FrameQueue::frame_queue_unref_item(Frame *vp)
{
    av_frame_unref(vp->frame);
    avsubtitle_free(&vp->sub);
}

/* return the number of undisplayed frames in the queue */
int FrameQueue::frame_queue_nb_remaining()
{
    return size - rindex_shown;
}

/* return last shown position */
 int64_t FrameQueue::frame_queue_last_pos()
{
    Frame *fp = &queue[rindex];
    if (rindex_shown && fp->serial == pktq->serial)
        return fp->pos;
    else
        return -1;
}



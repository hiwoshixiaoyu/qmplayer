#ifndef DECODETHREAD_H
#define DECODETHREAD_H

#include "VideoState.h"

class DecodeThread
{
public:
    DecodeThread(VideoState *is);

    int RunAudio();
    int RunVideo();
    int RunSubtitle();

    int get_video_frame(AVFrame *frame);
    int queue_picture(AVFrame *src_frame, double pts, double duration, int64_t pos, int serial);


    VideoState *is;
};

#endif // DECODETHREAD_H

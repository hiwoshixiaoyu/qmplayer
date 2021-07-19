#ifndef READTHREAD_H
#define READTHREAD_H

#include "DecodeThread.h"
#include "VideoState.h"
#include "clock.h"

#include "decoder.h"
#include "packetqueue.h"
class ReadThread
{
public:
    ReadThread();


        VideoState* getState();
    void start();
private:
    void init();

    int  Run();
    void RunDecoder(int type);



    AVPacket *pkt;
    int st_index[AVMEDIA_TYPE_NB];

    //文件名
    std::string m_filename;

    AVFormatContext *ic = nullptr;

    VideoState *m_VideoState;

    //解复用线程
    std::thread *m_threadRead;

    //视频、音频、字幕解封装线程
    std::thread *m_threadVideo;
    std::thread *m_threadAudio;
    std::thread *m_threadSubtitle;


    DecodeThread *m_threaddecodeImp;

    int audio_disable = 0;
    int video_disable = 0;
    int subtitle_disable = 0;


};


#endif // READTHREAD_H

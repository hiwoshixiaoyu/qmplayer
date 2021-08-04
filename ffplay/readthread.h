#ifndef READTHREAD_H
#define READTHREAD_H

#include "DecodeThread.h"
#include "VideoState.h"
#include "clock.h"
#include <queue>
#include "decoder.h"
#include "packetqueue.h"
class ReadThread
{
public:
    enum PlayerState
    {
        Player_Normal,
        Player_SetSource,
        Player_Play,
        Player_Pause,
    };


    std::queue<Task> m_queuetasks;
    std::mutex m_mut;

    void SendMsg(Task task);

    ReadThread();


    VideoState* getState();
    void start();


    //get msg from queue
    Task getQueueTask();

private:
    //set source file
    bool setsource();
    //Close player
    bool closePlayer();

    void init();
    int  Run();
    void RunDecoder(int type);




    AVFormatContext *m_ic;
    AVPacket *m_pkt;

    int st_index[AVMEDIA_TYPE_NB];

    //文件名
    std::string m_filename;

    AVFormatContext *ic = nullptr;

    VideoState *m_VideoState;

    //解复用线程
    std::thread *m_threadRead;


    //视频、音频、字幕解封装线程
    std::thread *m_threadVideo = nullptr;
    std::thread *m_threadAudio = nullptr;
    std::thread *m_threadSubtitle= nullptr;


    DecodeThread *m_threaddecodeImp;
    PlayerState m_playerstate;
};


#endif // READTHREAD_H

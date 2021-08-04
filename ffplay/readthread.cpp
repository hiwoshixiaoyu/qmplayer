#include "readthread.h"
#define AUDIO_DIFF_AVG_NB   20

static int fast = 0;
static int lowres = 0;
static int genpts = 0;

static const char* wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};




//static enum ShowMode show_mode = SHOW_MODE_NONE;
void ReadThread::SendMsg(Task task)
{
    m_mut.lock();
    m_queuetasks.push(task);
    m_mut.unlock();
}

ReadThread::ReadThread():
    m_playerstate(Player_Normal)
{



}


void ReadThread::init()
{
    av_log_set_flags(AV_LOG_SKIP_REPEATED);
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
    avformat_network_init();
    m_VideoState = new VideoState;

    m_VideoState->init();
    m_threaddecodeImp = new DecodeThread(m_VideoState);
    m_threadRead = new std::thread(&ReadThread::Run,this);
    m_threadRead->detach();


    m_pkt = av_packet_alloc();


}

void ReadThread::start()
{
    init();

}


//static int read_thread(void *arg)
Task ReadThread::getQueueTask()
{
    Task task;
    m_mut.lock();
    if(m_queuetasks.size()>0)
    {
        task = m_queuetasks.front();
        m_queuetasks.pop();
    }
    m_mut.unlock();
    return  task;
}

bool ReadThread::setsource()
{
    bool ret = false;
    do
    {
        memset(st_index, -1, sizeof(st_index));
        m_VideoState->setEOFStart();


        //创建上下文
        m_ic = avformat_alloc_context();
        if (nullptr == m_ic)
        {
            break;
        }


        //参数1:context上下文，如果为空内部创建，需要主动释放
        //参数2:支持http\rtsp\file
        //参数3:指定输入封装格式,传空自动探测
        //options:字典
        int err = avformat_open_input(&m_ic, m_filename.c_str(), nullptr, nullptr);
        if (err < 0)
        {
            print_error(m_filename.c_str(), err);
            break;
        }
        m_VideoState->setAVFormatContext(m_ic);

        //搜索流信息
        err = avformat_find_stream_info(m_ic, nullptr);
        if (err < 0)
        {
            av_log(NULL, AV_LOG_WARNING, "%s: could not find codec parameters\n", m_filename.c_str());
            break;
        }


        m_VideoState->setMaxFrameDuration();
        m_VideoState->setrealtime();

        //输出视频信息
        av_dump_format(m_ic, 0, m_filename.c_str(), 0);


        st_index[AVMEDIA_TYPE_VIDEO] =av_find_best_stream(m_ic, AVMEDIA_TYPE_VIDEO,st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
        st_index[AVMEDIA_TYPE_AUDIO] =av_find_best_stream(m_ic, AVMEDIA_TYPE_AUDIO,
                                    st_index[AVMEDIA_TYPE_AUDIO],
                                    st_index[AVMEDIA_TYPE_VIDEO],
                                    NULL, 0);

        st_index[AVMEDIA_TYPE_SUBTITLE] =
                av_find_best_stream(m_ic, AVMEDIA_TYPE_SUBTITLE,
                                    st_index[AVMEDIA_TYPE_SUBTITLE],
                                    (st_index[AVMEDIA_TYPE_AUDIO] >= 0 ?
                                     st_index[AVMEDIA_TYPE_AUDIO] :
                                     st_index[AVMEDIA_TYPE_VIDEO]),
                                    NULL, 0);


        //阻塞释放之前的线程
        RunDecoder(AVMEDIA_TYPE_VIDEO);
        RunDecoder(AVMEDIA_TYPE_AUDIO);
        RunDecoder(AVMEDIA_TYPE_SUBTITLE);

        if (m_VideoState->video_stream < 0 && m_VideoState->audio_stream < 0) {
            av_log(NULL, AV_LOG_FATAL, "Failed to open file '%s' or configure filtergraph\n", m_filename.c_str());
            break;
        }

        ret = true;
    }while(false);
    return  ret;
}

bool ReadThread::closePlayer()
{
    if(Task_Play== m_playerstate ||  //播放
    Task_Pause == m_playerstate ||   //暂停
    Task_Stop == m_playerstate ||    //停止
    Task_Seek == m_playerstate )
    {

        m_VideoState->destroy();
        if (nullptr != m_ic)
        {
            avformat_close_input(&m_ic);
            avformat_free_context(m_ic);
            m_ic = nullptr;
        }




    }
//    m_playerstate = Player_Normal;
//    m_VideoState->init();
}
int ReadThread::Run()
{
    bool ret = false;
    for(;;)
    {
        Task task = getQueueTask();
        if( Task_Nil == task.type)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            //如果播放中执行队列
            if (Player_Play == m_playerstate &&
                    m_VideoState->tickTask(m_pkt))
            {
                break;
            }
            continue;
        }

        if(Task_Close == task.type)
        {
            closePlayer();
            break;
        }

        if(Task_File == task.type)
        {
            m_filename = task.filePath;
            ret = setsource();
            if(ret)
            {
               m_playerstate = Player_SetSource;
            }
        }

        if(Task_Play == task.type)
        {
            //todo
            m_playerstate = Player_Play;
        }


    }


    if (nullptr != m_ic)
    {
        avformat_close_input(&m_ic);
        avformat_free_context(m_ic);
    }

    if(nullptr != m_pkt)
    {
        av_packet_free(&m_pkt);
    }
    return ret;
}

void ReadThread::RunDecoder(int type)
{

    if( AVMEDIA_TYPE_VIDEO == type && st_index[AVMEDIA_TYPE_VIDEO] >= 0)
    {

        m_VideoState->stream_component_open(st_index[AVMEDIA_TYPE_VIDEO]);
        if(m_threadVideo ==  nullptr)
        {
            m_threadVideo = new std::thread(&DecodeThread::RunVideo,m_threaddecodeImp);
            m_threadVideo->detach();
        }
    }

//    if( AVMEDIA_TYPE_AUDIO == type && st_index[AVMEDIA_TYPE_VIDEO] >= 0)
//    {
//       m_VideoState->stream_component_open(st_index[AVMEDIA_TYPE_AUDIO]);
//       m_threadAudio = new std::thread(&DecodeThread::RunAudio,m_threaddecodeImp);
//       m_threadAudio->detach();
//    }

//    if( AVMEDIA_TYPE_SUBTITLE == type && st_index[AVMEDIA_TYPE_SUBTITLE] >= 0)
//    {
//        m_VideoState->stream_component_open(st_index[AVMEDIA_TYPE_SUBTITLE]);
//        m_threadSubtitle = new std::thread(&DecodeThread::RunSubtitle,m_threaddecodeImp);
//        m_threadSubtitle->detach();
//    }

}

VideoState *ReadThread::getState()
{
    return m_VideoState;
}


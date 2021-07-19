#include "readthread.h"
#define AUDIO_DIFF_AVG_NB   20

static int fast = 0;
static int lowres = 0;
static int genpts = 0;

static const char* wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};




//static enum ShowMode show_mode = SHOW_MODE_NONE;
ReadThread::ReadThread()
{
    av_log_set_flags(AV_LOG_SKIP_REPEATED);
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
    avformat_network_init();
    m_filename = "E://a.mp4";
    m_VideoState = new VideoState;
    this->init();

}


void ReadThread::init()
{
    m_VideoState = new VideoState;
    m_VideoState->init();
    m_threadRead = new std::thread(&ReadThread::Run,this);
    m_threaddecodeImp = new DecodeThread(m_VideoState);
}

void ReadThread::start()
{
    m_threadRead->detach();
}


//static int read_thread(void *arg)
int ReadThread::Run()
{
    int ret = 0;
    bool isAllocContex = false;
    do
    {
        memset(st_index, -1, sizeof(st_index));
        m_VideoState->setEOFStart();

        pkt = av_packet_alloc();
        if ( nullptr == pkt)
        {
            av_log(NULL, AV_LOG_FATAL, "Could not allocate packet.\n");
            ret = -1;
            break;
        }

        //创建上下文
        ic = avformat_alloc_context();
        if (nullptr == ic)
        {
            av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
            ret = -1;
            break;
        }
        isAllocContex = true;

        //参数1:context上下文，如果为空内部创建，需要主动释放
        //参数2:支持http\rtsp\file
        //参数3:指定输入封装格式,传空自动探测
        //options:字典
        int err = avformat_open_input(&ic, m_filename.c_str(), nullptr, nullptr);
        if (err < 0)
        {
            print_error(m_filename.c_str(), err);
            ret = -1;
            break;
        }
        m_VideoState->setAVFormatContext(ic);

        if (genpts)//???这个是啥
        {
            //Generate missing pts even if it requires parsing future frames.
             ic->flags |= AVFMT_FLAG_GENPTS;
        }

        //???
        av_format_inject_global_side_data(ic);

        //搜索流信息
        err = avformat_find_stream_info(ic, nullptr);
        if (err < 0)
        {
            av_log(NULL, AV_LOG_WARNING, "%s: could not find codec parameters\n", m_filename.c_str());
            ret = -1;
            break;
        }

        if (ic->pb)
        {
            ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end
        }

        m_VideoState->setMaxFrameDuration();
        m_VideoState->setrealtime();

        //输出视频信息
        av_dump_format(ic, 0, m_filename.c_str(), 0);

        //???
//        for (int i = 0; i < ic->nb_streams; i++)
//        {
//           AVStream *st = ic->streams[i];
//           enum AVMediaType type = st->codecpar->codec_type;
//           st->discard = AVDISCARD_ALL;
//           if (type >= 0 /*&& wanted_stream_spec[type] */&& st_index[type] == -1)
//           {
//               if (avformat_match_stream_specifier(ic, st,nullptr/*, wanted_stream_spec[type]*/) > 0)
//               {
//                     st_index[type] = i;
//                }

//           }
//        }

        if (!video_disable)
        {
            st_index[AVMEDIA_TYPE_VIDEO] =
                av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO,
                                    st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
        }
        if (!audio_disable)
        {
            st_index[AVMEDIA_TYPE_AUDIO] =
                av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
                                    st_index[AVMEDIA_TYPE_AUDIO],
                                    st_index[AVMEDIA_TYPE_VIDEO],
                                    NULL, 0);
        }

        if (!video_disable && !subtitle_disable)
        {
            st_index[AVMEDIA_TYPE_SUBTITLE] =
                av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE,
                                    st_index[AVMEDIA_TYPE_SUBTITLE],
                                    (st_index[AVMEDIA_TYPE_AUDIO] >= 0 ?
                                     st_index[AVMEDIA_TYPE_AUDIO] :
                                     st_index[AVMEDIA_TYPE_VIDEO]),
                                    NULL, 0);
        }


        RunDecoder(AVMEDIA_TYPE_VIDEO);
        RunDecoder(AVMEDIA_TYPE_AUDIO);
        RunDecoder(AVMEDIA_TYPE_SUBTITLE);


        if (m_VideoState->video_stream < 0 && m_VideoState->audio_stream < 0) {
            av_log(NULL, AV_LOG_FATAL, "Failed to open file '%s' or configure filtergraph\n",
                   m_filename.c_str());
            ret = -1;
            break;
        }

        for (;;)
        {
            if (m_VideoState->tickTask(pkt))
            {
               break;
            }
        }
    }while(false);


    if(isAllocContex)
    {
        if (ic && !m_VideoState->ic)
            avformat_close_input(&ic);
    }
    return ret;
}

void ReadThread::RunDecoder(int type)
{

    if( AVMEDIA_TYPE_VIDEO == type && st_index[AVMEDIA_TYPE_VIDEO] >= 0)
    {
        m_VideoState->stream_component_open(st_index[AVMEDIA_TYPE_VIDEO]);
        m_threadVideo = new std::thread(&DecodeThread::RunAudio,m_threaddecodeImp);
        m_threadVideo->detach();

    }

    if( AVMEDIA_TYPE_AUDIO == type && st_index[AVMEDIA_TYPE_VIDEO] >= 0)
    {
       m_VideoState->stream_component_open(st_index[AVMEDIA_TYPE_AUDIO]);
       m_threadAudio = new std::thread(&DecodeThread::RunAudio,m_threaddecodeImp);
       m_threadAudio->detach();
    }

    if( AVMEDIA_TYPE_SUBTITLE == type && st_index[AVMEDIA_TYPE_SUBTITLE] >= 0)
    {
        m_VideoState->stream_component_open(st_index[AVMEDIA_TYPE_SUBTITLE]);
        m_threadSubtitle = new std::thread(&DecodeThread::RunSubtitle,m_threaddecodeImp);
        m_threadSubtitle->detach();
    }

}

VideoState *ReadThread::getState()
{
    return m_VideoState;
}


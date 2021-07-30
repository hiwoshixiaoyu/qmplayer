#ifndef VIDEOSTATE_H
#define VIDEOSTATE_H
#include "clock.h"
#include "common.h"
#include "decoder.h"
#include "framequeue.h"


static int startup_volume = 100;
static int av_sync_type = AV_SYNC_AUDIO_MASTER;
static int64_t duration = AV_NOPTS_VALUE;
static int64_t start_time = AV_NOPTS_VALUE;
static int infinite_buffer = -1;
#define MAX_QUEUE_SIZE (15 * 1024 * 1024)

#define SAMPLE_ARRAY_SIZE (8 * 65536)
#define MIN_FRAMES 25
class   VideoState
{
public:
  const AVInputFormat *iformat;

  //是否弃用
  int abort_request;
  //是否强制刷新
  int force_refresh;
  //是否暂停
  int paused;
  int last_paused;
  int queue_attachments_req;
  //seek请求
  int seek_req;
  int seek_flags;
  //seek目标位置
  int64_t seek_pos;
  //seek位置的增量
  int64_t seek_rel;
  int read_pause_return;

  AVFormatContext *ic;
  int realtime;

  //音频时钟、视频时钟、外部时钟
  Clock audclk;
  Clock vidclk;
  Clock extclk;


  //解封装的队列 视频队列、音频队列、字幕队列
  PacketQueue videoq;
  PacketQueue audioq;
  PacketQueue subtitleq;

  //解复用的队列 视频队列、音频队列、字幕队列
  FrameQueue pictq;
  FrameQueue subpq;
  FrameQueue sampq;


  //视频、音频、字幕解码器
  Decoder viddec;
  Decoder auddec;
  Decoder subdec;

  //视频流索引、音频索引、字幕索引
  int video_stream;
  int audio_stream;
  int subtitle_stream;



  //视频流、音频流、字幕流
  AVStream *video_st;
  AVStream *audio_st;
  AVStream *subtitle_st;


  int av_sync_type;

  double audio_clock;
  int audio_clock_serial;
  double audio_diff_cum; /* used for AV difference average computation */
  double audio_diff_avg_coef;
  double audio_diff_threshold;
  int audio_diff_avg_count;


  int audio_hw_buf_size;
  uint8_t *audio_buf;
  uint8_t *audio_buf1;
  unsigned int audio_buf_size; /* in bytes */
  unsigned int audio_buf1_size;
  int audio_buf_index; /* in bytes */
  int audio_write_buf_size;
  //音量
  int audio_volume;
  //是否静音
  int muted;
  struct AudioParams audio_src;


  struct AudioParams audio_tgt;
  //音频重采样上下文
  struct SwrContext *swr_ctx;
  //丢弃视频packet数
  int frame_drops_early;
  //丢弃视频frame数
  int frame_drops_late;

  enum ShowMode {
      SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
  } show_mode;

  int16_t sample_array[SAMPLE_ARRAY_SIZE];
  int sample_array_index;
  int last_i_start;
  RDFTContext *rdft;
  int rdft_bits;
  FFTSample *rdft_data;
  int xpos;
  double last_vis_time;


  double frame_timer;
  double frame_last_returned_time;
  double frame_last_filter_delay;



  double max_frame_duration;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
  struct SwsContext *img_convert_ctx;
  struct SwsContext *sub_convert_ctx;
  int eof;

  char *filename;
  int width, height, xleft, ytop;
  int step;


  int last_video_stream, last_audio_stream, last_subtitle_stream;


  //SDL_cond *continue_read_thread;

  std::mutex wait_mutex;
  std::condition_variable continue_read_thread;



public:
  void initDecoder();
  void init();
  bool tickTask(AVPacket *pkt);
  int  stream_component_open(int stream_index);
  void   stream_toggle_pause();
  double get_master_clock();
  int    get_master_sync_type();

  int queue_picture(AVFrame *src_frame, double pts, double duration, int64_t pos, int serial);

  int get_video_frame(AVFrame *frame);

  void setEOFStart()
  {
      eof = 0;
  }
  void setEOFEnd()
  {
      eof = 1;
  }
  void setAVFormatContext(AVFormatContext *ic)
  {
      this->ic = ic;
  }
  void setMaxFrameDuration()
  {
      max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
  }


  void  setrealtime()
  {
      realtime = 0;
      do
      {
          if(   !strcmp(ic->iformat->name, "rtp")
             || !strcmp(ic->iformat->name, "rtsp")
             || !strcmp(ic->iformat->name, "sdp")
          )
          {
                realtime = 1;
          }


          if(ic->pb && (   !strncmp(ic->url, "rtp:", 4)
                       || !strncmp(ic->url, "udp:", 4)
                      )
          )
          {
                realtime =1;
          }

      }while(false);
      return ;
  }


  void check_external_clock_speed();
private:


  bool dealPaused();
  void dealSeek_req();
  int  dealqueue_attachments_req(AVPacket *pkt);
  int  dealReadFrame(AVPacket *pkt);
  bool dealinfinite_buffer();

  void packet_queue_put(AVPacket *pkt);
  void step_to_next_frame();
  void pushNullPack(AVPacket *pkt);
  void wait();

};
#endif // VIDEOSTATE_H




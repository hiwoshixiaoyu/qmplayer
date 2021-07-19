#ifndef CLOCK_H
#define CLOCK_H
#include "common.h"

#define AV_NOSYNC_THRESHOLD 10.0

class  Clock
{
public:
    //当前帧 显示时间戳
    double pts;
    //当前帧显示与当前系统时钟的时间差
    double pts_drift;
    //当前时钟(如视频时钟)最后一次更新时间，也可以说是当前时钟时间
    double last_updated;
    //播放速度
    double speed;
    //序列号
    int serial;           /* clock is based on a packet with this serial */
    //暂停标志
    int paused;
    //指向queue_serial
    int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */

    void set_clock_at(double pts, int serial, double time)
    {
        this->pts = pts;
        this->last_updated = time;
        this->pts_drift = pts - time;
        this->serial = serial;
    }

    void set_clock( double pts, int serial)
    {
        double time = av_gettime_relative() / 1000000.0;
        set_clock_at(pts, serial, time);
    }

     void set_clock_speed(double speed)
    {
        set_clock(get_clock(), serial);
        this->speed = speed;
    }

     void init_clock(int *queue_serial)
    {
        this->speed = 1.0;
        this->paused = 0;
        this->queue_serial = queue_serial;
        set_clock(NAN, -1);
    }

     void sync_clock_to_slave(Clock *c, Clock *slave)
    {
        double clock = c->get_clock();
        double slave_clock = slave->get_clock();
        if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
            c->set_clock(slave_clock, slave->serial);
    }

    double get_clock()
    {
        if (*queue_serial != this->serial)
            return NAN;
        if (this->paused) {
            return this->pts;
        }
        double time = av_gettime_relative() / 1000000.0;
        return this->pts_drift + time - (time - this->last_updated) * (1.0 - this->speed);

    }
} ;
#endif // CLOCK_H

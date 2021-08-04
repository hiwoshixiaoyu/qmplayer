FFMPEG_INCLUDE  = $$PWD/include
FFMPEG_LIB      = $$PWD/lib

LIBS += $$FFMPEG_LIB/libavcodec.dll.a      \
        $$FFMPEG_LIB/libavdevice.dll.a     \
        $$FFMPEG_LIB/libavfilter.dll.a     \
        $$FFMPEG_LIB/libavformat.dll.a     \
        $$FFMPEG_LIB/libavutil.dll.a       \
        $$FFMPEG_LIB/libswresample.dll.a   \
        $$FFMPEG_LIB/libswscale.dll.a      \

INCLUDEPATH += $$FFMPEG_INCLUDE

HEADERS += \
    $$PWD/videostate.h \
    $$PWD/decodethread.h \
    $$PWD/clock.h \
    $$PWD/common.h \
    $$PWD/decoder.h \
    $$PWD/packetqueue.h \
    $$PWD/framequeue.h \
    $$PWD/readthread.h \

SOURCES += \
    $$PWD/videostate.cpp \
    $$PWD/decoder.cpp \
    $$PWD/packetqueue.cpp \
    $$PWD/framequeue.cpp \
    $$PWD/readthread.cpp \
    $$PWD/decodethread.cpp \

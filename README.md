# qmplayer
base on ffplay.c use Qt

# qmplayer
 基于ffplay、C++11开发的跨平台视频播放器。旨在把解码和渲染播放进行剥离，方便进行移植。

# 如何编译
- 安装Qt5 
- 使用Qt Creater、minggw64编辑器
- windows下载ffmpeg

 下载网址：https://github.com/BtbN/FFmpeg-Builds/releases

- 添加include到如下文件夹
\qmplayer\ffplay\include
- 添加.lib  libswresample.dll.到如下文件夹
\qmplayer\ffplaylib
- 添加dll到编译完成exe同级目录即可


# 参考代码ffplay.c代码
https://github.com/FFmpeg/FFmpeg/tree/master/fftools


作者邮箱：
hiwoshixiaoyu@163.com

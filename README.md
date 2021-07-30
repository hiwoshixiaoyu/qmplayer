# qmplayer
base on ffplay.c use Qt

# qmplayer
 基于ffplay、C++11开发的跨平台视频播放器。旨在把解码和渲染播放进行剥离，方便进行移植。

# 进度
- 完成了fflpay音频解码队列
- 完成了fflpay视频解码队列
- 完成了fflpay字幕解码队列
- 完成了队列的读写测试
- opengl4.3使用了 vbo vao fbo进行高效渲染
- 完成了YUV opengl渲染
# 问题
- 内存泄漏比较严重

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

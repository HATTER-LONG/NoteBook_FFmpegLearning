# FFmpeg 工具安装

ffmpeg 需要写配套库，否则类似 264 265 转码功能无效，Ubuntu20.04 环境：

```shell
❯ sudo apt-get install libavcodec-extra libfdk-aac-dev libass-dev libopus-dev libtheora-dev libvorbis-dev libvpx-dev libssl-dev nasm libx264-dev libx265-dev

❯ ./configure --cc=clang --cxx=c++ --enable-gpl --enable-libass --enable-libx264 --enable-libx265

❯ make -j16

❯ sudo make install

❯ ffmpeg -version
ffmpeg version N-100770-g1775688292 Copyright (c) 2000-2021 the FFmpeg developers
built with clang version 11.0.0 (https://github.com/llvm/llvm-project.git 0160ad802e899c2922bc9b29564080c22eb0908c)
configuration: --cc=clang --cxx=c++ --enable-gpl --enable-libass --enable-libx264 --enable-libx265
libavutil      56. 63.101 / 56. 63.101
libavcodec     58.117.101 / 58.117.101
libavformat    58. 65.101 / 58. 65.101
libavdevice    58. 11.103 / 58. 11.103
libavfilter     7. 96.100 /  7. 96.100
libswscale      5.  8.100 /  5.  8.100
libswresample   3.  8.100 /  3.  8.100
libpostproc    55.  8.100 / 55.  8.100
```

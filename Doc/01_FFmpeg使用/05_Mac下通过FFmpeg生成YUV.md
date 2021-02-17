# 生成 YUV

- [生成 YUV](#生成-yuv)
  - [FFmpeg 命令提取 yuv](#ffmpeg-命令提取-yuv)

## FFmpeg 命令提取 yuv

ffmpeg -i small_bunny_1080p_60fps.mp4 -an -c:v rawvideo -pix_fmt yuv420p out.yuv

- `-an`：audio null 没有音频；
- `-c:v`：video codec 视频解码器，指定使用 rawvideo 处理视频获取 yuv 数据；
- `-pix_fmt yuv420p`：指定 yuv 输出格式。

ffplay -pix_fmt yuv420p -s 1920x1080 out.yuv

- `-pix_fmt yuv420p`：指定 yuv 播放格式；
- `-s 1920x1080`：指定播放分辨率。

ffplay -pix_fmt yuv420p -s 1920x1080 -vf extractplanes='y' out.yuv

- `-vf extractplanes='y'`：简单滤波，单独播放 y 分量。

ffmpeg -i small_bunny_1080p_60fps.mp4 -filter_complex "extractplanes=y+u+v[y][u][v]" -map "[y]" y.yuv -map "[u]" u.yuv -map "[v]" v.yuv

- 复杂滤波器，将 y、u、v 分量保存到单独文件中。

播放各个分量：

- ffplay -pix_fmt gray -s 1920x1080 y.yuv
  - `-pix_fmt gray`：需要指定播放 y 分量

- ffplay -pix_fmt gray -s 960x540 u.yuv
- ffplay -pix_fmt gray -s 960x540 v.yuv
  - 分辨率 4:2 要除 2

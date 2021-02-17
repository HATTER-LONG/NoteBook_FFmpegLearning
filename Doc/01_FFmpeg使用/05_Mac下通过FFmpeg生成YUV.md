# 生成 YUV

- [生成 YUV](#生成-yuv)
  - [FFmpeg 命令提取 yuv](#ffmpeg-命令提取-yuv)
  - [修改代码](#修改代码)
  - [FFmpeg 查看设备命令](#ffmpeg-查看设备命令)

[参考代码](../../LibFFmpegUsingExample/Src/01_FFmpegUsingExample/03_FFmpegReadYUMBaseMac.c)

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

## 修改代码

1. 修改采集设备，由原来的音频设备更改为视频设备（摄像头）。
2. 增加参数 AVDictionary 给采集器，需要采集的分辨率、帧率、YUV的格式等。

    ```cpp
    //[[video device]:[audio device]]
    // mac 有两个默认相机 0：机器本身相机 1：桌面
    char* devicename = "0";
    // ffmpeg -s 1920*1080 out.yum
    // key: video_size == -s
    //      framerate 帧率
    // value: 1920x1080
    av_dict_set(&options, "video_size", "640x480", 0);
    av_dict_set(&options, "framerate", "30", 0);
    av_dict_set(&options, "pixel_format", "uyvy422", 0);
    ```

3. 修改名称录制下来的 .pcm 改为 yuv。
4. 修改写入 size。

    ```cpp
    while ((ret = av_read_frame(fmt_ctx, &pkt)) == 0 && rec_status)
    {
        // write file
        //（宽 x 高）x (位深 yuv422 = 2/ yuv420 = 1.5/ yuv444 = 3)
        fwrite(pkt.data, 1, 614400, outfile);   // 614400 分辨率计算码率  640*480* 2(yuv422)
        fflush(outfile);

        // release pkt
        av_packet_unref(&pkt);
    }
    ```

## FFmpeg 查看设备命令

```shell
ffmpeg -devices
ffmpeg -f avfoundation -list_devices true -i ""
ffmpeg -f avfoundation -framerate 30 -i "0" -target pal-vcd test.mpg
```

```shell
ffmpeg -f avfoundation -i "0"  -vcodec libx264 myDesktop2.mkv
ffmpeg -i myDesktop2.mkv -an -c:v rawvideo out.yuv
ffplay -s "1920x1080"  -pix_fmt yuv422p out.yuv

# 转 yuv 格式
ffmpeg -pix_fmt yuv422p -s "1920x1080" -i out.yuv -pix_fmt nv12 nv12out.yuv
```

查询 yuv 格式[网址](www.fourcc.org/yuv.php)

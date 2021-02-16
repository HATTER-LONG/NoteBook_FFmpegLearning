# 创建 AAC 编码器

- [创建 AAC 编码器](#创建-aac-编码器)
  - [FFmpeg 编码过程](#ffmpeg-编码过程)
    - [创建并打开编码器 API](#创建并打开编码器-api)
  - [实验 AAC 打开编码器](#实验-aac-打开编码器)

[本节源码](../../LibFFmpegUsingExample/Src/01_FFmpegUsingExample/02_FFmpegCodecAACBaseMac.c)

## FFmpeg 编码过程

1. 创建编码器。
2. 创建上下文。
3. 打开编码器。
4. 送数据给编码器：编码器内部有缓冲区，主要是由于视频 B 帧编码可能时依赖后面的帧，因此送数据后不会立即得到编码完成的数据。
5. 编码。
6. 释放资源。

### 创建并打开编码器 API

1. 创建编码器：avcodec_find_encoder 查找编码器。
2. 创建上下文：avcodec_alloc_context3 第三版创建方法。
3. 打开编码器：avcodec_open2。

## 实验 AAC 打开编码器

1. 创建并打开编码器，编码器的参数应该与重采样后的输出保持一致，置于为什么需要重采样将 AV_SAMPLE_FMT_FLT 转为 AV_SAMPLE_FMT_S16，主要由于 libfdk_aac Api 中采样大小规定了 S16：

    ```cpp
    static AVCodecContext* open_coder()
    {

        //打开编码器
        // avcodec_find_encoder(AV_CODEC_ID_AAC); 通过枚举查找编码器
        AVCodec* codec = avcodec_find_encoder_by_name("libfdk_aac");

        //创建 codec 上下文
        AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
        // 这部分与重采样输出的格式信息一致
        codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;         //输入音频的采样大小
        codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;   //输入音频的channel layout
        codec_ctx->channels = 2;                           //输入音频 channel 个数
        codec_ctx->sample_rate = 44100;                    //输入音频的采样率
        codec_ctx->bit_rate = 0;                           // AAC_LC: 128K, AAC HE: 64K, AAC HE V2: 32K
        codec_ctx->profile = FF_PROFILE_AAC_HE_V2;         //设置了 profile 后就不需要再设置 bit_rate 了，改成 0 .阅读 ffmpeg 代码

        //打开编码器
        if (avcodec_open2(codec_ctx, codec, NULL) < 0)
        {
            //

            return NULL;
        }

        return codec_ctx;
    }
    ```

2. 发送数据给编码器：

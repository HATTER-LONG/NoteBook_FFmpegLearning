# 创建 AAC 编码器

- [创建 AAC 编码器](#创建-aac-编码器)
  - [FFmpeg 编码过程](#ffmpeg-编码过程)
    - [创建并打开编码器 API](#创建并打开编码器-api)
    - [使用编码器的 API](#使用编码器的-api)
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

### 使用编码器的 API

1. 输入数据：avcodec_send_frame。输入 AVFrame 结构数据，代指未编码的数据。
   - 至于为什么从设备上读取的是 AVPacket 而不直接是 AVFrame。由于 avformat_open_input 会将目标认为多媒体文件进行打开，会认为读取的是编码后的数据。正常流程应当再读取完成数据后进行解码操作，但是这里由于从设备采集的就是未编码数据，因此跳过了这一步，直接使用 memcpy 拷贝了过来。
2. 获取编码后的数据：avcodec_receive_packet。获取 AVPacket 结构数据，表示编码完成的数据。

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

2. 创建输入用的缓冲 AVFrame 和获取编码后数据用的 AVPacket，注意 AVFrame 创建分为两步：

    ```cpp
    static AVFrame* create_frame()
    {
        AVFrame*buff frame = NULL;

        //音频输入数据
        //1. 创建一个 AVFrame 结构，但是其中 AVBufferRef *buf[AV_NUM_DATA_POINTERS]; 字段需要另外初始化。
        frame = av_frame_alloc();

        // set parameters
        frame->nb_samples = 512;                       //单通道一个音频帧的采样数
        frame->format = AV_SAMPLE_FMT_S16;             //每个采样的大小 2
        frame->channel_layout = AV_CH_LAYOUT_STEREO;   // channel layout 2

        // alloc inner memory
        //2. 给 AVBufferRef *buf[AV_NUM_DATA_POINTERS]; 进行初始化。大小依赖于我们设定的参数
        av_frame_get_buffer(frame, 0);   // 512 * 2 * 2= 2048
    }
    //////////////////////////////////////////

    AVFrame* frame = create_frame();
    if (!frame)
    {
        // printf(...)
        goto __ERROR;
    }

    newpkt = av_packet_alloc();   //分配编码后的数据空间
    if (!newpkt)
    {
        printf("Error, Failed to alloc buf in frame!\n");
        goto __ERROR;
    }
    ```

3. 输入数据到编码器，将抽采样后的数据拷贝到 frame 中，[注意 AVFrame 结构中的 data 与 buff 字段区别](https://blog.csdn.net/m0_37599645/article/details/111937673)：

    ```cpp
    // read data from device
    while ((ret = av_read_frame(fmt_ctx, &pkt)) == 0 && rec_status)
    {

        //进行内存拷贝，按字节拷贝的
        memcpy((void*)src_data[0], (void*)pkt.data, pkt.size);

        //重采样
        swr_convert(swr_ctx,             //重采样的上下文
            dst_data,                    //输出结果缓冲区
            512,                         //每个通道的采样数
            (const uint8_t**)src_data,   //输入缓冲区
            512);                        //输入单个通道的采样数

        //将重采样的数据拷贝到 frame 中
        memcpy((void*)frame->data[0], dst_data[0], dst_linesize);

        // encode
        encode(c_ctx, frame, newpkt, outfile);

        //
        av_packet_unref(&pkt);   // release pkt
    }

    //强制将编码器缓冲区中的音频进行编码输出
    encode(c_ctx, NULL, newpkt, outfile);
    ```

4. 编码器编码，通过 avcodec_send_frame 送到编码器缓冲区，通过 while 循环读取编码好的数据，直到读空，其实这里设计最好多线程进行，单独一条线程用于处理编码好的数据：

    ```cpp
    static void encode(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, FILE* output)
    {

        int ret = 0;

        //将数据送编码器
        ret = avcodec_send_frame(ctx, frame);

        //如果 ret>=0 说明数据设置成功
        while (ret >= 0)
        {
            //获取编码后的音频数据,如果成功，需要重复获取，直到失败为止
            ret = avcodec_receive_packet(ctx, pkt);

            // 数据空时退出循环
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                return;
            }
            if (ret < 0)
            {
                printf("Error, encoding audio frame\n");
                exit(-1);
            }

            // write file
            fwrite(pkt->data, 1, pkt->size, output);
            fflush(output);
        }

        return;
    }
    ```

5. 释放资源：

    ```cpp
    static void free_data_4_resample(uint8_t** src_data, uint8_t** dst_data)
    {
        //释放输入输出缓冲区
        if (src_data)
        {
            av_freep(&src_data[0]);
        }
        av_freep(&src_data);

        if (dst_data)
        {
            av_freep(&dst_data[0]);
        }
        av_freep(&dst_data);
    }
    /////////////////////////////////////////////
    __ERROR:
    //释放 AVFrame 和 AVPacket
    if (frame)
    {
        av_frame_free(&frame);
    }

    if (newpkt)
    {
        av_packet_free(&newpkt);
    }

    //释放重采样缓冲区
    free_data_4_resample(src_data, dst_data);
    ```

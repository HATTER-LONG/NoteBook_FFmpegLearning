# 音频重采样

- [音频重采样](#音频重采样)
  - [音频重采样的概念](#音频重采样的概念)
    - [重采样的原因](#重采样的原因)
    - [是否需要重采样](#是否需要重采样)
    - [重采样步骤](#重采样步骤)
  - [实验](#实验)

[本节源码](../../LibFFmpegUsingExample/Src/01_FFmpegUsingExample/01_FFmpegWithAudioBaseMac.c)

## 音频重采样的概念

将音频三元组：采样率、采样大小（位深）和通道数的值转成另外一组值。

例如：将 44100/16/2 转成 48000/16/2

### 重采样的原因

1. 设备采集的音频数据与编码器要求的数据不一致。
2. 扬声器要求的音频数据与要播放的音频数据不一致。
3. 更方便运算，例如回音消除时会先将多声道转为单声道处理。

### 是否需要重采样

1. 要了解音频设备的参数。
2. 查看 FFmpeg 源码。

### 重采样步骤

1. 创建重采样上下文。
2. 设置参数。
3. 初始化重采样。
4. 进行重采样。
5. 清理。

- API:
  1. swr_alloc_set_opts：创建一个上下文，同时设置重采样参数。
  2. swr_init：重采样初始化。
  3. swr_convert：音频转换。
  4. swr_free：释放资源。

## 实验

1. 重采样的头文件：

    ```cpp
    #include "libswresample/swresample.h
    ```

2. 创建重采样上下文，在读取音频数据前设置 av_read_frame 之前：

    ```cpp
    SwrContext* init_swr(){
        
        SwrContext *swr_ctx = NULL;
        
        //channel, number/
        swr_ctx = swr_alloc_set_opts(NULL,                //ctx
                                    AV_CH_LAYOUT_STEREO, //输出channel布局
                                    AV_SAMPLE_FMT_S16,   //输出的采样格式
                                    44100,               //采样率
                                    AV_CH_LAYOUT_STEREO, //输入channel布局
                                    AV_SAMPLE_FMT_FLT,   //输入的采样格式
                                    44100,               //输入的采样率
                                    0, NULL);
        if(!swr_ctx){
            
        }
        
        if(swr_init(swr_ctx) < 0){
            
        }
        
        return swr_ctx;
    }
    ```

3. 开始重采样，重采样接口需要输入、输出缓冲区，可以使用 av_samples_alloc_array_and_samples 来生成：

    ```cpp
    //重采样缓冲区
    uint8_t **src_data = NULL;
    int src_linesize = 0;
    
    uint8_t **dst_data = NULL;
    int dst_linesize = 0;
    //....
    SwrContext* swr_ctx = init_swr();
    
    //4096/4(32位)=1024/2(双通道)=512(单通道)
    //创建输入缓冲区
    av_samples_alloc_array_and_samples(&src_data,         //输出缓冲区地址
                                       &src_linesize,     //缓冲区的大小
                                       2,                 //通道个数
                                       512,               //单通道采样个数
                                       AV_SAMPLE_FMT_FLT, //采样格式
                                       0);
    
    //创建输出缓冲区
    av_samples_alloc_array_and_samples(&dst_data,         //输出缓冲区地址
                                       &dst_linesize,     //缓冲区的大小
                                       2,                 //通道个数
                                       512,               //单通道采样个数
                                       AV_SAMPLE_FMT_S16, //采样格式
                                       0);
    //read data from device
    //read data from device
    while((ret = av_read_frame(fmt_ctx, &pkt)) == 0 &&
          rec_status) {
        
        av_log(NULL, AV_LOG_INFO,
               "packet size is %d(%p)\n",
               pkt.size, pkt.data);
        
        //进行内存拷贝，按字节拷贝的
        memcpy((void*)src_data[0], (void*)pkt.data, pkt.size);
        
        //重采样
        swr_convert(swr_ctx,                    //重采样的上下文
                    dst_data,                   //输出结果缓冲区
                    512,                        //每个通道的采样数
                    (const uint8_t **)src_data, //输入缓冲区
                    512);                       //输入单个通道的采样数
        
        //write file
        //fwrite(pkt.data, 1, pkt.size, outfile);
        fwrite(dst_data[0], 1, dst_linesize, outfile);
        fflush(outfile);
        av_packet_unref(&pkt); //release pkt
    }
    ```

4. 清理：

    ```cpp
    //释放输入输出缓冲区
    if(src_data){
        av_freep(&src_data[0]);
    }
    av_freep(&src_data);
    
    if(dst_data){
        av_freep(&dst_data[0]);
    }
    av_freep(&dst_data);
    
    //释放重采样的上下文
    swr_free(&swr_ctx);
    ```

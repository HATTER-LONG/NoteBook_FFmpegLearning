#include "03_FFmpegReadYUMBaseMac.h"

#include <string.h>

static int rec_status = 0;

void set_status(int status)
{
    rec_status = status;
}

/**
 * @brief open audio device
 * @return succ: AVFormatContext*, fail: NULL
 */
static AVFormatContext* open_dev()
{

    int ret = 0;
    char errors[1024] = {
        0,
    };

    // ctx
    AVFormatContext* fmt_ctx = NULL;
    AVDictionary* options = NULL;

    //[[video device]:[audio device]]
    // mac 有两个默认相机 0：机器本身相机 1：桌面
    // char* devicename = "0"
    char* devicename = "/Users/caolei/Desktop/test/out.yuv";

    // ffmpeg -s 1920*1080 out.yum
    // key: video_size == -s
    //      framerate 帧率
    // value: 1920x1080
    av_dict_set(&options, "video_size", "1920x1080", 0);
    av_dict_set(&options, "framerate", "30", 0);
    av_dict_set(&options, "pixel_format", "yuv422p", 0);
    // get format
    AVInputFormat* iformat = NULL;   // 从文件读取不需要输入设备 av_find_input_format("avfoundation");

    // open device
    if ((ret = avformat_open_input(&fmt_ctx, devicename, iformat, &options)) < 0)
    {
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to open audio device, [%d]%s\n", ret, errors);
        return NULL;
    }

    return fmt_ctx;
}

/**
 */
static void read_data_and_encode(AVFormatContext* fmt_ctx, FILE* outfile)
{
    int ret = 0;

    // pakcet
    AVPacket pkt;
    av_init_packet(&pkt);
    // read data from device
    while ((ret = av_read_frame(fmt_ctx, &pkt)) == 0 && rec_status)
    {
        // write file
        //（宽 x 高）x (位深 yuv422 = 2/ yuv420 = 1.5/ yuv444 = 3)
        fwrite(pkt.data, 1, 4147200, outfile);   // 614400 分辨率计算码率  640*480* 2(yuv422)
        fflush(outfile);

        // release pkt
        av_packet_unref(&pkt);
    }
    if (ret < 0)
    {
        char errors[1024] = { 0 };
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to av_read_frame, [%d]%s\n", ret, errors);
    }
}

// void rec_audio()
void rec_video()
{
    // context
    AVFormatContext* fmt_ctx = NULL;

    // set log level
    av_log_set_level(AV_LOG_DEBUG);

    // register audio device
    avdevice_register_all();

    // start record
    rec_status = 1;

    // create file
    // char *out = "./audio.pcm";
    // char* out = "./audio.aac";
    char* out = "./record.yuv";
    FILE* outfile = fopen(out, "wb+");
    if (!outfile)
    {
        printf("Error, Failed to open file!\n");
        goto __ERROR;
    }

    //打开设备
    fmt_ctx = open_dev();
    if (!fmt_ctx)
    {
        printf("Error, Failed to open device!\n");
        goto __ERROR;
    }

    // encode
    read_data_and_encode(fmt_ctx, outfile);

__ERROR:

    // close device and release ctx
    if (fmt_ctx)
    {
        avformat_close_input(&fmt_ctx);
    }

    if (outfile)
    {
        // close file
        fclose(outfile);
    }

    av_log(NULL, AV_LOG_DEBUG, "finish!\n");

    return;
}
int anotherMethod()
{
    // rec_video();
    AVFormatContext* pFormatCtx;
    int i, videoindex;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;

    avdevice_register_all();
    // avformat_network_init();
    // Register Device
    // show_dshow_device();
    pFormatCtx = avformat_alloc_context();

    AVInputFormat* ifmt = av_find_input_format("avfoundation");
    if (avformat_open_input(&pFormatCtx, "0", ifmt, NULL) != 0)
    {
        printf("Couldn't open input stream.\n");
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        printf("Couldn't find stream information.\n");
        return -1;
    }
    videoindex = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoindex = i;
        }
    }
    if (videoindex == -1)
    {
        printf("Couldn't find a video stream.\n");
        return -1;
    }
    pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL)
    {
        printf("Codec not found.\n");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        printf("Could not open codec.\n");
        return -1;
    }
    AVFrame *pFrame, *pFrameYUV;
    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();
    uint8_t* out_buffer =
        (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    avpicture_fill(
        (AVPicture*)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
    int ret, got_picture;
    AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));
    FILE* fp_yuv = fopen("output.yuv", "wb");
    struct SwsContext* img_convert_ctx;
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
        pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    ///这里打印出视频的宽高
    fprintf(stderr, "w= %d h= %d\n", pCodecCtx->width, pCodecCtx->height);
    ///我们就读取1000张图像
    for (int i = 0; i < 1000; i++)
    {
        if ((ret = av_read_frame(pFormatCtx, packet)) < 0)
        {
            char errors[1024] = { 0 };
            av_strerror(ret, errors, 1024);
            fprintf(stderr, "Failed to open audio device, [%d]%s\n", ret, errors);
            break;
        }
        if (packet->stream_index == videoindex)
        {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0)
            {
                printf("Decode Error.\n");
                return -1;
            }
            if (got_picture)
            {
                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
                    pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
                int y_size = pCodecCtx->width * pCodecCtx->height;
                fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);       // Y
                fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);   // U
                fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);   // V
            }
        }
        av_free_packet(packet);
    }
    sws_freeContext(img_convert_ctx);
    fclose(fp_yuv);
    av_free(out_buffer);
    av_free(pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}

#if 1
int main(int argc, char* argv[])
{
    rec_video();
    return 0;
}
#endif

#include "04_FFmpegReadNV12ToYum420BaseMac.h"

#include <stdlib.h>
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
    char* devicename = "/Users/caolei/Desktop/test/nv12out.yuv";

    // ffmpeg -s 1920*1080 out.yum
    // key: video_size == -s
    //      framerate 帧率
    // value: 1920x1080
    av_dict_set(&options, "video_size", "1920x1080", 0);
    av_dict_set(&options, "framerate", "30", 0);
    av_dict_set(&options, "pixel_format", "nv12", 0);
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
        fwrite(pkt.data, 1, 3110400, outfile);   // 4147200 分辨率计算码率  1920*1080* 2(yuv422)
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

/**
 *   @brief xxx
 *   @param[int] xxx
 */
static void open_encoder(int width, int height, AVCodecContext** enc_ctx)
{
    AVCodec* codec = NULL;
    codec = avcodec_find_encoder_by_name("libx264");
    if (!codec)
    {
        printf("Codec libx264 not found\n");
        exit(1);
    }

    *enc_ctx = avcodec_alloc_context3(codec);

    if (!*enc_ctx)
    {
        printf("Could not allocate vide codec context!\n");
        exit(1);
    }

    // SPS/PPS
    (*enc_ctx)->profile = FF_PROFILE_H264_HIGH_444;
    (*enc_ctx)->level = 50;   // 标识 Level 为 5.0

    // 设置分辨率
    (*enc_ctx)->width = width;
    (*enc_ctx)->height = height;

    // GOP 设置
    (*enc_ctx)->gop_size = 250;
    (*enc_ctx)->keyint_min = 25;   //可选 最小插入I帧间隔，如果在 GOP 过程中如果图像由突变会自动插入 I 帧

    // 设置 B 帧数量
    (*enc_ctx)->max_b_frames = 3;   //可选
    (*enc_ctx)->has_b_frames = 1;   //可选

    // 设置参考帧数量
    (*enc_ctx)->refs = 3;   //可选

    //设置输入 YUV 编码格式 libx264 要求输入为 420P
    (*enc_ctx)->pix_fmt = AV_PIX_FMT_YUV420P;

    //设置码率
    (*enc_ctx)->bit_rate = 3150000;   // 3150 Kbps

    //设置帧率
    (*enc_ctx)->time_base = (AVRational) { 1, 25 };   // 帧与帧之间的间隔是 timebase
    (*enc_ctx)->framerate = (AVRational) { 25, 1 };   // 帧率，每秒 25 帧与 timebase 成倒数关系

    int ret = avcodec_open2((*enc_ctx), codec, NULL);
    if (ret != 0)
    {
        char errors[1024] = {
            0,
        };
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to open audio device, [%d]%s\n", ret, errors);
        exit(1);
    }
}

// void rec_audio()
void rec_video()
{
    // context
    AVFormatContext* fmt_ctx = NULL;
    AVCodecContext* enc_ctx = NULL;
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
    //宽高与输入信息匹配
    open_encoder(1920, 1080, &enc_ctx);

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

#if 1
int main(int argc, char* argv[])
{
    rec_video();
    return 0;
}
#endif

#include "04_FFmpegReadNV12ToYum420BaseMac.h"

#include <stdlib.h>
#include <string.h>

#define WIDTH 1920
#define HEIGHT 1080
#define WIDTHXHEIGHT "1920x1080"

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
    av_dict_set(&options, "video_size", WIDTHXHEIGHT, 0);
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

static AVFrame* create_frame(int width, int height)
{
    AVFrame* frame = NULL;
    frame = av_frame_alloc();

    char errors[1024] = {
        0,
    };
    int ret = 0;
    if (!frame)
    {
        printf("Error, No Memory!\n");
        goto __ERROR;
    }
    frame->width = width;
    frame->height = height;
    frame->format = AV_PIX_FMT_YUV420P;

    // alloc inner memory
    ret = av_frame_get_buffer(frame, 32);   // 按 32 位对齐
    if (ret < 0)
    {
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to open audio device, [%d]%s\n", ret, errors);
        goto __ERROR;
    }
    return frame;
__ERROR:
    if (frame)
    {
        av_frame_free(&frame);
    }
    return NULL;
}

static void encode(AVCodecContext* enc_ctx, AVFrame* frame, AVPacket* newpkt, FILE* outfile)
{
    int ret = 0;
    if (frame)
    {
        long long ptsTmp = frame->pts;
        printf("send frame to encoder, pts = %lld\n", ptsTmp);
    }
    // 送原始数据进入编码器
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0)
    {
        printf("Error, send a frame for encoding failed\n");
        exit(1);
    }

    // 从编码器获取编码好的数据
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(enc_ctx, newpkt);
        //如果编码器数据不足时会返回 EAGAIN，或者数据尾时会返回 AVERROR_EOF
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return;
        }
        if (ret < 0)
        {
            printf("Error, failed to encode\n");
            exit(1);
        }
        if (ret != 0)
        {
            char errors[1024] = {
                0,
            };
            av_strerror(ret, errors, 1024);
            fprintf(stderr, "Failed to open audio device, [%d]%s\n", ret, errors);
        }
        fwrite(newpkt->data, 1, newpkt->size, outfile);
        av_packet_unref(newpkt);   // 注意释放资源
    }
}

/**
 */
static void read_data_and_encode(
    AVFormatContext* fmt_ctx, AVCodecContext* enc_ctx, FILE* yuvoutfile, FILE* h264outfile)
{
    int ret = 0;
    int base = 0;
    // pakcet
    AVPacket pkt;
    AVFrame* frame = NULL;
    AVPacket* newpkt = NULL;
    av_init_packet(&pkt);

    //创建 AVFrame
    frame = create_frame(WIDTH, HEIGHT);
    if (!frame)
    {
        printf("ERROR, Failed to create frame!\n");
        goto __ERROR;
    }

    newpkt = av_packet_alloc();

    if (!newpkt)
    {
        printf("ERROR, Failed to alloc packet!\n");
        goto __ERROR;
    }
    av_init_packet(newpkt);

    // read data from device
    while (rec_status && (ret = av_read_frame(fmt_ctx, &pkt)) == 0)
    {
        av_log(NULL, AV_LOG_INFO, "packet size is %d(%p)\n", pkt.size, (void*)pkt.data);

        // write file
        //（宽 x 高）x (位深 yuv422 = 2/ yuv420 = 1.5/ yuv444 = 3)

        // YYYYYYYYUVUV   NV12
        // YYYYYYYYUUVV  YUV420
        // 转换  先将 NV12 的 Y 数据拷贝到 data[0] 中
        // 1920 x 1080 = 2073600 Y 数据长度
        memcpy(frame->data[0], pkt.data, 2073600);   // copy Y data

        // UV
        // 2073600 / 4 为 UV 分辨率
        // 2073600之后是 UV 数据
        for (int i = 0; i < 2073600 / 4; i++)
        {
            frame->data[1][i] = pkt.data[2073600 + i * 2];
            frame->data[2][i] = pkt.data[2073600 + i * 2 + 1];
        }
        fwrite(frame->data[0], 1, 2073600, yuvoutfile);
        fwrite(frame->data[1], 1, 2073600 / 4, yuvoutfile);
        fwrite(frame->data[2], 1, 2073600 / 4, yuvoutfile);

        // 更新 pts  h264 要求 pts 每帧连续
        frame->pts = base++;
        encode(enc_ctx, frame, newpkt, h264outfile);
        // release pkt
        av_packet_unref(&pkt);
    }
    encode(enc_ctx, NULL, newpkt, h264outfile);   // 给编码器传入 NULL ,使得编码器情况缓冲区

    if (ret < 0)
    {
        char errors[1024] = { 0 };
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to av_read_frame, [%d]%s\n", ret, errors);
    }
    fflush(yuvoutfile);
    fflush(h264outfile);
__ERROR:
    if (newpkt)
    {
        av_packet_free(&newpkt);
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
    FILE* yuvoutfile = NULL;
    FILE* h264outfile = NULL;
    // create file
    // char *out = "./audio.pcm";
    // char* out = "./audio.aac";
    char* yuvout = "./record.yuv";
    yuvoutfile = fopen(yuvout, "wb+");
    if (!yuvoutfile)
    {
        printf("Error, Failed to open file yuvoutfile!\n");
        goto __ERROR;
    }
    char* h264out = "./record.h264";
    h264outfile = fopen(h264out, "wb+");
    if (!h264outfile)
    {
        printf("Error, Failed to open file h264outfile!\n");
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
    open_encoder(WIDTH, HEIGHT, &enc_ctx);

    // encode
    read_data_and_encode(fmt_ctx, enc_ctx, yuvoutfile, h264outfile);

__ERROR:

    // close device and release ctx
    if (fmt_ctx)
    {
        avformat_close_input(&fmt_ctx);
    }

    if (yuvoutfile)
    {
        // close file
        fclose(yuvoutfile);
    }
    if (h264outfile)
    {
        // close file
        fclose(h264outfile);
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

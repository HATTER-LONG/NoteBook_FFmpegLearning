#include "05_PushStream.h"

#include <librtmp/rtmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
void set_status(int status)
{
    return;
}

/**
 * flv 头一共有 9 个字节
 * 1-3  signature: 'F' 'L' 'V'
 * 4    Version: 1
 * 5    0-5位 保留 必须是 0
 * 5     6 位 是否有音频 Tag
 * 5     7 位 保留 必须是 0
 * 5     8 位 是否有视频 Tag
 * 6-9  Header 的大小 必须是 9
 */
static FILE* open_flv(char* FlvName)
{
    FILE* fp = NULL;
    fp = fopen(FlvName, "rb");
    if (!fp)
    {
        printf("Failed to open flv:%s\n", FlvName);
        return NULL;
    }

    // 跳过头的 9 字节
    fseek(fp, 9, SEEK_SET);

    // 跳过头后边第一个 pre_tagsize 4字节  SEEK_SET 是从文件头跳
    fseek(fp, 4, SEEK_CUR);

    return fp;
}


static RTMP* connect_rtmp_server(char* RtmpAddr)
{
    RTMP* rtmp = NULL;

    // 1. 创建 RTMP 对象，并进行初始化
    rtmp = RTMP_Alloc();
    if (!rtmp)
    {
        printf("Failed to alloc rtmp object\n");
        goto __ERROR;
    }
    RTMP_Init(rtmp);

    // 2. 先设置 RTMP 服务器地址，以及设置链接超时事件
    RTMP_SetupURL(rtmp, RtmpAddr);
    rtmp->Link.timeout = 10;

    // 3. 设置推流/拉流属性，设置该开关推流 未设置拉流
    RTMP_EnableWrite(rtmp);

    // 4. 建立链接
    if (RTMP_Connect(rtmp, NULL) != 0)
    {
        printf("Failed to Connect RTMP Server!\n");
        goto __ERROR;
    }

    // 5. 创建流
    if (RTMP_ConnectStream(rtmp, 0) != 0)
    {
        printf("Failed to Connect Stream With RTMP Server!\n");
        goto __ERROR;
    }

    return rtmp;
__ERROR:
    if (rtmp)
    {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
    return NULL;
}

static RTMPPacket* alloc_packet()
{
    RTMPPacket* packet = NULL;
    int ret = 0;
    packet = (RTMPPacket*)malloc(sizeof(RTMPPacket));
    if (!packet)
    {
        printf("No Memory, Failed to alloc RTMPPacket!\n");
        return NULL;
    }
    ret = RTMPPacket_Alloc(packet, 64 * 1024);
    if (!ret)
    {
        free(packet);
        packet = NULL;
        return packet;
    }
    RTMPPacket_Reset(packet);

    packet->m_hasAbsTimestamp = 0;
    packet->m_nChannel = 0x4;

    return packet;
}

static void read_u8(FILE* fp, u_int* u8)
{
    fread(u8, 1, 1, fp);
    return;
}

/**
 *  内存：.... | x1 | x2  | x3  | .........
 *               读取 ↑
 *  文件：.... x1, x2, x3, x4,.........
 *
 *  小->大端转换：.....|x3 | x2 | x1 | ......
 *  注意大段小段区别
 */
static void read_u24(FILE* fp, u_int* u24)
{
    u_int tmp = 0;

    fread(&tmp, 1, 3, fp);
    // uint xxx = ((tmp << 16) & 0xFF0000);
    // uint xxxx = ((tmp & 0xFF00));
    // printf("tmp  = %x 1 = %x 2 = %x 3= %x\n", tmp, (tmp >> 16) & 0xFF, xxx, xxxx);

    *u24 = (((tmp >> 16) & 0xFF) | ((tmp << 16) & 0xFF0000) | (tmp & 0xFF00));
    // printf("u24  = %x\n", *u24);
}

static void read_u32(FILE* fp, u_int* u32)
{
    u_int tmp = 0;
    fread(&tmp, 1, 4, fp);
    *u32 = (((tmp >> 24) & 0xFF) | ((tmp << 24) & 0xFF000000) | ((tmp >> 16) & 0xFF00) |
            ((tmp << 16) & 0xFF0000));
}

void show_bytes(char* start, int len)
{
    int i;
    for (i = 0; i < len; i++)
        printf(" %.2x", start[i]);   // line:data:show_bytes_printf
    printf("\n");
}

/**
 * @brief
 *
 * @param fp[in] flv file
 * @param packet[out] data from flv
 * @return int 0 success; -1 failed
 */
static int read_data(FILE* fp, RTMPPacket** packet)
{
    /**
     * tag header
     *  第一个字节 TT (Tag Type), 0x8 音频 0x09 视频  0x12 script
     *  2-4, Tag body 长度 == PreTagSize - Tag Header size
     *  5-7, 时间戳 单位毫秒；script 时间戳为 0
     *  8, 扩展时间戳，5-7 为 0xFFFFFF 时使用扩展。时间戳结构 [扩展，时间戳] 一共 4 字节。
     *  9, streamID，0
     */
    int ret = -1;
    u_int tagType = 0;
    u_int tag_data_size = 0;
    u_int ts = 0;
    u_int streamID = 0;
    u_int readSize = 0;
    read_u8(fp, &tagType);
    read_u24(fp, &tag_data_size);
    read_u32(fp, &ts);
    read_u24(fp, &streamID);
    printf("tag header, ts: %u, tt:%d, datasize:%d\n", ts, tagType, tag_data_size);

    readSize = fread((*packet)->m_body, 1, tag_data_size, fp);
    if (tag_data_size != readSize)
    {
        printf("Error fread data tag_data_size[%u] readSize[%u]\n", tag_data_size, readSize);
        return ret;
    }
    //跳过 PreviousTagSize
    fseek(fp, 4, SEEK_CUR);
    // 控制 Basic Header 长度
    (*packet)->m_headerType = RTMP_PACKET_SIZE_LARGE;
    // 时间戳
    (*packet)->m_nTimeStamp = ts;
    // 数据类型 音频or视频
    (*packet)->m_packetType = tagType;
    // 数据长度
    (*packet)->m_nBodySize = tag_data_size;

    ret = 0;
    return ret;
}

//向流媒体服务器推流
static void send_data(FILE* fp, RTMP* rtmp)
{
    // 1. 分配 RTMPPacket
    RTMPPacket* packet = NULL;
    packet = alloc_packet();
    unsigned int pre_ps = 0;
    while (packet)
    {
        // 2. 从 flv 文件中获取数据
        int ret = read_data(fp, &packet);
        if (ret == -1)
        {
            break;
        }
        // 3. 判断 RTMP 链接是否正常
        if (!RTMP_IsConnected(rtmp))
        {
            printf("ERROR Disconnect.....\n");
            break;
        }

        // 4. 发送数据
        RTMP_SendPacket(rtmp, packet, 0);

        // 粗略解决 数据发送过块
        usleep(packet->m_nTimeStamp - pre_ps);
        pre_ps = packet->m_nTimeStamp;
        RTMPPacket_Reset(packet);
    }
    // release RTMP packet
    RTMPPacket_Free(packet);
    free(packet);
    return;
}

void PushStream()
{
    char* flvFile = "/mnt/f/MyVideo/Test.flv";
    char* rtmpAddr = "rtmp://localhost/live/home";

    // 1. 读 flv 文件
    FILE* fp = open_flv(flvFile);

    // 2. 链接 RTMP 服务器

    RTMP* rtmp = connect_rtmp_server(rtmpAddr);

    // 3. Push AV Data 到 RTMP 服务器
    send_data(fp, rtmp);
    return;
}

int main(void)
{
    PushStream();
    return 0;
}

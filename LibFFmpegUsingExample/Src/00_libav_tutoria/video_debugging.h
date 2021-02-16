#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavformat/avformat.h>

#ifdef __cplusplus
}   // endif extern "C"
#endif

void logging(const char* fmt, ...);
void log_packet(const AVFormatContext* fmt_ctx, const AVPacket* pkt);
void print_timing(char* name, AVFormatContext* avf, AVCodecContext* avc, AVStream* avs);
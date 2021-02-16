#pragma once

#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"

#include <stdio.h>

void set_status(int status);
void rec_audio(void);

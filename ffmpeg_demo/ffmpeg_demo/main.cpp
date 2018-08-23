/*
 * Copyright (c) 2001 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * video decoding with libavcodec API example
 *
 * @example decode_video.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "ffmpeg_tool.h"
#include "mediadata_h264.h"
/*
av_seek_frame 参数flag为0，则返回当前时间后面的第一个关键帧,如果后面没有关键帧则返回-1
单AVSEEK_FLAG_BACKWARD参数，则返回前面的一个关键帧
AVSEEK_FLAG_ANY参数代表任意帧
一般填个AVSEEK_FLAG_BACKWARD就代表向前找关键帧就够了
*/







int main(int argc, char **argv)
{

    //simplest_yuv420_split("one.mp4.yuv",1920,1080,1);
    //simplest_yuv420_halfy("one.mp4.yuv",848,480,1);
    //simplest_yuv420_gray("one.mp4.yuv",848,480,1);
    //simplest_yuv420_border("one.mp4.yuv",848,480,20,1);
    //simplest_yuv420_graybar(848, 480,0,255,10,"1509340580.flv.yuv");
    //simplest_rgb24_split("cie1931_500x500.rgb", 500, 500,1);
    //simplest_rgb24_to_bmp("cie1931_500x500.rgb",500,500,"output_lena.bmp");
    //simplest_yuv420_to_rgb24("one.mp4.yuv",1920,1080,100,"one.mp4.rgb");
    //simplest_rgb24_to_yuv420("lena_256x256_rgb24.rgb",256,256,1,"output_lena.yuv");
    //ffmpeg_format_transform("one.mp4.yuv",AV_PIX_FMT_YUV420P,1920,1080,"one.mp4.rgb",AV_PIX_FMT_YUYV422,352,288);

    //return change(argc,argv);

    //ffmpeg_media_transform("029.ts","029.mp4");
    ffmpeg_media_decorde("short.mp4");

    getchar();
}

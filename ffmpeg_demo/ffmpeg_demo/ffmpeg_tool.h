
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "util.h"

extern "C"
{
#include <libavcodec/avcodec.h>
//#include <libavcodec/h264_sei.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
}

int ffmpeg_format_transform(char*src,AVPixelFormat src_format,int src_w,int src_h,char *dst,AVPixelFormat dst_format,int dst_w,int dst_h);
int ffmpeg_media_demux(char*src_url,char *dst_video,char *dst_audio);
int ffmpeg_media_mux(char*src_v,char *src_a,char *dst);
int ffmpeg_media_mux_single(char*in_filename,char *out_filename);
int ffmpeg_media_decorde(char*src_url);
int ffmpeg_media_transform(char*src_url,char*dst_url);
int ffmpeg_media_push(char *url);
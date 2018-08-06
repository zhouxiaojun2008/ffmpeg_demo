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

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/avutil.h>
    #include <libavutil/imgutils.h>
}

#define SAVE_FRAME_INFO
#define SAVE_RAW
#define SAVE_YUV

#define INBUF_SIZE 4096

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename)
{
    FILE *f;
    int i;

    f = fopen(filename,"w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}

static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt,
                   const char *filename)
{
    char buf[1024];
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }

        printf("saving frame %3d\n", dec_ctx->frame_number);
        fflush(stdout);

        /* the picture is allocated by the decoder. no need to
           free it */
        _snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);
        pgm_save(frame->data[0], frame->linesize[0],
                 frame->width, frame->height, buf);
    }
}

void print_error(const char* name, int err)
{
    char errbuf[128];
    const char *errbuf_ptr = errbuf;

    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
        errbuf_ptr = strerror(AVUNERROR(err));
    av_log(NULL, AV_LOG_ERROR, "%s: %s %d\n", name, errbuf_ptr,err);
}

int change(int argc, char **argv);
/*
av_seek_frame 参数flag为0，则返回当前时间后面的第一个关键帧,如果后面没有关键帧则返回-1
单AVSEEK_FLAG_BACKWARD参数，则返回前面的一个关键帧
AVSEEK_FLAG_ANY参数代表任意帧
一般填个AVSEEK_FLAG_BACKWARD就代表向前找关键帧就够了
*/


int simplest_yuv420_split(char *url, int w, int h,int num);
int simplest_yuv420_halfy(char *url, int w, int h,int num);
int simplest_yuv420_gray(char *url, int w, int h,int num);
int simplest_yuv420_border(char *url, int w, int h,int border,int num);
int simplest_yuv420_graybar(int width, int height,int ymin,int ymax,int barnum,char *url_out);
int simplest_rgb24_split(char *url, int w, int h,int num);
int simplest_rgb24_to_bmp(const char *rgb24path,int width,int height,const char *bmppath);
int simplest_rgb24_to_yuv420(char *url_in, int w, int h,int num,char *url_out);
int simplest_yuv420_to_rgb24(char *url_in, int w, int h,int num,char *url_out);

int main(int argc, char **argv)
{
    simplest_yuv420_split("1509340580.flv.yuv",848,480,1);
    simplest_yuv420_halfy("1509340580.flv.yuv",848,480,1);
    simplest_yuv420_gray("1509340580.flv.yuv",848,480,1);
    simplest_yuv420_border("1509340580.flv.yuv",848,480,20,1);
    simplest_yuv420_graybar(848, 480,0,255,10,"1509340580.flv.yuv");
    simplest_rgb24_split("cie1931_500x500.rgb", 500, 500,1);
    simplest_rgb24_to_bmp("cie1931_500x500.rgb",500,500,"output_lena.bmp");
    simplest_yuv420_to_rgb24("short.mp4.yuv",1280,720,500,"short.mp4.rgb");
    //simplest_rgb24_to_yuv420("lena_256x256_rgb24.rgb",256,256,1,"output_lena.yuv");

    //return change(argc,argv);


    char *url = "short.mp4";
    av_register_all();
    int err = 0;

    AVFormatContext *pFormatContext = NULL;
    err = avformat_open_input(&pFormatContext,url,NULL,NULL);
    if ( err != 0) {
        print_error("avformat_open_input",err);
        return -1;
    }


    err = avformat_find_stream_info(pFormatContext,NULL);
    if ( err != 0) {
        print_error("avformat_find_stream_info",err);
        return -1;
    }
    
    av_dump_format(pFormatContext,0,url,0);

    int video_index = -1;
    int audio_index = -1;
    for (int i = 0; i < pFormatContext->nb_streams; i ++) {
        if (pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
        }else if (pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            
            audio_index = i;
        }
    }

    if (video_index == -1) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // Codec not found
    }

    AVCodecContext *pCodecCtxOrig = pFormatContext->streams[video_index]->codec;

    AVCodec *pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
    if(pCodec==NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // Codec not found
    }

    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
    
    avcodec_parameters_to_context(pCodecCtx, pFormatContext->streams[video_index]->codecpar);
   
    err = avcodec_open2(pCodecCtx,pCodec,NULL);
    if ( err != 0) {
        print_error("avcodec_open2",err);
        return -1;
    }

    AVFrame *pFrame = av_frame_alloc();
    AVFrame *pFrameRGB = av_frame_alloc();
    if (pFrame == NULL || pFrameRGB == NULL) {
        fprintf(stderr, "Couldn't alloc frame");
        return -1; // Error copying codec context
    }

    int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24,pCodecCtx->width,pCodecCtx->height);
    char *buffer = (char*)av_malloc_array(numBytes,1);
    
    char logname[64] = "";

#ifdef SAVE_RAW
    memset(logname,0x00,sizeof(logname));
    strcat(logname,url);
    strcat(logname,".264");
    FILE *pf_264 = fopen(logname,"wb");
#endif

#ifdef SAVE_YUV
    memset(logname,0x00,sizeof(logname));
    strcat(logname,url);
    strcat(logname,".yuv");
    FILE *pf_yuv = fopen(logname,"wb");
    int frame_seq = 1;
#endif

    
#ifdef SAVE_FRAME_INFO
    memset(logname,0x00,sizeof(logname));
    strcat(logname,url);
    strcat(logname,".txt");
    FILE *pf_frame = fopen(logname,"w+");
    int packet_seq = 1;
#endif

    AVPacket *pkt = av_packet_alloc();
    int count = 1;
    
    int got_picture_ptr = 0;
    bool is_over = false;
    AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");

    char* buf = new char[pCodecCtx->height * pCodecCtx->width * 3 / 2];
    memset(buf, 0, pCodecCtx->height * pCodecCtx->width * 3 / 2);
    int height = pCodecCtx->height;
    int width = pCodecCtx->width;
    int ret = -1;

    while (1)
    {
        err = av_read_frame(pFormatContext,pkt);
        if (err == 0)
        {
            if (pkt->stream_index == video_index) 
            { //packet pts dts的值是通过pFormatContext->streams[video_index]->time_base的单位，实际上就是mp4 track box里面的timescale

#ifdef SAVE_FRAME_INFO
                fprintf(pf_frame,"seq: %-6d,video packet,"
                    "dts: %-8lld,dts-time: %-8lld, pts: %-8lld, pts-time:%-8lld,size: %-6d, flags: %-2d, duration: %lld,duration-time: %lld.\n",
                    packet_seq++,
                    pkt->dts,
                    av_rescale_q(pkt->dts,pFormatContext->streams[video_index]->time_base,av_make_q(1, 1000)),
                    pkt->pts,
                    av_rescale_q(pkt->pts,pFormatContext->streams[video_index]->time_base,av_make_q(1, 1000)),
                    pkt->size,
                    pkt->flags,
                    pkt->duration,
                    av_rescale_q(pkt->duration,pFormatContext->streams[video_index]->time_base,av_make_q(1, 1000)));
#else 
                if (pkt->flags == 1)
                {
                    fprintf(pf_frame,"seq: %-6d,video packet,"
                        "dts: %-8lld,dts-time: %-8lld, pts: %-8lld, pts-time:%-8lld,size: %-6d, flags: %-2d, duration: %lld,duration-time: %lld.\n",
                        packet_seq++,
                        pkt->dts,
                        av_rescale_q(pkt->dts,pFormatContext->streams[video_index]->time_base,av_make_q(1, 1000)),
                        pkt->pts,
                        av_rescale_q(pkt->pts,pFormatContext->streams[video_index]->time_base,av_make_q(1, 1000)),
                        pkt->size,
                        pkt->flags,
                        pkt->duration,
                        av_rescale_q(pkt->duration,pFormatContext->streams[video_index]->time_base,av_make_q(1, 1000)));
                }
#endif

#ifdef SAVE_RAW
                av_bitstream_filter_filter(h264bsfc, pFormatContext->streams[video_index]->codec, NULL, &pkt->data, &pkt->size, pkt->data, pkt->size, 0);
                fwrite(pkt->data,1,pkt->size,pf_264);
#endif

#if 0
                avcodec_decode_video2(pCodecCtx,pFrame, &got_picture_ptr,pkt); 
                if (got_picture_ptr)
                {
#ifdef SAVE_YUV
                    fprintf(pf_frame,"seq: %-6d,video frame,"
                        "pts: %-8lld,frame dts: %-8lld, frame pts: %-8lld, flags: %-2d, frame duration: %lld\n",
                        frame_seq++,
                        pFrame->pts,
                        pFrame->pkt_dts,
                        pFrame->pkt_pts,
                        pFrame->flags,
                        pFrame->pkt_duration
                        );

                    int a = 0, i;
                    for (i = 0; i<height; i++)
                    {
                        memcpy(buf + a, pFrame->data[0] + i * pFrame->linesize[0], width);
                        a += width;
                    }
                    for (i = 0; i<height / 2; i++)
                    {
                        memcpy(buf + a, pFrame->data[1] + i * pFrame->linesize[1], width / 2);
                        a += width / 2;
                    }
                    for (i = 0; i<height / 2; i++)
                    {
                        memcpy(buf + a, pFrame->data[2] + i * pFrame->linesize[2], width / 2);
                        a += width / 2;
                    }
                    fwrite(buf, 1, pCodecCtx->height * pCodecCtx->width * 3 / 2, pf_yuv);
#endif
#else
                ret = avcodec_send_packet(pCodecCtx,pkt);
                if (ret != 0)
                {
                    print_error("avcodec_send_packet",ret);
                    continue;
                }

                while (1)
                {
                    ret = avcodec_receive_frame(pCodecCtx,pFrame);
                    if (ret != 0)
                    {
                        break;
                    }
                    fprintf(pf_frame,"seq: %-6d,video frame,"
                        "pts: %-8lld,frame dts: %-8lld, frame pts: %-8lld, flags: %-2d, frame duration: %lld\n",
                        frame_seq++,
                        pFrame->pts,
                        pFrame->pkt_dts,
                        pFrame->pkt_pts,
                        pFrame->flags,
                        pFrame->pkt_duration
                        );

                    int a = 0, i;
                    for (i = 0; i<height; i++)
                    {
                        memcpy(buf + a, pFrame->data[0] + i * pFrame->linesize[0], width);
                        a += width;
                    }
                    for (i = 0; i<height / 2; i++)
                    {
                        memcpy(buf + a, pFrame->data[1] + i * pFrame->linesize[1], width / 2);
                        a += width / 2;
                    }
                    for (i = 0; i<height / 2; i++)
                    {
                        memcpy(buf + a, pFrame->data[2] + i * pFrame->linesize[2], width / 2);
                        a += width / 2;
                    }
                    fwrite(buf, 1, pCodecCtx->height * pCodecCtx->width * 3 / 2, pf_yuv);
                }
#endif
            }else if (pkt->stream_index == audio_index) {
#ifdef SAVE_FRAME_INFO
                fprintf(pf_frame,"seq: %-6d,audio packet,"
                    "dts: %-8lld,dts-time: %-8lld, pts: %-8lld, pts-time:%-8lld,size: %-6d, flags: %-2d, duration: %lld,duration-time: %lld.\n",
                    packet_seq++,
                    pkt->dts,
                    av_rescale_q(pkt->dts,pFormatContext->streams[video_index]->time_base,av_make_q(1, 1000)),
                    pkt->pts,
                    av_rescale_q(pkt->pts,pFormatContext->streams[video_index]->time_base,av_make_q(1, 1000)),
                    pkt->size,
                    pkt->flags,
                    pkt->duration,
                    av_rescale_q(pkt->duration,pFormatContext->streams[video_index]->time_base,av_make_q(1, 1000)));
#endif
            }
        }else{
            /*
            #define AVSEEK_FLAG_BACKWARD 1 ///< seek backward
            #define AVSEEK_FLAG_BYTE     2 ///< seeking based on position in bytes
            #define AVSEEK_FLAG_ANY      4 ///< seek to any frame, even non-keyframes
            #define AVSEEK_FLAG_FRAME    8 ///< seeking based on frame number
            */
            //if (is_over == true)
            print_error("av_read_frame",err);
            break;
            
            int64_t seektime = 10;
            err = av_seek_frame(pFormatContext,video_index,av_rescale_q(seektime,av_make_q(1, 1),pFormatContext->streams[video_index]->time_base),AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY); 
            if (err != 0)
            {
                print_error("av_seek_frame",err);
                break;
            }
      
            is_over = true;
            continue;
        }
        
        av_packet_unref(pkt);
    }

    av_free_packet(pkt);
    av_frame_free(&pFrame);
    av_bitstream_filter_close(h264bsfc);
    avcodec_close(pCodecCtx);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatContext);
    

#ifdef SAVE_RAW
    fclose(pf_264);
#endif

#ifdef SAVE_YUV
    fclose(pf_yuv);
#endif


#ifdef SAVE_FRAME_INFO
    fclose(pf_frame);
#endif

    getchar();
}

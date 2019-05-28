/*
* copyright (c) 2017 老衲不出家
*
* 2017-08-11
*
*/
#include "ffmpeg_tool.h"
#include <windows.h>

//路径处理函数
#include <Shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

void print_error(const char* name, int err)
{
    char errbuf[128];
    const char *errbuf_ptr = errbuf;

    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
        errbuf_ptr = strerror(AVUNERROR(err));
    av_log(NULL, AV_LOG_ERROR, "%s: %s %d\n", name, errbuf_ptr,err);
}

int ffmpeg_media_demux(char*in_filename,char *out_filename_v,char *out_filename_a)
{
    AVOutputFormat *ofmt_a = NULL,*ofmt_v = NULL;
    //（Input AVFormatContext and Output AVFormatContext）
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx_a = NULL, *ofmt_ctx_v = NULL;
    AVPacket pkt;
    int ret, i;
    int videoindex=-1,audioindex=-1;
    int frame_index=0;



    av_register_all();
    //Input
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        printf( "Could not open input file.");
        goto end;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        printf( "Failed to retrieve input stream information");
        goto end;
    }

    //Output
    avformat_alloc_output_context2(&ofmt_ctx_v, NULL, NULL, out_filename_v);
    if (!ofmt_ctx_v) {
        printf( "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    ofmt_v = ofmt_ctx_v->oformat;

    avformat_alloc_output_context2(&ofmt_ctx_a, NULL, NULL, out_filename_a);
    if (!ofmt_ctx_a) {
        printf( "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    ofmt_a = ofmt_ctx_a->oformat;

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        //Create output AVStream according to input AVStream
        AVFormatContext *ofmt_ctx;
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVStream *out_stream = NULL;

        if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            videoindex=i;
            out_stream=avformat_new_stream(ofmt_ctx_v, in_stream->codec->codec);
            ofmt_ctx=ofmt_ctx_v;
        }else if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
            audioindex=i;
            out_stream=avformat_new_stream(ofmt_ctx_a, in_stream->codec->codec);
            ofmt_ctx=ofmt_ctx_a;
        }else{
            break;
        }

        if (!out_stream) {
            printf( "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        //Copy the settings of AVCodecContext
        if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
            printf( "Failed to copy context from input to output stream codec context\n");
            goto end;
        }
        out_stream->codec->codec_tag = 0;

        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    //Dump Format------------------
    printf("\n==============Input Video=============\n");
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    printf("\n==============Output Video============\n");
    av_dump_format(ofmt_ctx_v, 0, out_filename_v, 1);
    printf("\n==============Output Audio============\n");
    av_dump_format(ofmt_ctx_a, 0, out_filename_a, 1);
    printf("\n======================================\n");
    //Open output file
    if (!(ofmt_v->flags & AVFMT_NOFILE)) {
        if (avio_open(&ofmt_ctx_v->pb, out_filename_v, AVIO_FLAG_WRITE) < 0) {
            printf( "Could not open output file '%s'", out_filename_v);
            goto end;
        }
    }

    if (!(ofmt_a->flags & AVFMT_NOFILE)) {
        if (avio_open(&ofmt_ctx_a->pb, out_filename_a, AVIO_FLAG_WRITE) < 0) {
            printf( "Could not open output file '%s'", out_filename_a);
            goto end;
        }
    }

    //Write file header
    if (avformat_write_header(ofmt_ctx_v, NULL) < 0) {
        printf( "Error occurred when opening video output file\n");
        goto end;
    }
    if (avformat_write_header(ofmt_ctx_a, NULL) < 0) {
        printf( "Error occurred when opening audio output file\n");
        goto end;
    }

#if USE_H264BSF
    AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb"); 
#endif

    while (1) {
        AVFormatContext *ofmt_ctx;
        AVStream *in_stream, *out_stream;
        //Get an AVPacket
        if (av_read_frame(ifmt_ctx, &pkt) < 0)
            break;
        in_stream  = ifmt_ctx->streams[pkt.stream_index];


        if(pkt.stream_index==videoindex){
            out_stream = ofmt_ctx_v->streams[0];
            ofmt_ctx=ofmt_ctx_v;
            printf("Write Video Packet. size:%d\tpts:%lld\n",pkt.size,pkt.pts);
#if USE_H264BSF
            av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
        }else if(pkt.stream_index==audioindex){
            out_stream = ofmt_ctx_a->streams[0];
            ofmt_ctx=ofmt_ctx_a;
            printf("Write Audio Packet. size:%d\tpts:%lld\n",pkt.size,pkt.pts);
        }else{
            continue;
        }


        //Convert PTS/DTS
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        pkt.stream_index=0;
        //Write
        if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
            printf( "Error muxing packet\n");
            break;
        }
        //printf("Write %8d frames to output file\n",frame_index);
        av_free_packet(&pkt);
        frame_index++;
    }

#if USE_H264BSF
    av_bitstream_filter_close(h264bsfc);  
#endif

    //Write file trailer
    av_write_trailer(ofmt_ctx_a);
    av_write_trailer(ofmt_ctx_v);
end:
    avformat_close_input(&ifmt_ctx);
    /* close output */
    if (ofmt_ctx_a && !(ofmt_a->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx_a->pb);

    if (ofmt_ctx_v && !(ofmt_v->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx_v->pb);

    avformat_free_context(ofmt_ctx_a);
    avformat_free_context(ofmt_ctx_v);


    if (ret < 0 && ret != AVERROR_EOF) {
        printf( "Error occurred.\n");
        return -1;
    }
    return 0;

}

int ffmpeg_media_decorde(char *url)
{    
#define SAVE_FRAME_INFO
#define SAVE_RAW
#define SAVE_YUV
#define SAVE_AUDIO
#define INBUF_SIZE 4096

    int err = 0;

    av_register_all();

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
   
    AVDictionary *thread_opt = NULL;

    //av_dict_set(&thread_opt, "threads", "2", 0);

    //av_dict_set(&thread_opt, "threads", "auto", 0); // Is this needed ?

    err = avcodec_open2(pCodecCtx,pCodec,&thread_opt);
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

#ifdef SAVE_AUDIO
    memset(logname,0x00,sizeof(logname));
    strcat(logname,url);
    strcat(logname,".mp3");
    FILE *pf_mp3 = fopen(logname,"w+");
    
#endif

    AVPacket *pkt = av_packet_alloc();
    int count = 1;
    
    int got_picture_ptr = 0;
    bool is_over = false;
    AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("hevc_mp4toannexb");

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
                int now = GetTickCount();
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
                        "pts: %-8lld,frame dts: %-8lld, frame pts: %-8lld, flags: %-2d, frame duration: %lld,elapse:%d\n",
                        frame_seq++,
                        pFrame->pts,
                        pFrame->pkt_dts,
                        pFrame->pkt_pts,
                        pFrame->flags,
                        pFrame->pkt_duration,
                        GetTickCount()-now
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
                fwrite(pkt->data,1,pkt->size,pf_mp3);
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

#ifdef SAVE_AUDIO
    fclose(pf_mp3);
#endif
    return 0;
}


int ffmpeg_format_transform(char*srcfile,AVPixelFormat src_pixfmt,int src_w,int src_h,char* dstfile,AVPixelFormat dst_pixfmt,int dst_w,int dst_h)
{
    FILE *src_file =fopen(srcfile, "rb");
    int src_buffer_size = av_image_get_buffer_size(src_pixfmt, src_w, src_h, 16);

    FILE *dst_file = fopen(dstfile, "wb");
    int dst_buffer_size = av_image_get_buffer_size(dst_pixfmt, dst_w, dst_h, 16);

    uint8_t *src_data[4];
    int src_linesize[4];

    uint8_t *dst_data[4];
    int dst_linesize[4];

    int rescale_method=SWS_BICUBIC;
    struct SwsContext *img_convert_ctx;
    uint8_t *temp_buffer=(uint8_t *)malloc(src_buffer_size);

    int frame_idx=0;
    int ret=0;
    ret= av_image_alloc(src_data, src_linesize,src_w, src_h, src_pixfmt, 1);
    if (ret< 0) {
        printf( "Could not allocate source image\n");
        return -1;
    }
    ret = av_image_alloc(dst_data, dst_linesize,dst_w, dst_h, dst_pixfmt, 1);
    if (ret< 0) {
        printf( "Could not allocate destination image\n");
        return -1;
    }

    img_convert_ctx =sws_alloc_context();
    //Show AVOption
    av_opt_show2(img_convert_ctx,stdout,AV_OPT_FLAG_VIDEO_PARAM,0);
    //Set Value
    av_opt_set_int(img_convert_ctx,"sws_flags",SWS_POINT|SWS_PRINT_INFO,0);
    av_opt_set_int(img_convert_ctx,"srcw",src_w,0);
    av_opt_set_int(img_convert_ctx,"srch",src_h,0);
    av_opt_set_int(img_convert_ctx,"src_format",src_pixfmt,0);
    //'0' for MPEG (Y:0-235);'1' for JPEG (Y:0-255)
    av_opt_set_int(img_convert_ctx,"src_range",1,0);
    av_opt_set_int(img_convert_ctx,"dstw",dst_w,0);
    av_opt_set_int(img_convert_ctx,"dsth",dst_h,0);
    av_opt_set_int(img_convert_ctx,"dst_format",dst_pixfmt,0);
    av_opt_set_int(img_convert_ctx,"dst_range",1,0);
    sws_init_context(img_convert_ctx,NULL,NULL);


    while(1)
    {
        if (fread(temp_buffer, 1, src_buffer_size, src_file) != src_buffer_size){
            break;
        }

        switch(src_pixfmt){
        case AV_PIX_FMT_GRAY8:
            {
                memcpy(src_data[0],temp_buffer,src_w*src_h);
                break;
            }
        case AV_PIX_FMT_YUV420P:
            {
                memcpy(src_data[0],temp_buffer,src_w*src_h);                    //Y
                memcpy(src_data[1],temp_buffer+src_w*src_h,src_w*src_h/4);      //U
                memcpy(src_data[2],temp_buffer+src_w*src_h*5/4,src_w*src_h/4);  //V
                break;
            }
        case AV_PIX_FMT_YUV422P:
            {
                memcpy(src_data[0],temp_buffer,src_w*src_h);                    //Y
                memcpy(src_data[1],temp_buffer+src_w*src_h,src_w*src_h/2);      //U
                memcpy(src_data[2],temp_buffer+src_w*src_h*3/2,src_w*src_h/2);  //V
                break;
            }
        case AV_PIX_FMT_YUV444P:
            {
                memcpy(src_data[0],temp_buffer,src_w*src_h);                    //Y
                memcpy(src_data[1],temp_buffer+src_w*src_h,src_w*src_h);        //U
                memcpy(src_data[2],temp_buffer+src_w*src_h*2,src_w*src_h);      //V
                break;
            }
        case AV_PIX_FMT_YUYV422:
            {
                memcpy(src_data[0],temp_buffer,src_w*src_h*2);                  //Packed
                break;
            }
        case AV_PIX_FMT_RGB24:
            {
                memcpy(src_data[0],temp_buffer,src_w*src_h*3);                  //Packed
                break;
            }
        default:
            {
                printf("Not Support Input Pixel Format.\n");
                break;
            }
        }

        sws_scale(img_convert_ctx, src_data, src_linesize, 0, src_h, dst_data, dst_linesize);
        printf("Finish process frame %5d\n",frame_idx);
        frame_idx++;

        switch(dst_pixfmt)
        {
        case AV_PIX_FMT_GRAY8:
            {
                my_write(dst_file,dst_w*dst_h,dst_data[0]);
                //fwrite(dst_data[0],1,dst_w*dst_h,dst_file);
                break;
            }
        case AV_PIX_FMT_YUV420P:
            {
                my_write(dst_file,dst_w*dst_h,dst_data[0]);
                my_write(dst_file,dst_w*dst_h/4,dst_data[1]);
                my_write(dst_file,dst_w*dst_h/4,dst_data[2]);

                //fwrite(dst_data[0],1,dst_w*dst_h,dst_file);                 //Y
                //fwrite(dst_data[1],1,dst_w*dst_h/4,dst_file);               //U
                //fwrite(dst_data[2],1,dst_w*dst_h/4,dst_file);               //V
                break;
            }
        case AV_PIX_FMT_YUV422P:
            {
                my_write(dst_file,dst_w*dst_h,dst_data[0]);
                my_write(dst_file,dst_w*dst_h/2,dst_data[1]);
                my_write(dst_file,dst_w*dst_h/2,dst_data[2]);

                //fwrite(dst_data[0],1,dst_w*dst_h,dst_file);					//Y
                //fwrite(dst_data[1],1,dst_w*dst_h/2,dst_file);				//U
                //fwrite(dst_data[2],1,dst_w*dst_h/2,dst_file);				//V
                break;
            }
        case AV_PIX_FMT_YUV444P:
            {
                my_write(dst_file,dst_w*dst_h,dst_data[0]);
                my_write(dst_file,dst_w*dst_h,dst_data[1]);
                my_write(dst_file,dst_w*dst_h,dst_data[2]);

                //fwrite(dst_data[0],1,dst_w*dst_h,dst_file);                 //Y
                //fwrite(dst_data[1],1,dst_w*dst_h,dst_file);                 //U
                //fwrite(dst_data[2],1,dst_w*dst_h,dst_file);                 //V
                break;
            }
        case AV_PIX_FMT_YUYV422:
            {
                my_write(dst_file,dst_w*dst_h*2,dst_data[0]);
                //fwrite(dst_data[0],1,dst_w*dst_h*2,dst_file);               //Packed
                break;
            }
        case AV_PIX_FMT_RGB24:
            {
                my_write(dst_file,dst_w*dst_h*3,dst_data[0]);
                //fwrite(dst_data[0],1,dst_w*dst_h*3,dst_file);               //Packed
                break;
            }
        default:
            {
                printf("Not Support Output Pixel Format.\n");
                break;
            }
        }
    }

    sws_freeContext(img_convert_ctx);

    free(temp_buffer);
    fclose(dst_file);
    av_freep(&src_data[0]);
    av_freep(&dst_data[0]);

    return 0;
}

int ffmpeg_media_mux(char*in_filename_v,char *in_filename_a,char *out_filename)
{
    AVOutputFormat *ofmt = NULL;
    //Input AVFormatContext and Output AVFormatContext
    AVFormatContext *ifmt_ctx_v = NULL, *ifmt_ctx_a = NULL,*ofmt_ctx = NULL;
    AVPacket pkt;
    int ret, i;
    int videoindex_v=-1,videoindex_out=-1;
    int audioindex_a=-1,audioindex_out=-1;
    int frame_index=0;
    int64_t cur_pts_v=0,cur_pts_a=0;
    
    av_register_all();
    //Input
    if ((ret = avformat_open_input(&ifmt_ctx_v, in_filename_v, 0, 0)) < 0) {
        printf( "Could not open input file.");
        goto end;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx_v, 0)) < 0) {
        printf( "Failed to retrieve input stream information");
        goto end;
    }

    if ((ret = avformat_open_input(&ifmt_ctx_a, in_filename_a, 0, 0)) < 0) {
        printf( "Could not open input file.");
        goto end;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx_a, 0)) < 0) {
        printf( "Failed to retrieve input stream information");
        goto end;
    }
    printf("===========Input Information==========\n");
    av_dump_format(ifmt_ctx_v, 0, in_filename_v, 0);
    av_dump_format(ifmt_ctx_a, 0, in_filename_a, 0);
    printf("======================================\n");
    //Output
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        printf( "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    ofmt = ofmt_ctx->oformat;

    for (i = 0; i < ifmt_ctx_v->nb_streams; i++) {
        //Create output AVStream according to input AVStream
        if(ifmt_ctx_v->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            AVStream *in_stream = ifmt_ctx_v->streams[i];
            AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
            videoindex_v=i;
            if (!out_stream) {
                printf( "Failed allocating output stream\n");
                ret = AVERROR_UNKNOWN;
                goto end;
            }
            videoindex_out=out_stream->index;
            //Copy the settings of AVCodecContext
            if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
                printf( "Failed to copy context from input to output stream codec context\n");
                goto end;
            }
            out_stream->codec->codec_tag = 0;
            if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            break;
        }
    }

    for (i = 0; i < ifmt_ctx_a->nb_streams; i++) {
        //Create output AVStream according to input AVStream
        if(ifmt_ctx_a->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
            AVStream *in_stream = ifmt_ctx_a->streams[i];
            AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
            audioindex_a=i;
            if (!out_stream) {
                printf( "Failed allocating output stream\n");
                ret = AVERROR_UNKNOWN;
                goto end;
            }
            audioindex_out=out_stream->index;
            //Copy the settings of AVCodecContext
            if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
                printf( "Failed to copy context from input to output stream codec context\n");
                goto end;
            }
            out_stream->codec->codec_tag = 0;
            if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            break;
        }
    }

    printf("==========Output Information==========\n");
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    printf("======================================\n");
    //Open output file
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
            printf( "Could not open output file '%s'", out_filename);
            goto end;
        }
    }
    //Write file header
    if (avformat_write_header(ofmt_ctx, NULL) < 0) {
        printf( "Error occurred when opening output file\n");
        goto end;
    }


    //FIX
#if USE_H264BSF
    AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb"); 
#endif
#if USE_AACBSF
    AVBitStreamFilterContext* aacbsfc =  av_bitstream_filter_init("aac_adtstoasc"); 
#endif

    int seq = 1;
    while (1) {
        AVFormatContext *ifmt_ctx;
        int stream_index=0;
        AVStream *in_stream, *out_stream;

        //Get an AVPacket
        if(av_compare_ts(cur_pts_v,ifmt_ctx_v->streams[videoindex_v]->time_base,cur_pts_a,ifmt_ctx_a->streams[audioindex_a]->time_base) <= 0){
            ifmt_ctx=ifmt_ctx_v;
            stream_index=videoindex_out;

            if(av_read_frame(ifmt_ctx, &pkt) >= 0){
                do{
                    in_stream  = ifmt_ctx->streams[pkt.stream_index];
                    out_stream = ofmt_ctx->streams[stream_index];

                    if(pkt.stream_index==videoindex_v){
                        //FIX：No PTS (Example: Raw H.264)
                        //Simple Write PTS
                        if(pkt.pts==AV_NOPTS_VALUE){
                            //Write PTS
                            AVRational time_base1=in_stream->time_base;
                            //Duration between 2 frames (us)
                            int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(in_stream->r_frame_rate);
                            //Parameters
                            pkt.pts=(double)(frame_index*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
                            pkt.dts=pkt.pts;
                            pkt.duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
                            frame_index++;
                        }

                        cur_pts_v=pkt.pts;
                        break;
                    }
                }while(av_read_frame(ifmt_ctx, &pkt) >= 0);
            }else{
                break;
            }
        }else{
            ifmt_ctx=ifmt_ctx_a;
            stream_index=audioindex_out;
            if(av_read_frame(ifmt_ctx, &pkt) >= 0){
                do{
                    in_stream  = ifmt_ctx->streams[pkt.stream_index];
                    out_stream = ofmt_ctx->streams[stream_index];

                    if(pkt.stream_index==audioindex_a){

                        //FIX：No PTS
                        //Simple Write PTS
                        if(pkt.pts==AV_NOPTS_VALUE){
                            //Write PTS
                            AVRational time_base1=in_stream->time_base;
                            //Duration between 2 frames (us)
                            int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(in_stream->r_frame_rate);
                            //Parameters
                            pkt.pts=(double)(frame_index*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
                            pkt.dts=pkt.pts;
                            pkt.duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
                            frame_index++;
                        }
                        cur_pts_a=pkt.pts;

                        break;
                    }
                }while(av_read_frame(ifmt_ctx, &pkt) >= 0);
            }else{
                break;
            }

        }

        //FIX:Bitstream Filter
#if USE_H264BSF
        av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
#if USE_AACBSF
        av_bitstream_filter_filter(aacbsfc, out_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif


        //Convert PTS/DTS
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        pkt.stream_index=stream_index;

        printf("Write %d Packet. size:%5d\tpts:%lld\n",seq++,pkt.size,pkt.pts);
        //Write
        if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
            printf( "Error muxing packet\n");
            break;
        }
        av_free_packet(&pkt);

    }
    //Write file trailer
    av_write_trailer(ofmt_ctx);

#if USE_H264BSF
    av_bitstream_filter_close(h264bsfc);
#endif
#if USE_AACBSF
    av_bitstream_filter_close(aacbsfc);
#endif

end:
    avformat_close_input(&ifmt_ctx_v);
    avformat_close_input(&ifmt_ctx_a);
    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    if (ret < 0 && ret != AVERROR_EOF) {
        printf( "Error occurred.\n");
        return -1;
    }
    return 0;
}

int ffmpeg_media_mux_single(char*in_filename,char *out_filename)
{
    AVOutputFormat *ofmt = NULL;
    //Input AVFormatContext and Output AVFormatContext
    AVFormatContext *ifmt_ctx= NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    int ret, i;
    int stream_index=-1,streamindex_out=-1;
    
    int frame_index=0;
    int64_t cur_pts=0;

    av_register_all();
    //Input
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        printf( "Could not open input file.");
        goto end;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        printf( "Failed to retrieve input stream information");
        goto end;
    }

    printf("===========Input Information==========\n");
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    
    printf("======================================\n");
    //Output
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        printf( "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    ofmt = ofmt_ctx->oformat;

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        //Create output AVStream according to input AVStream
        if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            AVStream *in_stream = ifmt_ctx->streams[i];
            AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
            stream_index=i;
            if (!out_stream) {
                printf( "Failed allocating output stream\n");
                ret = AVERROR_UNKNOWN;
                goto end;
            }
            streamindex_out=out_stream->index;
            //Copy the settings of AVCodecContext
            if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
                printf( "Failed to copy context from input to output stream codec context\n");
                goto end;
            }
            out_stream->codec->codec_tag = 0;
            if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            break;
        }
    }



    printf("==========Output Information==========\n");
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    printf("======================================\n");
    //Open output file
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
            printf( "Could not open output file '%s'", out_filename);
            goto end;
        }
    }
    //Write file header
    if (avformat_write_header(ofmt_ctx, NULL) < 0) {
        printf( "Error occurred when opening output file\n");
        goto end;
    }


    //FIX
#if USE_H264BSF
    AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb"); 
#endif
#if USE_AACBSF
    AVBitStreamFilterContext* aacbsfc =  av_bitstream_filter_init("aac_adtstoasc"); 
#endif

    AVStream *in_stream, *out_stream;

    int seq = 1;
    while (1) {

        //Get an AVPacket
        if(av_read_frame(ifmt_ctx, &pkt) >= 0)
        {
            in_stream  = ifmt_ctx->streams[pkt.stream_index];
            out_stream = ofmt_ctx->streams[streamindex_out];

            if(pkt.pts==AV_NOPTS_VALUE){
                //Write PTS
                AVRational time_base1=in_stream->time_base;
                //Duration between 2 frames (us)
                int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(in_stream->r_frame_rate);
                //Parameters
                pkt.pts=(double)(frame_index*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
                pkt.dts=pkt.pts;
                pkt.duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
                frame_index++;
            }

            cur_pts=pkt.pts;
        }
        else
            break;

        //FIX:Bitstream Filter
#if USE_H264BSF
        av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
#if USE_AACBSF
        av_bitstream_filter_filter(aacbsfc, out_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif


        //Convert PTS/DTS
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        pkt.stream_index=stream_index;

        printf("Write %d Packet. size:%5d\tdts:%lld\tpts:%lld\n",seq++,pkt.size,pkt.dts,pkt.pts);
        //Write
        if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
            printf( "Error muxing packet\n");
            break;
        }
        av_free_packet(&pkt);

    }
    //Write file trailer
    av_write_trailer(ofmt_ctx);

#if USE_H264BSF
    av_bitstream_filter_close(h264bsfc);
#endif
#if USE_AACBSF
    av_bitstream_filter_close(aacbsfc);
#endif

end:
    avformat_close_input(&ifmt_ctx);
    
    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    if (ret < 0 && ret != AVERROR_EOF) {
        printf( "Error occurred.\n");
        return -1;
    }
    return 0;
}

int ffmpeg_media_transform(char*in_filename,char*out_filename)
{
    AVOutputFormat *ofmt = NULL;
    //Input AVFormatContext and Output AVFormatContext
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    
    int ret, i;
    int frame_index=0;

    av_register_all();
    //Input
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        printf( "Could not open input file.");
        goto end;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        printf( "Failed to retrieve input stream information");
        goto end;
    }
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    //Output
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        printf( "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    ofmt = ofmt_ctx->oformat;
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        //Create output AVStream according to input AVStream
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
        if (!out_stream) {
            printf( "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        //Copy the settings of AVCodecContext
        if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
            printf( "Failed to copy context from input to output stream codec context\n");
            goto end;
        }
        out_stream->codec->codec_tag = 0;
#if 1
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
#endif
    }
    //Output information------------------
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    //Open output file
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            printf( "Could not open output file '%s'", out_filename);
            goto end;
        }
    }


    AVDictionary* options = NULL;
    av_dict_set(&options, "movflags", "rtphint+faststart", 0); 

    //Write file header
    if (avformat_write_header(ofmt_ctx, &options) < 0) {
        printf( "Error occurred when opening output file\n");
        goto end;
    }

    while (1) {
        AVStream *in_stream, *out_stream;
        //Get an AVPacket
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
            break;
        in_stream  = ifmt_ctx->streams[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];

        //Convert PTS/DTS
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        //Write
        if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
            printf( "Error muxing packet\n");
            break;
        }
        printf("Write %8d frames to output file\n",frame_index);
        av_free_packet(&pkt);
        frame_index++;
    }
    //Write file trailer
    av_write_trailer(ofmt_ctx);
end:
    avformat_close_input(&ifmt_ctx);
    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

    return 0;
}

int ffmpeg_media_push(char *filename)
{
    AVOutputFormat *ofmt = NULL;
    //输入对应一个AVFormatContext，输出对应一个AVFormatContext
    //（Input AVFormatContext and Output AVFormatContext）
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    
    int ret, i;
    int videoindex = -1;
    int frame_index = 0;
    int64_t start_time = 0;

    av_register_all();
    avformat_network_init();

    char in_filename[128] = "";
    char out_filename[128] = "";

    strcpy(in_filename, filename);
    

    //输入（Input）
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        printf("Could not open input file.");
        goto end;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        printf("Failed to retrieve input stream information");
        goto end;
    }

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    }
        

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    //输出（Output）
    PathStripPath(in_filename);
    sprintf(out_filename, "rtsp://127.0.0.1:6554/mypush/%s", in_filename);

    avformat_alloc_output_context2(&ofmt_ctx, NULL, "rtsp", out_filename); //rtsp
    if (!ofmt_ctx) {
        printf("Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    ofmt = ofmt_ctx->oformat;
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        //根据输入流创建输出流（Create output AVStream according to input AVStream）
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
        if (!out_stream) {
            printf("Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        //复制AVCodecContext的设置（Copy the settings of AVCodecContext）
        ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
        if (ret < 0) {
            printf("Failed to copy context from input to output stream codec context\n");
            goto end;
        }
        out_stream->codec->codec_tag = 0;
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    //Dump Format------------------
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    //打开输出URL（Open output URL）
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            printf("Could not open output URL '%s'", out_filename);
            goto end;
        }
    }
    //写文件头（Write file header）
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        printf("Error occurred when opening output URL\n");
        goto end;
    }

    start_time = av_gettime();
    while (1) {
        AVStream *in_stream, *out_stream;
        //获取一个AVPacket（Get an AVPacket）
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
            break;
        //FIX：No PTS (Example: Raw H.264)
        //Simple Write PTS
        if (pkt.pts == AV_NOPTS_VALUE) {
            //Write PTS
            AVRational time_base1 = ifmt_ctx->streams[videoindex]->time_base;
            //Duration between 2 frames (us)
            int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(ifmt_ctx->streams[videoindex]->r_frame_rate);
            //Parameters
            pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
            pkt.dts = pkt.pts;
            pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
        }
        //Important:Delay
        if (pkt.stream_index == videoindex) {
            AVRational time_base = ifmt_ctx->streams[videoindex]->time_base;
            AVRational time_base_q = { 1,AV_TIME_BASE };
            int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
            int64_t now_time = av_gettime() - start_time;
            if (pts_time > now_time)
                av_usleep(pts_time - now_time);

        }

        in_stream = ifmt_ctx->streams[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];
        /* copy packet */
        //转换PTS/DTS（Convert PTS/DTS）
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        //Print to Screen
        if (pkt.stream_index == videoindex) {
            printf("Send %8d video frames to output URL\n", frame_index);
            frame_index++;
        }
        //ret = av_write_frame(ofmt_ctx, &pkt);
        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);

        if (ret < 0) {
            printf("Error muxing packet\n");
            break;
        }

        av_free_packet(&pkt);

    }
    //写文件尾（Write file trailer）
    av_write_trailer(ofmt_ctx);
end:
    avformat_close_input(&ifmt_ctx);
    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    if (ret < 0 && ret != AVERROR_EOF) {
        printf("Error occurred.\n");
        return -1;
    }
    return 0;
}
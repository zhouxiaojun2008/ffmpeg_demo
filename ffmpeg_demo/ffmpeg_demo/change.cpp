/*
* copyright (c) 2017 老衲不出家
*
* 2017-08-11
*
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

extern "C"
{
#include "libswscale/swscale.h" 
#include "libavutil/pixfmt.h"
}


const char *srcFileName = "1509340580.flv.yuv";
const char *dstFileName = "1509340580.flv.change.yuv";

int change(int argc, char **argv)
{
    // 設定原始 YUV 的長寬 
    const int in_width = 848;
    const int in_height = 480;


    // 設定目的 YUV 的長寬 
    int out_width = 1920;
    int out_height = 1080;

    out_width = (out_width + 0xf) & ~0xf;
    out_height = (out_height + 0xf) & ~0xf;

    const int read_size = in_width * in_height * 3 / 2;
    const int write_size = out_width * out_height * 3 / 2;
    struct SwsContext *img_convert_ctx = nullptr;

    unsigned char *inbuf[4];
    unsigned char *outbuf[4];

    int inlinesize[4] = { in_width, in_width / 2, in_width / 2, 0 };
    int outlinesize[4] = { out_width, out_width / 2, out_width / 2, 0 };

    unsigned char *ptr_src_yuv_buf = nullptr;
    unsigned char *ptr_dst_yuv_buf = nullptr;
    ptr_src_yuv_buf = new unsigned char[in_width * in_height * 3/2];
    ptr_dst_yuv_buf = new unsigned char[out_width * out_height * 3 /2];

    FILE *fin = fopen(srcFileName, "rb");
    FILE *fout = fopen(dstFileName, "wb");

    if (fin == NULL) {
        fprintf(stderr, "open input file %s error.\n", srcFileName);
        return -1;
    }

    if (fout == NULL) {
        fprintf(stderr, "open output file %s error.\n", dstFileName);
        return -1;
    }

    inbuf[0] = (unsigned char *)malloc(in_width*in_height);
    inbuf[1] = (unsigned char *)malloc(in_width*in_height >> 2);
    inbuf[2] = (unsigned char *)malloc(in_width*in_height >> 2);
    inbuf[3] = NULL;

    outbuf[0] = (unsigned char *)malloc(out_width*out_height);
    outbuf[1] = (unsigned char *)malloc(out_width*out_height >> 2);
    outbuf[2] = (unsigned char *)malloc(out_width*out_height >> 2);
    outbuf[3] = NULL;

    // ********* Initialize software scaling ********* 
    // ********* sws_getContext ********************** 
    img_convert_ctx = sws_getContext(in_width, in_height, AV_PIX_FMT_YUV420P,
        out_width, out_height, AV_PIX_FMT_YUV420P, SWS_POINT, nullptr, nullptr, nullptr);
    if (img_convert_ctx == NULL) {
        fprintf(stderr, "Cannot initialize the conversion context!\n");
        return -1;
    }

    int32_t in_y_size = in_width*in_height;
    int32_t out_y_size;

    bool bExit = false;
    while (!bExit) {

        if ((fread(ptr_src_yuv_buf, 1, read_size, fin) < 0) || (feof(fin))) {
            bExit = true;
            break;
        }

        memcpy(inbuf[0], ptr_src_yuv_buf, in_y_size);
        memcpy(inbuf[1], ptr_src_yuv_buf + in_y_size, in_y_size/4);
        memcpy(inbuf[2], ptr_src_yuv_buf + in_y_size*5/4, in_y_size / 4);

        // ********* 主要的 function ****** 
        // ********* sws_scale ************ 
        sws_scale(img_convert_ctx, inbuf, inlinesize,
            0, in_height, outbuf, outlinesize);

        memcpy(ptr_dst_yuv_buf, outbuf[0], out_width*out_height);
        memcpy(ptr_dst_yuv_buf + out_width*out_height, outbuf[1], out_width*out_height >> 2);
        memcpy(ptr_dst_yuv_buf + (out_width*out_height * 5 >> 2), outbuf[2], out_width*out_height >> 2);

        fwrite(ptr_dst_yuv_buf, 1, write_size, fout);
    }
    // ********* 結束的 function ******* 
    // ********* sws_freeContext ******* 
    sws_freeContext(img_convert_ctx);

    fclose(fin);
    fclose(fout);

    delete[] ptr_src_yuv_buf;
    ptr_src_yuv_buf = nullptr;

    delete[] ptr_dst_yuv_buf;
    ptr_dst_yuv_buf = nullptr;

    getchar();
    return 0;
}

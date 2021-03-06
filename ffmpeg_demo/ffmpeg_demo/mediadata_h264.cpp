#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "util.h"

int simplest_yuv420_split(char *url, int w, int h,int num)
{
    FILE *fp=fopen(url,"rb+");
    if (fp == NULL)
        return 0;

    char filename[64] = "";
    
    strcpy(filename,url);
    strcat(filename,"_y.yuv");
    FILE *fpy=fopen(filename,"wb+");

    strcpy(filename,url);
    strcat(filename,"_u.yuv");
    FILE *fpu=fopen(filename,"wb+");

    strcpy(filename,url);
    strcat(filename,"_v.yuv");
    FILE *fpv=fopen(filename,"wb+");

    unsigned char *pic=(unsigned char *)malloc(w*h*3/2);

    for (int i=0;i<num;i++){
        fread(pic,1,w*h*3/2,fp);

        fwrite(pic,1,w*h,fpy);

        fwrite(pic+w*h,1,w*h/4,fpu);

        fwrite(pic+w*h*5/4,1,w*h/4,fpv);
    }

    fclose(fp);

    fclose(fpy);

    fclose(fpu);

    fclose(fpv);
}

int simplest_yuv420_gray(char *url, int w, int h,int num)
{
    FILE *fp=fopen(url,"rb+");
    if (fp == NULL)
        return 0;

    char filename[64] = "";

    strcpy(filename,url);
    strcat(filename,"_gray.yuv");
    FILE *fp_half=fopen(filename,"wb+");

   

    unsigned char *pic=(unsigned char *)malloc(w*h*3/2);

    for (int i=0;i<num;i++){
        fread(pic,1,w*h*3/2,fp);

        memset(pic+w*h,128,w*h/2);
        fwrite(pic,1,w*h*3/2,fp_half);
    }

    fclose(fp);
    fclose(fp_half);
}

int simplest_yuv420_halfy(char *url, int w, int h,int num)
{
    FILE *fp=fopen(url,"rb+");
    if (fp == NULL)
        return 0;

    char filename[64] = "";

    strcpy(filename,url);
    strcat(filename,"_half.yuv");
    FILE *fp_half=fopen(filename,"wb+");



    unsigned char *pic=(unsigned char *)malloc(w*h*3/2);

    for (int i=0;i<num;i++){
        fread(pic,1,w*h*3/2,fp);

        for (int j=0;j<w*h;j++)
            pic[j] = pic[j]/2;
        fwrite(pic,1,w*h*3/2,fp_half);
    }

    fclose(fp);
    fclose(fp_half);
}

int simplest_yuv420_border(char *url, int w, int h,int border,int num)
{
    FILE *fp=fopen(url,"rb+");
    if (fp == NULL)
        return 0;

    char filename[64] = "";

    strcpy(filename,url);
    strcat(filename,"_border.yuv");
    FILE *fp_border=fopen(filename,"wb+");



    unsigned char *pic=(unsigned char *)malloc(w*h*3/2);

    for (int i=0;i<num;i++){
        fread(pic,1,w*h*3/2,fp);

        for(int j=0;j<h;j++){
            for(int k=0;k<w;k++){
                if(k<border||k>(w-border)||j<border||j>(h-border)){
                    //pic[j*w+k]=255;
                    pic[j*w+k]=0;
                }
            }
        }

        fwrite(pic,1,w*h*3/2,fp_border);
    }

    fclose(fp);
    fclose(fp_border);
}

int simplest_yuv420_graybar(int width, int height,int ymin,int ymax,int barnum,char *url_out)
{
    int barwidth;
    float lum_inc;
    unsigned char lum_temp;
    int uv_width,uv_height;
    FILE *fp=NULL;
    unsigned char *data_y=NULL;
    unsigned char *data_u=NULL;
    unsigned char *data_v=NULL;
    int t=0,i=0,j=0;

    barwidth=width/barnum;
    lum_inc=((float)(ymax-ymin))/((float)(barnum-1));
    uv_width=width/2;
    uv_height=height/2;

    data_y=(unsigned char *)malloc(width*height);
    data_u=(unsigned char *)malloc(uv_width*uv_height);
    data_v=(unsigned char *)malloc(uv_width*uv_height);

    if((fp=fopen(url_out,"wb+"))==NULL){
        printf("Error: Cannot create file!");
        return -1;
    }

    //Output Info
    printf("Y, U, V value from picture's left to right:\n");
    for(t=0;t<(width/barwidth);t++){
        lum_temp=ymin+(char)(t*lum_inc);
        printf("%3d, 128, 128\n",lum_temp);
    }
    //Gen Data
    for(j=0;j<height;j++){
        for(i=0;i<width;i++){
            t=i/barwidth;
            lum_temp=ymin+(char)(t*lum_inc);
            data_y[j*width+i]=lum_temp;
        }
    }
    for(j=0;j<uv_height;j++){
        for(i=0;i<uv_width;i++){
            data_u[j*uv_width+i]=128;
        }
    }
    for(j=0;j<uv_height;j++){
        for(i=0;i<uv_width;i++){
            data_v[j*uv_width+i]=128;
        }
    }
    fwrite(data_y,width*height,1,fp);
    fwrite(data_u,uv_width*uv_height,1,fp);
    fwrite(data_v,uv_width*uv_height,1,fp);
    fclose(fp);
    free(data_y);
    free(data_u);
    free(data_v);
    return 0;
}

int simplest_rgb24_split(char *url, int w, int h,int num)
{
    FILE *fp=fopen(url,"rb+");
    FILE *fp1=fopen("output_r.y","wb+");
    FILE *fp2=fopen("output_g.y","wb+");
    FILE *fp3=fopen("output_b.y","wb+");

    unsigned char *pic=(unsigned char *)malloc(w*h*3);

    for(int i=0;i<num;i++){

        fread(pic,1,w*h*3,fp);

        for(int j=0;j<w*h*3;j=j+3){
            //R
            fwrite(pic+j,1,1,fp1);
            //G
            fwrite(pic+j+1,1,1,fp2);
            //B
            fwrite(pic+j+2,1,1,fp3);
        }
    }

    free(pic);
    fclose(fp);
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);

    return 0;
}

int simplest_rgb24_to_bmp(const char *rgb24path,int width,int height,const char *bmppath)
{
    typedef struct 
    {  
        long imageSize;
        long blank;
        long startPosition;
    }BmpHead;

    typedef struct
    {
        long  Length;
        long  width;
        long  height;
        unsigned short  colorPlane;
        unsigned short  bitColor;
        long  zipFormat;
        long  realSize;
        long  xPels;
        long  yPels;
        long  colorUse;
        long  colorImportant;
    }InfoHead;

    int i=0,j=0;
    BmpHead m_BMPHeader={0};
    InfoHead  m_BMPInfoHeader={0};
    char bfType[2]={'B','M'};
    int header_size=sizeof(bfType)+sizeof(BmpHead)+sizeof(InfoHead);
    unsigned char *rgb24_buffer=NULL;
    FILE *fp_rgb24=NULL,*fp_bmp=NULL;

    if((fp_rgb24=fopen(rgb24path,"rb"))==NULL){
        printf("Error: Cannot open input RGB24 file.\n");
        return -1;
    }
    if((fp_bmp=fopen(bmppath,"wb"))==NULL){
        printf("Error: Cannot open output BMP file.\n");
        return -1;
    }

    rgb24_buffer=(unsigned char *)malloc(width*height*3);
    fread(rgb24_buffer,1,width*height*3,fp_rgb24);

    m_BMPHeader.imageSize=3*width*height+header_size;
    m_BMPHeader.startPosition=header_size;

    m_BMPInfoHeader.Length=sizeof(InfoHead); 
    m_BMPInfoHeader.width=width;
    //BMP storage pixel data in opposite direction of Y-axis (from bottom to top).
    m_BMPInfoHeader.height=-height;
    m_BMPInfoHeader.colorPlane=1;
    m_BMPInfoHeader.bitColor=24;
    m_BMPInfoHeader.realSize=3*width*height;

    fwrite(bfType,1,sizeof(bfType),fp_bmp);
    fwrite(&m_BMPHeader,1,sizeof(m_BMPHeader),fp_bmp);
    fwrite(&m_BMPInfoHeader,1,sizeof(m_BMPInfoHeader),fp_bmp);

    //BMP save R1|G1|B1,R2|G2|B2 as B1|G1|R1,B2|G2|R2
    //It saves pixel data in Little Endian
    //So we change 'R' and 'B'
    for(j =0;j<height;j++){
        for(i=0;i<width;i++){
            char temp=rgb24_buffer[(j*width+i)*3+2];
            rgb24_buffer[(j*width+i)*3+2]=rgb24_buffer[(j*width+i)*3+0];
            rgb24_buffer[(j*width+i)*3+0]=temp;
        }
    }
    fwrite(rgb24_buffer,3*width*height,1,fp_bmp);
    fclose(fp_rgb24);
    fclose(fp_bmp);
    free(rgb24_buffer);
    printf("Finish generate %s!\n",bmppath);
    return 0;
    return 0;
}

unsigned char clip_value(unsigned char x,unsigned char min_val,unsigned char  max_val){
    if(x>max_val){
        return max_val;
    }else if(x<min_val){
        return min_val;
    }else{
        return x;
    }
}

//RGB to YUV420
bool RGB24_TO_YUV420(unsigned char *RgbBuf,int w,int h,unsigned char *yuvBuf)
{
    unsigned char*ptrY, *ptrU, *ptrV, *ptrRGB;
    memset(yuvBuf,0,w*h*3/2);
    ptrY = yuvBuf;
    ptrU = yuvBuf + w*h;
    ptrV = ptrU + (w*h*1/4);
    unsigned char y, u, v, r, g, b;
    for (int j = 0; j<h;j++){
        ptrRGB = RgbBuf + w*j*3 ;
        for (int i = 0;i<w;i++){
            int pos = w*i+j;
            r = *(ptrRGB++);
            g = *(ptrRGB++);
            b = *(ptrRGB++);
            y = (unsigned char)( ( 66 * r + 129 * g +  25 * b + 128) >> 8) + 16  ;          
            u = (unsigned char)( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128 ;          
            v = (unsigned char)( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128 ;
            *(ptrY++) = clip_value(y,0,255);
            if (j%2==0&&i%2 ==0){
                *(ptrU++) =clip_value(u,0,255);
            }
            else{
                if (i%2==0){
                    *(ptrV++) =clip_value(v,0,255);
                }
            }
        }
    }
    return true;
}

bool YUV420_TO_RGB24(unsigned char *yuvBuf,int w,int h,unsigned char *RgbBuf)
{
    unsigned char*ptrY, *ptrU, *ptrV, *ptrRGB;
    memset(RgbBuf,0,w*h*3);
    ptrY = yuvBuf;
    ptrU = ptrY + w*h;
    ptrV = ptrU + (w*h*1/4);
    unsigned char y, u, v, r, g, b;
    for (int i = 0; i<h;i++){
        ptrRGB = RgbBuf + w*i*3 ;
        for (int j = 0;j<w;j++){

            y = *(ptrY++);
            u = *(ptrU + (i/2) * (w/2) + (j/2));
            v = *(ptrV + (i/2) * (w/2) + (j/2));

            b = (int)(y + 1.732446 * (u - 128));                                    // b分量
            g = (int)(y - 0.698001 * (u - 128) - 0.703125 * (v - 128));            // g分量
            r = (int)(y + 1.370705 * (v - 128));                                    // r分量


            r = clip_value(r,0,255); // 四舍五入到最近的整数
            g = clip_value(g,0,255);
            b = clip_value(b,0,255);

         
           *(ptrRGB++) = r;
           *(ptrRGB++) = g;
           *(ptrRGB++) = b;
        }
    }
    return true;
}

int simplest_rgb24_to_yuv420(char *url_in, int w, int h,int num,char *url_out)
{
    FILE *fp=fopen(url_in,"rb+");
    FILE *fp1=fopen(url_out,"wb+");

    unsigned char *pic_rgb24=(unsigned char *)malloc(w*h*3);
    unsigned char *pic_yuv420=(unsigned char *)malloc(w*h*3/2);

    for(int i=0;i<num;i++){
        fread(pic_rgb24,1,w*h*3,fp);
        RGB24_TO_YUV420(pic_rgb24,w,h,pic_yuv420);
        fwrite(pic_yuv420,1,w*h*3/2,fp1);
    }

    free(pic_rgb24);
    free(pic_yuv420);
    fclose(fp);
    fclose(fp1);

    return 0;
}

int simplest_yuv420_to_rgb24(char *url_in, int w, int h,int num,char *url_out)
{
    FILE *fp=fopen(url_in,"rb+");
    FILE *fp1=fopen(url_out,"wb+");

    unsigned char *pic_rgb24=(unsigned char *)malloc(w*h*3);
    unsigned char *pic_yuv420=(unsigned char *)malloc(w*h*3/2);

    int ret = 0;
    int total = 0;
    for(int i=0;i<num;i++){
        fread(pic_yuv420,1,w*h*3/2,fp);
        YUV420_TO_RGB24(pic_yuv420,w,h,pic_rgb24);
        
        total = 0;
        while(1)
        {
            ret = fwrite(pic_rgb24,1,w*h*3,fp1);
            if (ret == w*h*3)
                break;
            else if (ret > 0)
                total += ret;
            else if (ret <= 0)
            {
                fprintf(stderr,"fwrite failed,ret = %d",ret);
                exit(-1);
            }
            if (total == w*h*3)
                break;
        }
        
    }

    free(pic_rgb24);
    free(pic_yuv420);
    fclose(fp);
    fclose(fp1);

    return 0;
}





typedef struct
{
    int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
    unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
    unsigned max_size;            //! Nal Unit Buffer size
    int forbidden_bit;            //! should be always FALSE
    int nal_reference_idc;        //! NALU_PRIORITY_xxxx
    int nal_unit_type;            //! NALU_TYPE_xxxx    
    char *buf;                    //! contains the first byte followed by the EBSP
} NALU_t;

typedef enum {
    NALU_TYPE_SLICE    = 1,
    NALU_TYPE_DPA      = 2,
    NALU_TYPE_DPB      = 3,
    NALU_TYPE_DPC      = 4,
    NALU_TYPE_IDR      = 5,
    NALU_TYPE_SEI      = 6,
    NALU_TYPE_SPS      = 7,
    NALU_TYPE_PPS      = 8,
    NALU_TYPE_AUD      = 9,
    NALU_TYPE_EOSEQ    = 10,
    NALU_TYPE_EOSTREAM = 11,
    NALU_TYPE_FILL     = 12,
} NaluType;

typedef enum {
    NALU_PRIORITY_DISPOSABLE = 0,
    NALU_PRIRITY_LOW         = 1,
    NALU_PRIORITY_HIGH       = 2,
    NALU_PRIORITY_HIGHEST    = 3
} NaluPriority;

FILE *h264bitstream = NULL;                //!< the bit stream file
int info2=0, info3=0;

static int FindStartCode2 (unsigned char *Buf){
    if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1) return 0; //0x000001?
    else return 1;
}

static int FindStartCode3 (unsigned char *Buf){
    if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) return 0;//0x00000001?
    else return 1;
}

int GetAnnexbNALU (NALU_t *nalu)
{
    int pos = 0;
    int StartCodeFound, rewind;
    unsigned char *Buf;

    if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL) 
        printf ("GetAnnexbNALU: Could not allocate Buf memory\n");

    nalu->startcodeprefix_len=3;

    if (3 != fread (Buf, 1, 3, h264bitstream)){
        free(Buf);
        return 0;
    }
    info2 = FindStartCode2 (Buf);
    if(info2 != 1) {
        if(1 != fread(Buf+3, 1, 1, h264bitstream)){
            free(Buf);
            return 0;
        }
        info3 = FindStartCode3 (Buf);
        if (info3 != 1){ 
            free(Buf);
            return -1;
        }
        else {
            pos = 4;
            nalu->startcodeprefix_len = 4;
        }
    }
    else{
        nalu->startcodeprefix_len = 3;
        pos = 3;
    }
    StartCodeFound = 0;
    info2 = 0;
    info3 = 0;

    while (!StartCodeFound){
        if (feof (h264bitstream)){
            nalu->len = (pos-1)-nalu->startcodeprefix_len;
            memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);     
            nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
            nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
            nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
            free(Buf);
            return pos-1;
        }
        Buf[pos++] = fgetc (h264bitstream);
        info3 = FindStartCode3(&Buf[pos-4]);
        if(info3 != 1)
            info2 = FindStartCode2(&Buf[pos-3]);
        StartCodeFound = (info2 == 1 || info3 == 1);
    }

    // Here, we have found another start code (and read length of startcode bytes more than we should
    // have.  Hence, go back in the file
    rewind = (info3 == 1)? -4 : -3;

    if (0 != fseek (h264bitstream, rewind, SEEK_CUR)){
        free(Buf);
        printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
    }

    // Here the Start code, the complete NALU, and the next start code is in the Buf.  
    // The size of Buf is pos, pos+rewind are the number of bytes excluding the next
    // start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code

    nalu->len = (pos+rewind)-nalu->startcodeprefix_len;
    memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);//
    nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
    nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
    nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
    free(Buf);

    return (pos+rewind);
}

int simplest_h264_parser(char *url)
{
    NALU_t *n;
    int buffersize=1 * 1024 * 1024;

    char logname[128] = "";
    memset(logname,0x00,sizeof(logname));
    strcat(logname,url);
    strcat(logname,".nalu");
    FILE *myout = fopen(logname,"wb");
    //FILE *myout=stdout;

    h264bitstream=fopen(url, "rb+");
    if (h264bitstream==NULL){
        printf("Open file error\n");
        return 0;
    }

    n = (NALU_t*)calloc (1, sizeof (NALU_t));
    if (n == NULL){
        printf("Alloc NALU Error\n");
        return 0;
    }

    n->max_size=buffersize;
    n->buf = (char*)calloc (buffersize, sizeof (char));
    if (n->buf == NULL){
        free (n);
        printf ("AllocNALU: n->buf");
        return 0;
    }


    int data_offset=0;
    int nal_num=0;
    fprintf(myout,"-----+-------- NALU Table ------+---------+\n");
    fprintf(myout," NUM |    POS  |    IDC |  TYPE |   LEN   |\n");
    fprintf(myout,"-----+---------+--------+-------+---------+\n");

    while(!feof(h264bitstream)) 
    {
        int data_lenth;
        data_lenth=GetAnnexbNALU(n);

        char type_str[20]={0};
        switch(n->nal_unit_type){
        case NALU_TYPE_SLICE:sprintf(type_str,"SLICE");break;
        case NALU_TYPE_DPA:sprintf(type_str,"DPA");break;
        case NALU_TYPE_DPB:sprintf(type_str,"DPB");break;
        case NALU_TYPE_DPC:sprintf(type_str,"DPC");break;
        case NALU_TYPE_IDR:sprintf(type_str,"IDR");break;
        case NALU_TYPE_SEI:sprintf(type_str,"SEI");break;
        case NALU_TYPE_SPS:sprintf(type_str,"SPS");break;
        case NALU_TYPE_PPS:sprintf(type_str,"PPS");break;
        case NALU_TYPE_AUD:sprintf(type_str,"AUD");break;
        case NALU_TYPE_EOSEQ:sprintf(type_str,"EOSEQ");break;
        case NALU_TYPE_EOSTREAM:sprintf(type_str,"EOSTREAM");break;
        case NALU_TYPE_FILL:sprintf(type_str,"FILL");break;
        }
        char idc_str[20]={0};
        switch(n->nal_reference_idc>>5){
        case NALU_PRIORITY_DISPOSABLE:sprintf(idc_str,"DISPOS");break;
        case NALU_PRIRITY_LOW:sprintf(idc_str,"LOW");break;
        case NALU_PRIORITY_HIGH:sprintf(idc_str,"HIGH");break;
        case NALU_PRIORITY_HIGHEST:sprintf(idc_str,"HIGHEST");break;
        }

        fprintf(myout,"%5d| %8d| %7s| %6s| %8d|\n",nal_num,data_offset,idc_str,type_str,n->len);

        data_offset=data_offset+data_lenth;

        nal_num++;
    }

    //Free
    if (n){
        if (n->buf){
            free(n->buf);
            n->buf=NULL;
        }
        free (n);
    }

    fclose(myout);
    fclose(h264bitstream);
    return 0;
}

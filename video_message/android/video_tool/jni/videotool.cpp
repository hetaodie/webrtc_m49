
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#if defined (ANDROID_NDK)
#include <android/log.h>
#endif

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "x264.h"
#include "x264_config.h"
};


extern "C" int ClipAndScaleH264(char *sInH264FileName, char* sOutH264FileName,
                                int startFrame, int totalFrame,
                                int width, int height, float qp){
    //h264Dec parameter:
    AVPacket packet;
    AVCodec *pCodec= NULL;
    AVCodecContext *pCodecCtx= NULL;
    AVCodecParserContext *pCodecParserCtx=NULL;
    FILE *fpInFile=NULL;
    FILE *fpOutFile=NULL;
    AVFrame	*pFrame=NULL;
    AVFrame *pFrameYUV=NULL;
    uint8_t *pOutYuvBuf=NULL;
    const int InBufSize=4096;
    uint8_t InBuffer[InBufSize]={0};
    uint8_t *pCurPtr=NULL;
    uint8_t *pInBufAddr=NULL;
    struct SwsContext *img_convert_ctx=NULL;
    AVCodecID codec_id=AV_CODEC_ID_H264;
    int ret=0;
    int iCurSize=0;
    int iGotPic=0;
    int iFistTime=1;
    int iFrameCount = 0;
    int iEncCount=0;
    
    //x264Enc parameter:
    x264_picture_t pic_in,pic_out;
    x264_t *encoder=NULL;
    x264_param_t m_param;
    int iNalCnt=0;
    int32_t iFrameSize=0;
    int64_t iPts=0;
    x264_nal_t *nals=NULL;
    x264_nal_t *nal=NULL;

    int iPicInMallocFlag = 0; //x264_picture_alloc中分配内存标志，如果为1表明分配过内存，使用后需要释放;
    
    //下面初始化h264解码库
    avcodec_register_all();
    pCodec = avcodec_find_decoder(codec_id);
    if (!pCodec) {
        printf("Codec not found\n");
        goto INSIDE_MEM_FREE;
    }
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx){
        printf("Could not allocate video codec context\n");
        goto INSIDE_MEM_FREE;
    }
    
    pCodecCtx->thread_count = 3; //thread control
    pCodecParserCtx=av_parser_init(codec_id);
    if (!pCodecParserCtx){
        printf("Could not allocate video parser context\n");
        goto INSIDE_MEM_FREE;
    }
    if(pCodec->capabilities&CODEC_CAP_TRUNCATED)
        pCodecCtx->flags|= CODEC_FLAG_TRUNCATED; // we do not send complete frames
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec\n");
        goto INSIDE_MEM_FREE;
    }
    
    //Input File
    fpInFile = fopen(sInH264FileName, "rb");
    if (!fpInFile) {
        printf("Could not open input stream\n");
        goto INSIDE_MEM_FREE;
    }

    //下面初始化h264编码库
    fpOutFile = fopen(sOutH264FileName,"wb");
    if(!fpOutFile)
    {
        printf("could not open output 264 stream file\n");
        goto INSIDE_MEM_FREE;
    }
    
    // Get default params for preset/tuning: ultrafast/superfast/veryfast/faster/fast/medium
    if(x264_param_default_preset(&m_param,"veryfast",NULL) < 0){
        printf("x264_param_default_preset error\n");
        goto INSIDE_MEM_FREE;
    }
    
    // Configure non-default params
    m_param.i_csp = X264_CSP_I420;
    m_param.i_width  = width;
    m_param.i_height = height;
    m_param.b_vfr_input = 0;
    m_param.b_repeat_headers = 1;
    m_param.b_annexb = 1;
    if (qp < 1 ) {
        qp = 1;
    } else if (qp > 51) {
        qp = 51;
    }
    m_param.rc.f_rf_constant = qp;
    
    // Apply profile restrictions:  "main" "high" "high10"
    if(x264_param_apply_profile(&m_param,"baseline") < 0){
        printf("x264_param_apply_profile error\n");
        goto INSIDE_MEM_FREE;
    }
    
    encoder = x264_encoder_open(&m_param);
    if (!encoder) {
        printf("x264_encoder_open error\n");
        goto INSIDE_MEM_FREE;
    }
    
    x264_encoder_parameters( encoder, &m_param );
    
    if(x264_picture_alloc(&pic_in, X264_CSP_I420, width, height) < 0){
        printf("x264_picture_alloc error\n");
        goto INSIDE_MEM_FREE;
    }
    iPicInMallocFlag = 1; //标志分配了输入图像存储空间，如果正常分配，则后续要释放;
    pInBufAddr = pic_in.img.plane[0]; //暂时存储pic_in分配的空间首地址，释放空间的时候用到;

    //处理流程: 解码一帧->缩放->编码:
    pFrame = av_frame_alloc();
    av_init_packet(&packet);

    while (1) {
        iCurSize = fread(InBuffer, 1, InBufSize, fpInFile); //read input stream
        if (iCurSize == 0)
            break;
        pCurPtr=InBuffer;
        while (iCurSize>0){
            int len = av_parser_parse2(pCodecParserCtx, pCodecCtx, &packet.data, &packet.size, pCurPtr, iCurSize ,
                                       AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
            pCurPtr += len;
            iCurSize -= len;
            if(packet.size==0)
                continue;

            ret = avcodec_decode_video2(pCodecCtx, pFrame, &iGotPic, &packet);
            if (ret < 0) {
                printf("Decode Error.(解码错误)\n");
                return ret;
            }

            if (iGotPic) {
                if(iFistTime){
                    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                                     width, height, PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL);
                    pFrameYUV=avcodec_alloc_frame();
                    pOutYuvBuf=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, width, height));
                    avpicture_fill((AVPicture *)pFrameYUV, pOutYuvBuf, PIX_FMT_YUV420P, width, height);
                    iFistTime=0;
                }
                
                if ((iFrameCount>=startFrame) && (iEncCount <= totalFrame)) {
                    sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                              pFrameYUV->data, pFrameYUV->linesize);
                    
                    //encoder frame:
                    pic_in.img.plane[0] = pFrameYUV->data[0];
                    pic_in.img.plane[1] = pFrameYUV->data[1];
                    pic_in.img.plane[2] = pFrameYUV->data[2];
                    pic_in.i_pts = iPts++;
                    
                    x264_encoder_encode(encoder, &nals, &iNalCnt, &pic_in, &pic_out);
                    
                    for (nal = nals; nal < nals + iNalCnt; nal++) {
                        fwrite(nal->p_payload, 1, nal->i_payload, fpOutFile);
                    }
                    iEncCount++;
                }
                
                iFrameCount++;
            }
        }
    }
    
    //Flush Decoder
    while(1){
        int len = av_parser_parse2(
                                   pCodecParserCtx, pCodecCtx,
                                   &packet.data, &packet.size,
                                   pCurPtr , iCurSize ,
                                   AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
        pCurPtr += len;
        iCurSize -= len;
        ret = avcodec_decode_video2(pCodecCtx, pFrame, &iGotPic, &packet);
        if (ret < 0) {
            printf("Decode Error.(解码错误)\n");
            return ret;
        }
        if (!iGotPic){
            break;
        }
        if (iGotPic) {
            if ((iFrameCount>=startFrame) && (iEncCount < totalFrame)) {
                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                          pFrameYUV->data, pFrameYUV->linesize);
                
                pic_in.img.plane[0] = pFrameYUV->data[0];
                pic_in.img.plane[1] = pFrameYUV->data[1];
                pic_in.img.plane[2] = pFrameYUV->data[2];
                pic_in.i_pts = iPts++;
                
                x264_encoder_encode(encoder, &nals, &iNalCnt, &pic_in, &pic_out);
                
                for (nal = nals; nal < nals + iNalCnt; nal++) {
                    fwrite(nal->p_payload, 1,nal->i_payload,fpOutFile);
                }
                iEncCount++;
            }
            iFrameCount ++;
        }
    }
    
    // Flush delayed frames
    while(x264_encoder_delayed_frames(encoder))
    {
        iFrameSize = x264_encoder_encode(encoder, &nal, &iNalCnt, NULL, &pic_out);
        if(iFrameSize < 0){
            printf("x264_encoder_encode error\n");
            goto INSIDE_MEM_FREE;
        }
        else if(iFrameSize)
        {
            if(!fwrite(nal->p_payload, iFrameSize, 1, fpOutFile)){
                printf("fwrite stdout error\n");
                goto INSIDE_MEM_FREE;
            }
        }
    }
    
    packet.data = NULL;
    packet.size = 0;
    
INSIDE_MEM_FREE:
    if (fpInFile) {
        fclose(fpInFile);
        fpInFile = NULL;
    }
    
    sws_freeContext(img_convert_ctx);
    av_parser_close(pCodecParserCtx);
    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    av_free(pCodecCtx);
    if (pOutYuvBuf) {
        free(pOutYuvBuf);
        pOutYuvBuf = NULL;
    }
    if (encoder) {
        x264_encoder_close(encoder);
    }
    
    if (fpOutFile) {
        fclose(fpOutFile);
        fpOutFile = NULL;
    }
    
    if (iPicInMallocFlag) {
        pic_in.img.plane[0] = pInBufAddr;
        x264_picture_clean(&pic_in);
    }
    
    return 0;
}


#ifndef __UDP_CAMERA_CLIENT__
#define __UDP_CAMERA_CLIENT__
#include <cassert>
#include <iostream>
#include "DisplayWindow.h"

extern "C"{  
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
class UdpCameraClient{
  AVCodecContext *pInCodecCtx;
  AVFormatContext *pInFormatCtx;
  DisplayWindow *display;
  int openInput();
public:
  UdpCameraClient();
  int play();
};

int UdpCameraClient::openInput(){
  
  const char* fileUrl = "udp://192.168.1.104:8989";
  pInFormatCtx = avformat_alloc_context();
  if (avformat_open_input(&pInFormatCtx, fileUrl, NULL, NULL)!=0){
    std::cout<<"open input failed. "<<std::endl;
    return -1;
  }
  
  std::cout<<"here "<<std::endl;
  if (avformat_find_stream_info(pInFormatCtx, 0)<0){
    std::cout<<"find stream failed. "<<std::endl;
    return -1;
  }
  
  std::cout<<"after find stream"<<std::endl;
  int videoStream =0;
  for (int i=0;i<pInFormatCtx->nb_streams;i++){
    if (pInFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
      videoStream = i;
      break;
    }
  }
  pInCodecCtx = pInFormatCtx->streams[videoStream]->codec;
  AVCodec *pInDec = avcodec_find_decoder(pInCodecCtx->codec_id);
  assert(pInDec != NULL);
  std::cout<<"dec id "<<pInDec->id<<std::endl;
  
  std::cout<<"before codec open"<<std::endl;
  if (avcodec_open2(pInCodecCtx, pInDec,NULL)<0){
    std::cout<<"open dec failed."<<std::endl;
    return -1;
  }
  
  display = new DisplayWindow(pInCodecCtx->width, pInCodecCtx->height);
}

UdpCameraClient::UdpCameraClient(){
  
  av_register_all();
  avcodec_register_all();
  avformat_network_init();
  
  openInput();
}

int UdpCameraClient::play(){
  AVPacket packet;
  memset(&packet, 0, sizeof(AVPacket));
  
  AVFrame *srcframe;
  uint8_t *srcframe_buf;
  
  AVFrame *dstframe;
  uint8_t *dstframe_buf;
  
  srcframe = av_frame_alloc();
  assert(srcframe!=NULL);
  int srcBufSize = av_image_get_buffer_size(pInCodecCtx->pix_fmt, pInCodecCtx->width, pInCodecCtx->height, 16);
  srcframe_buf = (uint8_t *)malloc(srcBufSize);
  av_image_fill_arrays(srcframe->data, srcframe->linesize, srcframe_buf, pInCodecCtx->pix_fmt, pInCodecCtx->width, pInCodecCtx->height,16);
  
  dstframe = av_frame_alloc();
  assert(dstframe != NULL);
  int dstBufSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pInCodecCtx->width, pInCodecCtx->height, 16);
  dstframe_buf = (uint8_t *)malloc(dstBufSize);
  av_image_fill_arrays(dstframe->data, dstframe->linesize, dstframe_buf, AV_PIX_FMT_YUV420P, pInCodecCtx->width, pInCodecCtx->height, 16);
  
  SwsContext *swsctx = sws_getContext(pInCodecCtx->width, pInCodecCtx->height, pInCodecCtx->pix_fmt,
		 pInCodecCtx->width, pInCodecCtx->height, AV_PIX_FMT_YUV420P,
		 SWS_BILINEAR, NULL, NULL, NULL
  );
  
  int got_picture;
  
  while (true){
    std::cout<<"decode ............"<<std::endl;
    av_read_frame(pInFormatCtx, &packet);
    if (avcodec_decode_video2(pInCodecCtx, srcframe, &got_picture, &packet)<0){
      std::cout<<"decode failed, now return. "<<std::endl;
      return -1;
    }
    
    if (got_picture){
      std::cout<<"got picture. "<<std::endl;
      sws_scale(swsctx, srcframe->data, srcframe->linesize,0, pInCodecCtx->height, dstframe->data, dstframe->linesize);
      display->refresh(dstframe_buf);
    }
  }
}
#endif
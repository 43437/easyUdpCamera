#ifndef __UDP_CAMERA__
#define __UDP_CAMERA__
#include <iostream>
#include <cassert>

extern "C"{
  
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <unistd.h>
}

class UdpCamera{
  
  const char *inputVideoSize = "640x360";
  bool isStop = false;
  const char *input_name = "video4linux2";
  const char *file_name = "/dev/video0";
  enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_YUV420P;
  AVStream *pOStream;
  AVCodecContext *pInCodecCtx = NULL;
  AVFormatContext *pInFmtCtx = NULL;
  AVFormatContext *pMuxFmtCtx = NULL;
  SwsContext *sws_ctx = NULL;
  
  AVCodecContext *pMuxCodecCtx=NULL;
  AVCodec *pMuxCodec= NULL;
  int openInput();
  int outputMux();
public:
  UdpCamera();
  int stream();
};

int UdpCamera::openInput(){
  AVInputFormat *pInFmt = av_find_input_format(input_name);
  if (pInFmt == NULL){
    std::cout<<"can not find input format. "<<std::endl;
    return -1;
  }
  
  const char *videosize1 = "640x360";
  AVDictionary *option;
  av_dict_set(&option, "video_size", videosize1, 0);
  
  if (avformat_open_input(&pInFmtCtx, file_name, pInFmt, &option)<0){
    std::cout<<"can not open input file."<<std::endl;
    return -1;
  }
  
  int videoindex = -1;
  for (int i=0;i<pInFmtCtx->nb_streams;i++){
    if(pInFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
      videoindex = i;
      break;
    }
  }
  
  if(videoindex == -1){
    std::cout<<"find video stream failed, now return."<<std::endl;
    return -1;
  }
  
  pInCodecCtx = pInFmtCtx->streams[videoindex]->codec;
  
  std::cout<<"picture width height format"<<pInCodecCtx->width<<pInCodecCtx->height<<pInCodecCtx->pix_fmt<<std::endl;
  
  sws_ctx = sws_getContext(pInCodecCtx->width, pInCodecCtx->height, pInCodecCtx->pix_fmt
    ,pInCodecCtx->width, pInCodecCtx->height, dst_pix_fmt, SWS_BILINEAR, NULL, NULL, NULL);
}

int UdpCamera::stream(){
  uint8_t *src_data[4];
  uint8_t *dst_data[4];
  int src_linesize[4];
  int dst_linesize[4];

  AVPacket outPkt;
  AVPacket *pInPkt;
  int got_picture;
  
  AVFrame *pOutFrame = av_frame_alloc();
  if (!pOutFrame){
    std::cout<<"out frame alloc failed, now return. "<<std::endl;
    return -1;
  }
  
  int outBufSize = avpicture_get_size(pMuxCodecCtx->pix_fmt, pMuxCodecCtx->width, pMuxCodecCtx->height);
  int dst_bufsize = av_image_alloc(dst_data, dst_linesize, pInCodecCtx->width, pInCodecCtx->height, dst_pix_fmt, 16);
  int src_bufsize = av_image_alloc(src_data, src_linesize, pInCodecCtx->width, pInCodecCtx->height, pInCodecCtx->pix_fmt, 16);
  
  std::cout<<"dst buf size "<<dst_bufsize<<std::endl;
  std::cout<<"out buf size "<<outBufSize<<std::endl;
  uint8_t *pFrame_buf = (uint8_t *)av_malloc(outBufSize);
  if (!pFrame_buf){
    std::cout<<"frame buf alloc failed. "<<std::endl;
    return -1;
  }
  avpicture_fill((AVPicture *)pOutFrame, pFrame_buf, pMuxCodecCtx->pix_fmt, pMuxCodecCtx->width, pMuxCodecCtx->height);
  
  
  //Write header
  avformat_write_header(pMuxFmtCtx, NULL);
  
  int i=0;
  while (!isStop){
    i++;
    //read data
    pInPkt = (AVPacket *)av_malloc(sizeof(AVPacket));
    av_read_frame(pInFmtCtx, pInPkt);
    
    //fill out data
    memcpy(src_data[0], pInPkt->data, pInPkt->size);
    sws_scale(sws_ctx, src_data, src_linesize, 0, pInCodecCtx->height, dst_data, dst_linesize);
    memcpy(pFrame_buf, dst_data[0], dst_bufsize);
    std::cout<<"dst bufsize "<<dst_bufsize<<std::endl;
    
    memset(&outPkt, 0, sizeof(AVPacket));
    av_init_packet(&outPkt);
    //encode data 
    pOutFrame->pts=i*(pMuxCodecCtx->time_base.den)/((pMuxCodecCtx->time_base.num)*25);
    int ret = avcodec_encode_video2(pMuxCodecCtx, &outPkt,pOutFrame, &got_picture);  
    if(ret < 0){  
	std::cout<<"Encode Error.\n"<<std::endl;
	return -1;  
    }
    
    //flush data
    if (got_picture==1){  
      std::cout<<"ecode success, write to file."<<std::endl;
      av_interleaved_write_frame(pMuxFmtCtx, &outPkt);
    }  
  }
  
  av_free_packet(&outPkt); 
  
  //Write Trailer  
  av_write_trailer(pMuxFmtCtx);  

  //end encode
  std::cout<<"Encode Successful.\n"<<std::endl;  

  if (pOStream){  
      avcodec_close(pOStream->codec);  
      av_free(pOutFrame);  
      av_free(pFrame_buf);  
  }  
  avio_close(pInFmtCtx->pb);  
  avformat_close_input(&pInFmtCtx);
  
  avformat_free_context(pInFmtCtx);  
  av_free_packet(pInPkt);
  av_freep(&dst_data[0]);
  sws_freeContext(sws_ctx);
}

int UdpCamera::outputMux(){
  
  const char* out_file = "udp://192.168.1.104:8989";
  AVOutputFormat *pMuxOutFmt;
  pMuxOutFmt = av_guess_format("mpegts", NULL, NULL);
  pMuxCodec = avcodec_find_encoder(pMuxOutFmt->video_codec);
  
  assert(pMuxCodec != NULL);
  if (!pMuxOutFmt){
    std::cout<<"mpegts format guess failed. "<<std::endl;
  }
  pMuxFmtCtx = avformat_alloc_context();
  if (avio_open(&pMuxFmtCtx->pb, out_file, AVIO_FLAG_WRITE) < 0){
    std::cout<<"udp io open failed. "<<std::endl;
    return -1;
  }
  pMuxFmtCtx->oformat = pMuxOutFmt;
  
  AVStream *pMuxStream = avformat_new_stream(pMuxFmtCtx, NULL);
  assert(pMuxStream != NULL);
  pMuxCodecCtx = pMuxStream->codec;
  pMuxCodecCtx->codec_id = pMuxOutFmt->video_codec;
  pMuxCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
  pMuxCodecCtx->pix_fmt = dst_pix_fmt;
  pMuxCodecCtx->width = pInCodecCtx->width;
  pMuxCodecCtx->height = pInCodecCtx->height;
  pMuxCodecCtx->time_base.num =1;
  pMuxCodecCtx->time_base.den = 25;
  pMuxCodecCtx->bit_rate = 400000;
  pMuxCodecCtx->gop_size=250;
  pMuxCodecCtx->qmin = 10;
  pMuxCodecCtx->qmax = 51;
  
  if (avcodec_open2(pMuxCodecCtx, pMuxCodec, NULL)<0){
    std::cout<<"mux codec open failed."<<std::endl;
    return -1;
  }
  return 0;
}

UdpCamera::UdpCamera(){
  
  av_register_all();
  avformat_network_init();
  avcodec_register_all();
  avdevice_register_all();
  
  openInput();
  outputMux();
}
#endif
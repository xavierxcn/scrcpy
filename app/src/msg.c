//
// Created by xiang on 2024/2/9.
//

#include <stdio.h>
#include <string.h>
#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
#include <libavformat/avformat.h>

#define MAX_BUF_SIZE 1024

int msg_pub_frame(AVFrame *frame);

static int pub_socket = -1;

static int get_pub_socket() {
    if (pub_socket == -1) {
        int rv;
        pub_socket = nn_socket(AF_SP, NN_PUB);
        if (pub_socket < 0) {
            fprintf(stderr, "nn_socket() failed: %s\n", nn_strerror(pub_socket));
            return -1;
        }

        rv = nn_bind(pub_socket, "tcp://*:5555");
        if (rv < 0) {
            fprintf(stderr, "nn_bind() failed: %s\n", nn_strerror(pub_socket));
            return -1;
        }
    }

    return pub_socket;
}

static uint8_t* frame_as_jpeg(AVFrame *frame, int *out_size) {
    AVOutputFormat *pOutputFmt;
    AVFormatContext *pFormatCtx;
    AVStream *pStream;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;

    // 分配一个输出媒体上下文
    pFormatCtx = avformat_alloc_context();

    // 猜测文件格式
    pOutputFmt = av_guess_format("mjpeg", NULL, NULL);
    if (!pOutputFmt) {
        printf("Could not guess image format\n");
        return NULL;
    }

    pFormatCtx->oformat = pOutputFmt;

    // 创建一个新的视频流
    pStream = avformat_new_stream(pFormatCtx, 0);
    if (!pStream) {
        printf("Failed to create new stream\n");
        return NULL;
    }

    pCodecCtx = pStream->codecpar;
    pCodecCtx->codec_id = AV_CODEC_ID_MJPEG;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    pCodecCtx->width = frame->width;
    pCodecCtx->height = frame->height;
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;

    // 查找编解码器
    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec) {
        printf("Codec not found\n");
        return NULL;
    }

    // 打开编解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec\n");
        return NULL;
    }

    // 创建一个动态内存缓冲区
    AVIOContext *avio_ctx = NULL;
    if (avio_open_dyn_buf(&avio_ctx) < 0) {
        printf("Could not open dynamic buffer\n");
        return NULL;
    }

    pFormatCtx->pb = avio_ctx;

    // 写文件头
    avformat_write_header(pFormatCtx, NULL);

    int got_output;
    AVPacket pkt = {0};
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;

    // 编码帧
    // send the frame for encoding
    if (avcodec_send_frame(pCodecCtx, frame) < 0) {
        printf("Error sending frame for encoding\n");
        return NULL;
    }

    // receive the packet
    int ret = avcodec_receive_packet(pCodecCtx, &pkt);
    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
        printf("Error receiving packet\n");
        return NULL;
    }


    if (ret >= 0) {
        av_write_frame(pFormatCtx, &pkt);
        av_packet_unref(&pkt);
    }

    // 写文件尾
    av_write_trailer(pFormatCtx);

    // 获取动态缓冲区的数据和大小
    uint8_t *buffer = NULL;
    *out_size = avio_close_dyn_buf(pFormatCtx->pb, &buffer);

    // 关闭编解码器和输出文件
    avcodec_close(pStream->codec);
    avformat_free_context(pFormatCtx);

    return buffer;
}

int msg_pub_frame(AVFrame *frame) {
    int sock;

    sock = get_pub_socket();
    char buf[MAX_BUF_SIZE];

    int size;
    uint8_t *data = frame_as_jpeg(frame, &size);
    if (data == NULL) {
        return -1;
    }

    int ret = nn_send(sock, data, size, 0);
    if (ret < 0) {
        fprintf(stderr, "nn_send() failed: %s\n", nn_strerror(ret));
        return -1;
    }

    return 0;
}
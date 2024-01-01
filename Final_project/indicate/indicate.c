#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

int main(int argc, char *argv[]) {
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "無法初始化SDL: %s\n", SDL_GetError());
        return -1;
    }

    // 打開視頻文件
    AVFormatContext *pFormatCtx = NULL;
    // if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0) {
    //     fprintf(stderr, "無法打開視頻文件\n");
    //     return -1;
    // }

    // 指定視頻文件的路徑
    if (avformat_open_input(&pFormatCtx, "movie.mp4", NULL, NULL) != 0) {
        fprintf(stderr, "無法打開視頻文件\n");
        return -1;
    }

    // 獲取視頻流信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        fprintf(stderr, "無法獲取流信息\n");
        return -1;
    }

    // 找到視頻流
    int videoStream = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    if (videoStream == -1) {
        fprintf(stderr, "找不到視頻流\n");
        return -1;
    }

    // 獲取視頻解碼器
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        fprintf(stderr, "找不到視頻解碼器\n");
        return -1;
    }

    // 打開解碼器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        fprintf(stderr, "無法打開視頻解碼器\n");
        return -1;
    }

    // 創建SDL窗口和渲染器
    SDL_Window *window = SDL_CreateWindow("FFmpeg SDL視頻播放器", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, pCodecCtx->width, pCodecCtx->height, SDL_WINDOW_OPENGL);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);

    // 創建解碼帧
    AVFrame *pFrame = av_frame_alloc();
    AVPacket packet;
    SDL_Event event;

    double slowdownFactor = 1.; //數字越代越慢, 1.0代表正常速度

    while (1) {
        if (av_read_frame(pFormatCtx, &packet) < 0)
            break;
        if (packet.stream_index == videoStream) {
            avcodec_send_packet(pCodecCtx, &packet);
            av_packet_unref(&packet);
            while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                ////////////變慢視頻////////////
                // 计算每一帧的等待时间
                int delayMilliseconds = (int)(1000.0 / (pCodecCtx->framerate.num / (double)pCodecCtx->framerate.den) * slowdownFactor);

                // 等待指定的时间
                SDL_Delay(delayMilliseconds);
                ///////////////////////////
                SDL_UpdateYUVTexture(texture, NULL, pFrame->data[0], pFrame->linesize[0], pFrame->data[1], pFrame->linesize[1], pFrame->data[2], pFrame->linesize[2]);
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
                SDL_PollEvent(&event);
                if (event.type == SDL_QUIT)
                    break;
            }
        }
    }

    // 清理資源
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

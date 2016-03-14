#ifndef _player_def_H
#define _player_def_H

#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avstring.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

#ifdef __MINGW32__
#undef main /* Prevents SDL from overriding main() */
#endif

#include <stdio.h>
#include <assert.h>

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 28, 1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

#define SDL_AUDIO_MIN_BUFFER_SIZE 512
#define SDL_AUDIO_BUFFER_SIZE 4800
#define MAX_AUDIO_FRAME_SIZE 192000
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE_MAX, SUBPICTURE_QUEUE_SIZE))

extern int quit;

typedef int bool;
#define true 1
#define false 0

typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

typedef struct AudioState {
    SDL_Thread *read_tid;
    AVFormatContext *format_ctx;
    AVCodecContext *audio_codec_ctx_orig;
    AVCodecContext *audio_codec_ctx;
    AVCodec *audio_codec;
    PacketQueue audio_queue;
    int audio_stream_index;
    int duration;  // total secs
    double audio_clock;  // current playing clock
    uint8_t silence_buf[SDL_AUDIO_MIN_BUFFER_SIZE];
    int audio_volume;
    int muted;
    int paused;
} AudioState;

typedef struct CPlayer {
    /* audio context */
    AudioState *is;
    char *input_filename;
} CPlayer;

#endif // end _player_def_H

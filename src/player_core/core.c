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

static void dump_metadata(void *ctx, AVDictionary *m, const char *indent)
{
    if (m && !(av_dict_count(m) == 1 && av_dict_get(m, "language", NULL, 0))) {
        AVDictionaryEntry *tag = NULL;

        av_log(ctx, AV_LOG_INFO, "%sMetadata:\n", indent);
        while ((tag = av_dict_get(m, "", tag, AV_DICT_IGNORE_SUFFIX)))
            if (strcmp("language", tag->key)) {
                printf("tag->key: %s\n", tag->key);
                printf("tag->value : %s\n", tag->value);
                const char *p = tag->value;
                av_log(ctx, AV_LOG_INFO,
                        "%s  %-16s: ", indent, tag->key);
                while (*p) {
                    char tmp[256];
                    size_t len = strcspn(p, "\x8\xa\xb\xc\xd");
                    av_strlcpy(tmp, p, FFMIN(sizeof(tmp), len+1));
                    av_log(ctx, AV_LOG_INFO, "%s", tmp);
                    p += len;
                    if (*p == 0xd) av_log(ctx, AV_LOG_INFO, " ");
                    if (*p == 0xa) av_log(ctx, AV_LOG_INFO, "\n%s  %-16s: ", indent, "");
                    if (*p) p++;
                }
                av_log(ctx, AV_LOG_INFO, "\n");
            }
    }
}

typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

PacketQueue audio_queue;

int quit = 0;

void packet_queue_init(PacketQueue *pkt_q) {
    memset(pkt_q, 0, sizeof(PacketQueue));
    pkt_q->mutex = SDL_CreateMutex();
    pkt_q->cond = SDL_CreateCond();
}

int packet_queue_put(PacketQueue *pkt_q, AVPacket *pkt) {
    AVPacketList *pkt1;
    pkt1 = (AVPacketList *)av_malloc(sizeof(AVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    SDL_LockMutex(pkt_q->mutex);

    if (!pkt_q->last_pkt)
        pkt_q->first_pkt = pkt1;
    else
        pkt_q->last_pkt->next = pkt1;
    pkt_q->last_pkt = pkt1;
    pkt_q->nb_packets++;
    pkt_q->size += pkt1->pkt.size;
    // Send signal to queue get function
    SDL_CondSignal(pkt_q->cond);

    SDL_UnlockMutex(pkt_q->mutex);
    return 0;
}

// if block set to True, this func will wait for SDL_CondSignal
int packet_queue_get(PacketQueue *pkt_q, AVPacket *pkt, int block) {
    AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(pkt_q->mutex);

    for (;;) {

        if (quit) {
            ret = -1;
            break;
        }

        pkt1 = pkt_q->first_pkt;
        if (pkt1) {
            pkt_q->first_pkt = pkt1->next;
            if (!pkt_q->first_pkt)
                pkt_q->last_pkt = NULL;
            pkt_q->nb_packets--;
            pkt_q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(pkt_q->cond, pkt_q->mutex);
        }
    }
    SDL_UnlockMutex(pkt_q->mutex);
    return ret;
}

static int audio_resampling(AVCodecContext *audio_decode_ctx,
                            AVFrame *audio_decode_frame,
                            enum AVSampleFormat out_sample_fmt,
                            int out_channels,
                            int out_sample_rate,
                            uint8_t *out_buf)
{
    SwrContext *swr_ctx = NULL;
    int ret = 0;
    int64_t in_channel_layout = audio_decode_ctx->channel_layout;
    int64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    int out_nb_channels = 0;
    int out_linesize = 0;
    int in_nb_samples = 0;
    int out_nb_samples = 0;
    int max_out_nb_samples = 0;
    uint8_t **resampled_data = NULL;
    int resampled_data_size = 0;

    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        printf("swr_alloc error\n");
        return -1;
    }

    in_channel_layout = (audio_decode_ctx->channels ==
                     av_get_channel_layout_nb_channels(audio_decode_ctx->channel_layout)) ?
                     audio_decode_ctx->channel_layout :
                     av_get_default_channel_layout(audio_decode_ctx->channels);
    if (in_channel_layout <=0) {
        printf("in_channel_layout error\n");
        return -1;
    }

    if (out_channels == 1) {
        out_channel_layout = AV_CH_LAYOUT_MONO;
    } else if (out_channels == 2) {
        out_channel_layout = AV_CH_LAYOUT_STEREO;
    } else {
        out_channel_layout = AV_CH_LAYOUT_SURROUND;
    }

    in_nb_samples = audio_decode_frame->nb_samples;
    if (in_nb_samples <=0) {
        printf("in_nb_samples error\n");
        return -1;
    }

    av_opt_set_int(swr_ctx, "in_channel_layout", in_channel_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", audio_decode_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", audio_decode_ctx->sample_fmt, 0);

    av_opt_set_int(swr_ctx, "out_channel_layout", out_channel_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", out_sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", out_sample_fmt, 0);

    if ((ret = swr_init(swr_ctx)) < 0) {
        printf("Failed to initialize the resampling context\n");
        return -1;
    }

    max_out_nb_samples = out_nb_samples = av_rescale_rnd(in_nb_samples,
                                                         out_sample_rate,
                                                         audio_decode_ctx->sample_rate,
                                                         AV_ROUND_UP);

    if (max_out_nb_samples <= 0) {
        printf("av_rescale_rnd error\n");
        return -1;
    }

    out_nb_channels = av_get_channel_layout_nb_channels(out_channel_layout);

    ret = av_samples_alloc_array_and_samples(&resampled_data, &out_linesize, out_nb_channels, out_nb_samples, out_sample_fmt, 0);
    if (ret < 0) {
        printf("av_samples_alloc_array_and_samples error\n");
        return -1;
    }

    out_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, audio_decode_ctx->sample_rate) + in_nb_samples,
                                    out_sample_rate, audio_decode_ctx->sample_rate, AV_ROUND_UP);
    if (out_nb_samples <= 0) {
        printf("av_rescale_rnd error\n");
        return -1;
    }

    if (out_nb_samples > max_out_nb_samples) {
        av_free(resampled_data[0]);
        ret = av_samples_alloc(resampled_data, &out_linesize, out_nb_channels, out_nb_samples, out_sample_fmt, 1);
        max_out_nb_samples = out_nb_samples;
    }

    if (swr_ctx) {
        ret = swr_convert(swr_ctx, resampled_data, out_nb_samples,
                          (const uint8_t **)audio_decode_frame->data, audio_decode_frame->nb_samples);
        if (ret < 0) {
            printf("swr_convert_error\n");
            return -1;
        }

        resampled_data_size = av_samples_get_buffer_size(&out_linesize, out_nb_channels, ret, out_sample_fmt, 1);
        if (resampled_data_size < 0) {
            printf("av_samples_get_buffer_size error\n");
            return -1;
        }
    } else {
        printf("swr_ctx null error\n");
        return -1;
    }

    memcpy(out_buf, resampled_data[0], resampled_data_size);

    if (resampled_data) {
        av_freep(&resampled_data[0]);
    }
    av_freep(&resampled_data);
    resampled_data = NULL;

    if (swr_ctx) {
        swr_free(&swr_ctx);
    }
    return resampled_data_size;
}

int audio_decode_frame(AVCodecContext *audio_codec_ctx, uint8_t *audio_buf, int buf_size){
    static AVPacket pkt;
    static uint8_t *audio_pkt_data = NULL;
    static int audio_pkt_size = 0;
    static AVFrame frame;

    int len1, data_size = 0;

    for (;;) {
        while (audio_pkt_size > 0) {
            int got_frame = 0;
            len1 = avcodec_decode_audio4(audio_codec_ctx, &frame, &got_frame, &pkt);
            if (len1 < 0) {
                // error, skip frame
                audio_pkt_size = 0;
                break;
            }
            audio_pkt_data += len1;
            audio_pkt_size -= len1;
            data_size = 0;
            if (got_frame) {
                // resamplling and copy buf to audio_buf
                data_size = audio_resampling(audio_codec_ctx, &frame, AV_SAMPLE_FMT_S16, frame.channels, frame.sample_rate, audio_buf);
                assert(data_size <= buf_size);
            }
            if (data_size <= 0) {
                // No data yet, get more frames
                continue;
            }
            // We have data, return it and come back for more later
            return data_size;
        }

        if (pkt.data) {
            av_packet_unref(&pkt);
        }

        if (quit) {
            return -1;
        }

        if (packet_queue_get(&audio_queue, &pkt, 1) < 0) {
            return -1;
        }
        audio_pkt_data = pkt.data;
        audio_pkt_size = pkt.size;
    }
}

void audio_callback(void *userdata, Uint8 *stream, int len){
    AVCodecContext *audio_codec_ctx = (AVCodecContext *)userdata;
    int len1, audio_size;

    static uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
    static unsigned int audio_buf_size = 0;  // one time got buffer size
    static unsigned int audio_buf_index = 0; // one time copied buffer length

    while (len > 0) {
        if (audio_buf_index >= audio_buf_size) {
            // We have already sent all our data; get more */
            audio_size = audio_decode_frame(audio_codec_ctx, audio_buf, sizeof(audio_buf));
            if (audio_size < 0) {
                /* If error, output silence */
                audio_buf_size = 1024; // eh...
                memset(audio_buf, 0, audio_buf_size);
            } else {
                audio_buf_size = audio_size;
            }
            audio_buf_index = 0;
        }
        len1 = audio_buf_size - audio_buf_index;
        if(len1 > len)
            len1 = len;
        memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
    }
}

int main(int argc, char *argv[])
{
    AVFormatContext *format_ctx = NULL;
    unsigned int i;
    int audio_stream_index;
    AVPacket packet;

    AVCodecContext *audio_codec_ctx_orig = NULL;
    AVCodecContext *audio_codec_ctx = NULL;
    AVCodec *audio_codec = NULL;

    SDL_Event event;
    SDL_AudioSpec wanted_spec, spec;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        exit(1);
    }

    // Register all formats and codecs
    av_register_all();

    // Init SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    // Open audio file
    if (avformat_open_input(&format_ctx, argv[1], NULL, NULL) != 0)
        return -1;  // Error

    // Retrieve stream information
    if (avformat_find_stream_info(format_ctx, NULL) < 0)
        return -1;

    // Dump information about file onto standard error
    /* av_dump_format(format_ctx, 0, argv[1], 0); */
    dump_metadata(NULL, format_ctx->metadata, "    ");

    // Find the first audio stream
    audio_stream_index = -1;
    for (i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index < 0) {
            audio_stream_index = i;
        }
    }
    if (audio_stream_index == -1)
        return -1;

    audio_codec_ctx_orig = format_ctx->streams[audio_stream_index]->codec;
    audio_codec = avcodec_find_decoder(audio_codec_ctx_orig->codec_id);
    if (!audio_codec) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1;
    }

    // Copy context
    audio_codec_ctx = avcodec_alloc_context3(audio_codec);
    if (avcodec_copy_context(audio_codec_ctx, audio_codec_ctx_orig) != 0) {
        fprintf(stderr, "Couldn't copy codec context\n");
        return -1;
    }

    // Set audio settings from codec info
    wanted_spec.freq = audio_codec_ctx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = audio_codec_ctx->channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = audio_codec_ctx;

    av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n",wanted_spec.channels, wanted_spec.freq, SDL_GetError());

    if (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
        fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
        return -1;
    }

    avcodec_open2(audio_codec_ctx, audio_codec, NULL);

    packet_queue_init(&audio_queue);
    SDL_PauseAudio(0);

    // Read frames and put to audio_queue
    while (av_read_frame(format_ctx, &packet) >= 0) {
        // Check wheather this frame is audio frame or not (maby the frame is video frame)
        if (packet.stream_index == audio_stream_index) {
            packet_queue_put(&audio_queue, &packet);
        } else {
            av_packet_unref(&packet); // Free the packet
        }
    }

    // Main func block here waiting for audio playback finish
    SDL_WaitEvent(&event);

    avcodec_close(audio_codec_ctx_orig);
    avcodec_close(audio_codec_ctx);

    // Close file
    avformat_close_input(&format_ctx);

    return 0;
}

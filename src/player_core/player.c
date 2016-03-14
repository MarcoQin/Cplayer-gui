#include "player.h"

int quit = 0;

CPlayer *global_cplayer_ctx;

static bool g_ffmpeg_global_inited = false;

static void dump_metadata(void *ctx, AVDictionary *m, const char *indent) {
    if (m && !(av_dict_count(m) == 1 && av_dict_get(m, "language", NULL, 0))) {
        AVDictionaryEntry *tag = NULL;

        av_log(ctx, AV_LOG_INFO, "%sMetadata:\n", indent);
        while ((tag = av_dict_get(m, "", tag, AV_DICT_IGNORE_SUFFIX)))
            if (strcmp("language", tag->key)) {
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

void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}

static void packet_queue_flush(PacketQueue *q) {
    AVPacketList *pkt, *pkt1;
    SDL_LockMutex(q->mutex);
    for (pkt = q->first_pkt; pkt != NULL; pkt=pkt1) {
        pkt1 = pkt->next;
        /* av_free_packet(&pkt->pkt); */
        av_packet_unref(&pkt->pkt);
        av_free(pkt);
    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    SDL_UnlockMutex(q->mutex);
}

static void packet_queue_destory(PacketQueue *q) {
    packet_queue_flush(q);
    if (q->mutex)
        SDL_DestroyMutex(q->mutex);
    if (q->cond)
        SDL_DestroyCond(q->cond);
}

void global_init() {
    if (g_ffmpeg_global_inited)
        return;
    // Register all formats and codecs
    av_register_all();

    // Init SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    g_ffmpeg_global_inited = true;
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

int audio_decode_frame(CPlayer *cp, uint8_t *audio_buf, int buf_size){
    AudioState *is = cp->is;
    static AVPacket pkt;
    static uint8_t *audio_pkt_data = NULL;
    static int audio_pkt_size = 0;
    static AVFrame frame;

    int len1, data_size = 0;

    for (;;) {
        while (audio_pkt_size > 0) {
            int got_frame = 0;
            len1 = avcodec_decode_audio4(is->audio_codec_ctx, &frame, &got_frame, &pkt);
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
                data_size = audio_resampling(is->audio_codec_ctx, &frame, AV_SAMPLE_FMT_S16, frame.channels, frame.sample_rate, audio_buf);
                assert(data_size <= buf_size);
            }
            if (data_size <= 0) {
                // No data yet, get more frames
                continue;
            }
            int n = 2 * frame.channels;
            is->audio_clock += (double)data_size / (double)(n * frame.sample_rate);
            // We have data, return it and come back for more later
            return data_size;
        }

        if (pkt.data) {
            av_packet_unref(&pkt);
        }

        if (quit) {
            return -1;
        }

        if (packet_queue_get(&is->audio_queue, &pkt, 1) < 0) {
            return -1;
        }
        audio_pkt_data = pkt.data;
        audio_pkt_size = pkt.size;
        /* if update, update the audio clock w/pts */
        if (pkt.pts != AV_NOPTS_VALUE) {
            is->audio_clock = av_q2d(is->audio_codec_ctx->pkt_timebase) * pkt.pts;
        }
    }
}

void audio_callback(void *userdata, Uint8 *stream, int len){
    CPlayer *cp = (CPlayer *)userdata;
    int len1, audio_size;

    static uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
    static unsigned int audio_buf_size = 0;  // one time got buffer size
    static unsigned int audio_buf_index = 0; // one time copied buffer length

    while (len > 0) {
        if (audio_buf_index >= audio_buf_size) {
            // We have already sent all our data; get more */
            audio_size = audio_decode_frame(cp, audio_buf, sizeof(audio_buf));
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


static int audio_open(CPlayer *arg) {
    CPlayer *cp = arg;
    AudioState *is = cp->is;
    SDL_AudioSpec wanted_spec, spec;

    // Set audio settings from codec info
    wanted_spec.freq = is->audio_codec_ctx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = is->audio_codec_ctx->channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = cp;

    av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n",wanted_spec.channels, wanted_spec.freq, SDL_GetError());

    if (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
        fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
        return -1;
    }

    SDL_PauseAudio(0);
    return 0;
}

static int audio_close() {
    SDL_CloseAudio();
    return 0;
}

static int read_thread(void *arg) {
    CPlayer *cp = (CPlayer *)arg;
    AudioState *is = cp->is;
    AVPacket packet;

    // Open audio file
    if (avformat_open_input(&is->format_ctx, cp->input_filename, NULL, NULL) != 0)
        return -1;  // Error

    // Retrieve stream information
    if (avformat_find_stream_info(is->format_ctx, NULL) < 0)
        return -1;

    is->audio_stream_index = -1;
    unsigned int i;
    for (i = 0; i < is->format_ctx->nb_streams; i++) {
        if (is->format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && is->audio_stream_index < 0) {
            is->audio_stream_index = i;
        }
    }
    if (is->audio_stream_index == -1)
        return -1;

    // Get metadata and others
    dump_metadata(NULL, is->format_ctx->metadata, "    ");
    /* av_dump_format(is->format_ctx, 0, cp->input_filename, 0); */

    if (is->format_ctx->duration != AV_NOPTS_VALUE) {
        int hours, mins, secs, us;
        int64_t duration = is->format_ctx->duration + (is->format_ctx->duration <= INT64_MAX - 5000 ? 5000 : 0);
        secs  = duration / AV_TIME_BASE;
        is->duration = secs;
        av_log(NULL, AV_LOG_INFO, "seconds: %d\n", secs);
        us    = duration % AV_TIME_BASE;
        mins  = secs / 60;
        secs %= 60;
        hours = mins / 60;
        mins %= 60;
        av_log(NULL, AV_LOG_INFO, "%02d:%02d:%02d.%02d\n", hours, mins, secs,
                (100 * us) / AV_TIME_BASE);
    }

    is->audio_codec_ctx_orig = is->format_ctx->streams[is->audio_stream_index]->codec;
    is->audio_codec = avcodec_find_decoder(is->audio_codec_ctx_orig->codec_id);
    if (!is->audio_codec) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1;
    }

    // Copy context
    is->audio_codec_ctx = avcodec_alloc_context3(is->audio_codec);
    if (avcodec_copy_context(is->audio_codec_ctx, is->audio_codec_ctx_orig) != 0) {
        fprintf(stderr, "Couldn't copy codec context\n");
        return -1;
    }

    // Open audio device
    audio_open(cp);

    avcodec_open2(is->audio_codec_ctx, is->audio_codec, NULL);
    // Read frames and put to audio_queue
    while (av_read_frame(is->format_ctx, &packet) >= 0) {
        // Check wheather this frame is audio frame or not (maby the frame is video frame)
        if (packet.stream_index == is->audio_stream_index) {
            packet_queue_put(&is->audio_queue, &packet);
        } else {
            av_packet_unref(&packet); // Free the packet
        }
    }
    return 0;
}

static void stream_close(CPlayer *cp) {
    AudioState *is = cp->is;
    av_log(NULL, AV_LOG_DEBUG, "wait for read_tid\n");
    SDL_WaitThread(is->read_tid, NULL);
    audio_close();
    if (is->audio_codec_ctx_orig) {
        avcodec_close(is->audio_codec_ctx_orig);
    }
    if (is->audio_codec_ctx) {
        avcodec_close(is->audio_codec_ctx);
    }
    if (is->format_ctx) {
        avformat_close_input(&is->format_ctx);
        is->format_ctx = NULL;
    }
    packet_queue_destory(&is->audio_queue);
    av_free(is);
}

AudioState *stream_open(CPlayer *cp, const char *filename) {
    assert(!cp->is);
    AudioState *is;
    is = (AudioState *) av_mallocz(sizeof(AudioState));
    if (!is)
        return NULL;
    cp->input_filename = av_strdup(filename);
    packet_queue_init(&is->audio_queue);
    is->audio_volume = SDL_MIX_MAXVOLUME;
    is->muted = 0;
    cp->is = is;

    is->read_tid = SDL_CreateThread(read_thread, "read_thread", cp);
    if (!is->read_tid) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateThread(): %s\n", SDL_GetError());
        stream_close(cp);
        return NULL;
    }
    return is;
}

CPlayer *player_create() {
    CPlayer *cp = (CPlayer *) av_mallocz(sizeof(CPlayer));
    if (!cp)
        return NULL;
    global_cplayer_ctx = cp;
    return cp;
}

void player_destory(CPlayer *cp) {
    if (!cp)
        return;
    if (cp->is) {
        stream_close(cp);
        av_log(NULL, AV_LOG_WARNING, "destroy_cplayer: force stream_close()\n");
        cp->is = NULL;
    }
    av_free(cp);
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    AVPacketList *pkt1;
    pkt1 = (AVPacketList *)av_malloc(sizeof(AVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    SDL_LockMutex(q->mutex);

    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    // Send signal to queue get function
    SDL_CondSignal(q->cond);

    SDL_UnlockMutex(q->mutex);
    return 0;
}

// if block set to True, this func will wait for SDL_CondSignal
int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) {
    AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    for (;;) {

        if (quit) {
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}

void cp_pause_audio() {
    AudioState *is = global_cplayer_ctx->is;
    is->paused = !is->paused;
    SDL_PauseAudio(is->paused);
}

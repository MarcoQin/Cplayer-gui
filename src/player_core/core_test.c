#include "player.h"

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

int main(int argc, char *argv[])
{

    SDL_Event event;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        exit(1);
    }

    // Register all formats and codecs
    global_init();

    CPlayer *cp = player_create();
    if (!cp) {
        printf("player_create error\n");
        return -1;
    }
    AudioState *is = stream_open(cp, argv[1]);
    if (!is) {
        printf("is is null\n");
        return -1;
    }
    printf("is->audio_stream_index: %d\n", is->audio_stream_index);
    printf("here audio state\n");

    // Dump information about file onto standard error
    /* av_dump_format(format_ctx, 0, argv[1], 0); */
    SDL_Delay(1000);
    if(is->format_ctx->metadata)
        dump_metadata(NULL, is->format_ctx->metadata, "    ");

    // Main func block here waiting for audio playback finish
    SDL_WaitEvent(&event);

    avcodec_close(is->audio_codec_ctx_orig);
    avcodec_close(is->audio_codec_ctx);

    // Close file
    avformat_close_input(&is->format_ctx);

    return 0;
}

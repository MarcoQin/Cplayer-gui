#include "player.h"

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

    // Dump information about file onto standard error
    /* av_dump_format(format_ctx, 0, argv[1], 0); */

    /* SDL_Delay(3000); */
    /* cp_pause_audio(); */
    /* SDL_Delay(3000); */
    /* cp_pause_audio(); */
    // Main func block here waiting for audio playback finish
    SDL_WaitEvent(&event);
    printf("current pos: %f\n", is->audio_clock);

    player_destory(cp);

    return 0;
}

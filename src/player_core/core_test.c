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
    CPlayer *cp = cp_load_file(argv[1]);
    SDL_Delay(1000);
    cp_seek_audio(50);
    /* SDL_Delay(3000); */
    while (1) {
        if (!cp_is_stopping()) {
            SDL_Delay(500);
            continue;
        } else {
            cp = cp_load_file(argv[2]);
            break;
        }
    }
    /* cp = cp_load_file(argv[2]); */


    /* CPlayer *cp = player_create(); */
    /* if (!cp) { */
        /* printf("player_create error\n"); */
        /* return -1; */
    /* } */
    /* AudioState *is = stream_open(cp, argv[1]); */
    /* if (!is) { */
        /* printf("is is null\n"); */
        /* return -1; */
    /* } */

    // Dump information about file onto standard error
    /* av_dump_format(format_ctx, 0, argv[1], 0); */

    /* SDL_Delay(3000); */
    /* cp_pause_audio(); */
    /* SDL_Delay(3000); */
    /* cp_pause_audio(); */
    // Main func block here waiting for audio playback finish
    SDL_WaitEvent(&event);

    player_destory(cp);

    return 0;
}

#include <stdio.h>
#include "player_core.h"
#include "utils.h"
#include "db.h"

int playing_status = 0;   /* play status */
int alive = KILL; /* pipe alive false */

void init_player(char *path) {
    global_init();
    playing_status = PLAYING; /* playing status */
    alive = ALIVE;
    printf("init_player before load file\n");
    CPlayer *cp = cp_load_file(path);
    printf("init_player after load file\n");
}

void load_song(int id) {
    char *path = get_song_path(id);
    db_update_song_state(PLAYING, id);
    if (alive == ALIVE) {
        printf("**********normal before load file************\n");
        CPlayer *cp = cp_load_file(path);
        printf("*************normal after load file*************\n");
        playing_status = PLAYING; /* playing status */
    } else {
        init_player(path);
    }
}

void stop_song() {
    if (alive == ALIVE && playing_status != STOP) {
        cp_stop_audio();
    }
    playing_status = STOP; /* playing status */
}

void pause_song() {
    if (alive == ALIVE && playing_status != STOP) {
        playing_status = playing_status == PAUSE ? PLAYING : PAUSE; /* playing status */
        cp_pause_audio();
    }
}

double get_time_pos() {
    if (alive == ALIVE && playing_status != STOP) {
        return cp_get_current_time_pos();
    }
    return 0;
}

int get_time_length() {
    if (alive == ALIVE && playing_status != STOP) {
        return cp_get_time_length();
    }
    return 0;
}

void seek(double percent) {
    if (alive == ALIVE && playing_status == PLAYING) {
        cp_seek_audio(percent);
    }
}

void set_volume(int volume) {
    if (alive == ALIVE && playing_status == PLAYING) {
        printf("volumn: %d\n", volume);
        cp_set_volume(volume);
    }
}

void free_player() {
    cp_free_player();
    playing_status = STOP;
    alive = KILL;
}

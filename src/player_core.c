#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "player_core.h"
#include "utils.h"
#include "db.h"
#include "popen2.h"
#include <signal.h>

int playing_status = 0;   /* play status */
int alive = KILL; /* pipe alive false */
char buf[512];
char *FIFO = "/tmp/my_fifo";
int fd;
pid_t mplayer_pid;
int infp, outfp;

void init_player(char *path) {
    mkfifo(FIFO, 0666);
    char *base = "mplayer -slave -quiet -input file=/tmp/my_fifo \"";
    /* char *tail = "\" < /dev/null 2>&1 &"; */
    char *tail = "\"";
    char *result = merge_str(base, path, tail);
    mplayer_pid = popen2(result, &infp, &outfp);
    mplayer_pid += 2; // handle subprocess
    /* mplayer_pid += 1; // handle subprocess */
    playing_status = PLAYING; /* playing status */
    alive = ALIVE;
    free(result);
}

int is_alive(void) {
    int code = kill(mplayer_pid, 0);
    return code;
}

void load_song(int id) {
    char *path = get_song_path(id);
    db_update_song_state(PLAYING, id);
    if (alive == ALIVE && is_alive() == ALIVE) {
        char *base = "loadfile \"";
        char *tail = "\"\n";
        char *s = merge_str(base, path, tail);
        fd = open(FIFO, O_WRONLY);
        write(fd, s, strlen(s));
        close(fd);
        free(s);
        playing_status = PLAYING; /* playing status */
    } else {
        init_player(path);
    }
}

void stop_song() {
    if (alive == ALIVE && is_alive() == ALIVE) {
        char *s = "stop\n";
        fd = open(FIFO, O_WRONLY);
        write(fd, s, strlen(s));
        close(fd);
    }
    playing_status = STOP; /* playing status */
}

void pause_song() {
    if (alive == ALIVE && is_alive() == ALIVE) {
        char *s = "pause\n";
        fd = open(FIFO, O_WRONLY);
        write(fd, s, strlen(s));
        close(fd);
        playing_status = playing_status == PAUSE ? PLAYING : PAUSE; /* playing status */
    }
}

void get_time_percent_pos() {
    if (alive == ALIVE && is_alive() == ALIVE) {
        char *s = "get_percent_pos\n";
        fd = open(FIFO, O_WRONLY);
        write(fd, s, strlen(s));
        close(fd);
    }
}

void get_time_length() {
    if (alive == ALIVE && is_alive() == ALIVE) {
        char *s = "get_time_length\n";
        fd = open(FIFO, O_WRONLY);
        write(fd, s, strlen(s));
        close(fd);
    }
}

void seek(char *seconds) {
    if (alive == ALIVE && is_alive() == ALIVE && playing_status == PLAYING) {
        char *base = "seek ";
        char *tail = "\n";
        char *s = merge_str(base, seconds, tail);
        fd = open(FIFO, O_WRONLY);
        write(fd, s, strlen(s));
        close(fd);
        free(s);
    }
}

void free_player() {
    if (playing_status != STOP && is_alive() == ALIVE)
        stop_song();
    playing_status = STOP;
    alive = KILL;
    pclose2(mplayer_pid);
}

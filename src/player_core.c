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
int fifo_fd = 0;
int fifo_made = 0;
int fd;
pid_t mplayer_pid = 0;
int infp, outfp;

void init_player(char *path) {
    if (!fifo_made) {
        mkfifo(FIFO, 0666);
        fifo_made = 1;
    }
    char *base = "mplayer -slave -nolirc -msglevel all=-1:global=5 -quiet -input file=/tmp/my_fifo \"";
    char *tail = "\"";
    char *result = merge_str(base, path, tail);
    mplayer_pid = popen2(result, &infp, &outfp);
    mplayer_pid += 2; // handle subprocess
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
        if (!fifo_fd) {
            fifo_fd = open(FIFO, O_WRONLY | O_NONBLOCK);
        }
        write(fifo_fd, s, strlen(s));
        free(s);
        playing_status = PLAYING; /* playing status */
    } else {
        init_player(path);
    }
}

void stop_song() {
    if (alive == ALIVE && is_alive() == ALIVE) {
        char *s = "stop\n";
        if (!fifo_fd) {
            fifo_fd = open(FIFO, O_WRONLY | O_NONBLOCK);
        }
        write(fifo_fd, s, strlen(s));
    }
    playing_status = STOP; /* playing status */
}

void pause_song() {
    if (alive == ALIVE && is_alive() == ALIVE) {
        char *s = "pause\n";
        if (!fifo_fd) {
            fifo_fd = open(FIFO, O_WRONLY | O_NONBLOCK);
        }
        write(fifo_fd, s, strlen(s));
        playing_status = playing_status == PAUSE ? PLAYING : PAUSE; /* playing status */
    }
}

void get_time_percent_pos() {
    if (alive == ALIVE && is_alive() == ALIVE) {
        char *s = "get_percent_pos\n";
        if (!fifo_fd) {
            fifo_fd = open(FIFO, O_WRONLY | O_NONBLOCK);
        }
        int st = write(fifo_fd, s, strlen(s));
    }
}

void get_time_pos() {
    if (alive == ALIVE && is_alive() == ALIVE) {
        char *s = "get_time_pos\n";
        if (!fifo_fd) {
            fifo_fd = open(FIFO, O_WRONLY | O_NONBLOCK);
        }
        write(fifo_fd, s, strlen(s));
    }
}

void get_time_length() {
    if (alive == ALIVE && is_alive() == ALIVE) {
        char *s = "get_time_length\n";
        if (!fifo_fd) {
            fifo_fd = open(FIFO, O_WRONLY | O_NONBLOCK);
        }
        write(fifo_fd, s, strlen(s));
    }
}

void seek(double percent) {
    if (alive == ALIVE && is_alive() == ALIVE && playing_status == PLAYING) {
        char *base = "seek ";
        char *tail = " 1\n";
        char percent_str[15];
        snprintf(percent_str, 15, "%.4f", percent);
        char *s = merge_str(base, percent_str, tail);
        if (!fifo_fd) {
            fifo_fd = open(FIFO, O_WRONLY | O_NONBLOCK);
        }
        write(fifo_fd, s, strlen(s));
        free(s);
    }
}

void set_volume(int volume) {
    if (alive == ALIVE && is_alive() == ALIVE && playing_status == PLAYING) {
        char *base = "volume ";
        char *tail = " 1\n";
        char volume_str[3];
        snprintf(volume_str, 3, "%d", volume);
        char *s = merge_str(base, volume_str, tail);
        if (!fifo_fd) {
            fifo_fd = open(FIFO, O_WRONLY | O_NONBLOCK);
        }
        write(fifo_fd, s, strlen(s));
        free(s);
    }
}

void free_player() {
    if (playing_status != STOP && is_alive() == ALIVE)
        stop_song();
    playing_status = STOP;
    alive = KILL;
    if (mplayer_pid > 0) {
        pclose2(mplayer_pid);
    }
    if (fifo_fd > 0) {
        close(fifo_fd);
    }
}

#ifndef _player_core_h_
#define _player_core_h_
#include "player_core/player.h"
#define PLAYING 1
#define STOP 0
#define PAUSE 2
#define ALIVE 0
#define KILL 1
extern int mplayer_pid;
extern int playing_status;
extern int outfp;
extern int alive;
void init_player(char *path);
void load_song(int id);
void pause_song();
double get_time_pos();
int get_time_length();
void stop_song();
void seek(double percent);
void set_volume(int volume);
void free_player();
int is_alive(void);
#endif

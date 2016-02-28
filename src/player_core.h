#ifndef _player_core_h_
#define _player_core_h_
#define PLAYING 1
#define STOP 0
#define PAUSE 2
#define ALIVE 0
#define KILL 1
extern int mplayer_pid;
extern int playing_status;
extern int outfp;
void init_player(char *path);
void load_song(int id);
void pause_song();
void get_time_percent_pos();
void stop_song();
void seek(char *seconds);
void free_player();
#endif

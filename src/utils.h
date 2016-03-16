#ifndef _utils_h_
#define _utils_h_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavformat/avformat.h>
#include <ctype.h>
#include <libavutil/dict.h>
#include "db.h"

char *parse_p(char *s);
int index_of(char[], char[]);
char *extract_file_name(char *path);
int extract_song_id(const char *name);
char *merge_str(char *base, char *middle, char *tail);
void song_time_to_str(char *s, double total_length, double current_pos);
int extract_meta_data(char *file_path, SongInfo *info);
#endif

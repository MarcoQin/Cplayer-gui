#include "utils.h"

/* convert string's space to \space to fit bash command line */
char *parse_p(char *s) {
    int i = 0;
    int j = 0;
    int total = 0;
    while (s[i] != '\0') {
        if (s[i] == ' ')
            total += 1;
        i++;
    }
    i = 0;
    char *r = 0;
    r = (char *)malloc(1 + total + strlen(s));
    while (s[i] != '\0') {
        if (s[i] == ' ') {
            r[j] = '\\';
            r[++j] = s[i];
        } else {
            r[j] = s[i];
        }
        i++;
        j++;
    }
    r[++j] = '\0';
    return r;
}

int index_of(char src[], char str[]) {
    int i, j, firstOcc;
    i = 0, j = 0;

    while (src[i] != '\0') {

        while (src[i] != str[0] && src[i] != '\0')
            i++;

        if (src[i] == '\0')
            return (-1);

        firstOcc = i;

        while (src[i] == str[j] && src[i] != '\0' && str[j] != '\0') {
            i++;
            j++;
        }

        if (str[j] == '\0')
            return (firstOcc);
        if (src[i] == '\0')
            return (-1);

        i = firstOcc + 1;
        j = 0;
    }
    return -1;
}

char *extract_file_name(char *path) {
    int i, j, index;
    i = 0, j = 0, index = 0;
    while (path[i] != '\0') {
        if (path[i] == '/')
            index = i;
        i++;
    }
    char *r = 0;
    r = (char *)malloc(strlen(path) - index + 2);
    index++;
    while (path[index] != '\0') {
        r[j++] = path[index++];
    }
    r[j] = '\0';
    return r;
}

int extract_song_id(const char *name) {
    int i;
    i = 0;
    char id[5];
    while (name[i] != '.') {
        id[i] = name[i];
        i++;
    }
    id[i] = '\0';
    return atoi(id);
}

char *merge_str(char *base, char *middle, char *tail) {
    char *result = 0;
    result =
        (char *)malloc(strlen(base) + strlen(middle) + strlen(tail) + 1);
    strcpy(result, base);
    strcat(result, middle);
    strcat(result, tail);
    return result;
}

int mod (int a, int b)
{
   if(b < 0) //you can check for b == 0 separately and do what you want
     return mod(-a, -b);
   int ret = a % b;
   if(ret < 0)
     ret+=b;
   return ret;
}

void song_time_to_str(char *s, double total_length, double current_pos) {
    char label[14];
    int total_min = (int)total_length / 60;
    int total_sec = mod((int)total_length, 60);
    int current_min = (int)current_pos / 60;
    int current_sec = mod((int)current_pos, 60);
    sprintf(label, "%.2d:%.2d / %.2d:%.2d", current_min, current_sec, total_min, total_sec);
    strcpy(s, label);
}

int extract_meta_data(char *file_path, SongInfo *info) {
    AVFormatContext *fmt_ctx = NULL;
    AVDictionaryEntry *tag = NULL;
    int ret, i;

    info->title = NULL;
    info->album = NULL;
    info->artist = NULL;
    info->genre = NULL;
    info->track = NULL;
    info->date = NULL;

    av_register_all();
    if ((ret = avformat_open_input(&fmt_ctx, file_path, NULL, NULL)))
        return ret;

    while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        printf("%s=%s\n", tag->key, tag->value);
        char key[50];
        strcpy(key, tag->key);
        for(i = 0; key[i]; i++){
          key[i] = tolower(key[i]);
        }
        if (strcmp(key, "title") == 0) {
            info->title = (char *) malloc(strlen(tag->value)+1);
            strcpy(info->title, tag->value);
        } else if (strcmp(key, "album") == 0) {
            info->album = (char *) malloc(strlen(tag->value)+1);
            strcpy(info->album, tag->value);
        } else if (strcmp(key, "artist") == 0) {
            info->artist = (char *) malloc(strlen(tag->value)+1);
            strcpy(info->artist, tag->value);
        } else if (strcmp(key, "genre") == 0) {
            info->genre = (char *) malloc(strlen(tag->value)+1);
            strcpy(info->genre, tag->value);
        } else if (strcmp(key, "track") == 0) {
            info->track = (char *) malloc(strlen(tag->value)+1);
            strcpy(info->track, tag->value);
        } else if (strcmp(key, "date") == 0) {
            info->date = (char *) malloc(strlen(tag->value)+1);
            strcpy(info->date, tag->value);
        }
    }

    avformat_close_input(&fmt_ctx);
    return 0;
}

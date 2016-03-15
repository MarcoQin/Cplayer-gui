#ifndef PLAYER_H
#define PLAYER_H

#include "player_def.h"

void global_init();
CPlayer *player_create();
void player_destory(CPlayer *cp);

void packet_queue_init(PacketQueue *pkt_q);
int packet_queue_put(PacketQueue *pkt_q, AVPacket *pkt);
int packet_queue_get(PacketQueue *pkt_q, AVPacket *pkt, int block);
AudioState *stream_open(CPlayer *cp, const char *filename);
static void dump_metadata(void *ctx, AVDictionary *m, const char *indent);


/* control */
void cp_pause_audio();
CPlayer *cp_load_file(const char *filename);
#endif // PLAYER_H

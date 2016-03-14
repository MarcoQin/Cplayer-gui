#ifndef PLAYER_H
#define PLAYER_H

#include "player_def.h"

void global_init();
CPlayer *player_create();

void packet_queue_init(PacketQueue *pkt_q);
int packet_queue_put(PacketQueue *pkt_q, AVPacket *pkt);
int packet_queue_get(PacketQueue *pkt_q, AVPacket *pkt, int block);
AudioState *stream_open(CPlayer *cp, const char *filename);
#endif // PLAYER_H

#include "player.h"

int quit = 0;

static bool g_ffmpeg_global_inited = false;

void packet_queue_init(PacketQueue *pkt_q) {
    memset(pkt_q, 0, sizeof(PacketQueue));
    pkt_q->mutex = SDL_CreateMutex();
    pkt_q->cond = SDL_CreateCond();
}

void global_init() {
    if (g_ffmpeg_global_inited)
        return;
    // Register all formats and codecs
    av_register_all();

    // Init SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    g_ffmpeg_global_inited = true;
}

CPlayer *player_create() {
    CPlayer *cp = (CPlayer *) av_mallocz(sizeof(CPlayer));
    if (!cp)
        return NULL;
    return cp;
}

AudioState *stream_open(CPlayer *cp, const char *filename) {
    assert(!cp->is);
    AudioState *is;
    is = (AudioState *) av_mallocz(sizeof(AudioState));
    if (!is)
        return NULL;
    cp->input_filename = av_strdup(filename);
    packet_queue_init(&is->audio_queue);
    is->audio_volume = SDL_MIX_MAXVOLUME;
    is->muted = 0;
    cp->is = is;
    return is;
}


int packet_queue_put(PacketQueue *pkt_q, AVPacket *pkt) {
    AVPacketList *pkt1;
    pkt1 = (AVPacketList *)av_malloc(sizeof(AVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    SDL_LockMutex(pkt_q->mutex);

    if (!pkt_q->last_pkt)
        pkt_q->first_pkt = pkt1;
    else
        pkt_q->last_pkt->next = pkt1;
    pkt_q->last_pkt = pkt1;
    pkt_q->nb_packets++;
    pkt_q->size += pkt1->pkt.size;
    // Send signal to queue get function
    SDL_CondSignal(pkt_q->cond);

    SDL_UnlockMutex(pkt_q->mutex);
    return 0;
}

// if block set to True, this func will wait for SDL_CondSignal
int packet_queue_get(PacketQueue *pkt_q, AVPacket *pkt, int block) {
    AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(pkt_q->mutex);

    for (;;) {

        if (quit) {
            ret = -1;
            break;
        }

        pkt1 = pkt_q->first_pkt;
        if (pkt1) {
            pkt_q->first_pkt = pkt1->next;
            if (!pkt_q->first_pkt)
                pkt_q->last_pkt = NULL;
            pkt_q->nb_packets--;
            pkt_q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(pkt_q->cond, pkt_q->mutex);
        }
    }
    SDL_UnlockMutex(pkt_q->mutex);
    return ret;
}

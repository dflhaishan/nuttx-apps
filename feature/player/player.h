#ifndef PLAYER_H
#define PLAYER_H

#include <mqueue.h>
#include <pthread.h>
#include <nuttx/audio/audio.h>

enum player_state {
    PLAYER_STATE_IDLE = 0,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_PAUSED
};

struct player_t {
    int fd_dev;
    int fd_file;
    int format;
    int type;
    struct dec_ops_t *dec_ops;
    mqd_t mq;
    enum player_state state;
    uint16_t volume;            /* Volume as a whole percentage (0-100) */
    uint16_t balance;           /* Balance as a whole % (0=left off, 100=right off) */
    uint8_t treble;             /* Treble as a whole % */
    uint8_t bass;              /* Bass as a whole % */
    pthread_mutex_t mutex;
    bool streaming;
    bool running;
    bool failed;
};

struct dec_ops_t {
    int (*pre_parse)(int fd, uint32_t *samplerate,
                        uint8_t *chans, uint8_t *bps);
    int (*fill_data)(int fd, FAR struct ap_buffer_s *apb);
};

struct file_fmt_t {
    const char *ext;
    uint16_t   format;
    int        (*getsubformat)(void);
    struct dec_ops_t *(*getdec_ops)(void);
};

int player_init(struct player_t *player);

int player_fmtfromextension(const char *pfilename, int *subfmt);

struct dec_ops_t *player_decopsfromformat(int format);

int player_setvolume(struct player_t *player, uint16_t volume);

int player_setbalance(struct player_t *player, uint16_t balance);

int player_settreble(struct player_t *player, uint8_t treble);

int player_setbass(struct player_t *player, uint8_t bass);

int player_readbuffer(struct player_t *player, struct ap_buffer_s *apb);

int player_enqueuebuffer(struct player_t *player, struct ap_buffer_s *apb);

int player_getmp3subformat(void);
struct dec_ops_t *player_getmp3dec_ops(void);


int player_stop(struct player_t *player);

int player_pause(struct player_t *player);

int player_resume(struct player_t *player);

struct player_t *get_default_player(void);

#endif
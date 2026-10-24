#ifndef PLAYER_H
#define PLAYER_H

#include <nuttx/audio/audio.h>
struct player_t {
    int fd_dev;
    int fd_file;
    int format;
    int type;
    struct dec_ops_t *dec_ops;
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

int player_fmtfromextension(const char *pfilename, int *subfmt);

struct dec_ops_t *player_decopsfromformat(int format);

int player_getmp3subformat(void);
struct dec_ops_t *player_getmp3dec_ops(void);

#endif
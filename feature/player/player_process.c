#include <nuttx/config.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/input/buttons.h>
#include <semaphore.h>
#include <mqueue.h>
#include <nuttx/audio/audio.h>
#include "player.h"

#define PLAYER_DEVPATH "/dev/audio/vs1053d0"
#define MUSIC_FILEPATH "/mnt/sd/teers.mp3"
#define BUTTONS_SIGNO 32

pthread_t player_thread;
pthread_attr_t player_attr = {
    .priority     = PTHREAD_DEFAULT_PRIORITY,
    .policy       = SCHED_NORMAL,
    .inheritsched = PTHREAD_EXPLICIT_SCHED,
    .detachstate  = PTHREAD_CREATE_JOINABLE,
    .stackaddr    = NULL,
    .stacksize    = 4096,
    .guardsize    = PTHREAD_GUARD_DEFAULT,
};

extern struct player_t player;

void* player_process(FAR void* arg)
{
    player.fd_dev = open(PLAYER_DEVPATH, O_RDWR);
    if (player.fd_dev < 0) {
        printf("ERROR: Failed to open file %s\n", PLAYER_DEVPATH);
        return NULL;
    }
    struct audio_caps_s caps = {
        .ac_len = sizeof(caps),
        .ac_type = AUDIO_TYPE_QUERY,
        .ac_subtype = AUDIO_TYPE_QUERY,
    };
    if (ioctl(player.fd_dev, AUDIOIOC_GETCAPS, (unsigned long)&caps) != caps.ac_len) {
        printf("ERROR: Failed ioctl AUDIOIOC_GETCAPS\n");
        close(player.fd_dev);
        return NULL;
    }
    player.format = caps.ac_format.b[0] | (caps.ac_format.b[1] << 8);
    player.type = caps.ac_controls.b[0];

    printf("player %02x %02x\n", player.format, player.type);

    player.fd_file = open(MUSIC_FILEPATH, O_RDONLY);
    if (player.fd_file < 0) {
        printf("ERROR: Failed to open file %s\n", MUSIC_FILEPATH);
        close(player.fd_dev);
        return NULL;     
    }

    int file_fmt, file_subfmt;
    file_fmt = player_fmtfromextension(MUSIC_FILEPATH, &file_subfmt);
    if (file_fmt == AUDIO_FMT_UNDEF) {
        printf("ERROR: music file format error\n");
        close(player.fd_file);
        close(player.fd_dev);
        return NULL;            
    }

    if ((player.format & (1 << (file_fmt - 1))) == 0 ||
        (player.type & AUDIO_TYPE_OUTPUT) == 0)
    {
        /* Format not supported by the device */

        printf("ERROR: Format not supported by device: %d\n", file_fmt);
        return NULL;
    }

    if (player_get_dec_ops(&player, file_fmt) < 0) {
        printf("ERROR: Can not find dec ops for file_fmt: %d\n", file_fmt);
        return NULL;        
    }

    while (1) {
        usleep(1000);
    }
}
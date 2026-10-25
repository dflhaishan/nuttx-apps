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
#define PLAYER_MQPATH "/music_mq"
#define MUSIC_FILEPATH "/mnt/sd/teers.mp3"
#define BUTTONS_SIGNO 32

pthread_t player_thread;
pthread_attr_t player_attr = {
    .priority     = PTHREAD_DEFAULT_PRIORITY,
    .policy       = SCHED_NORMAL,
    .inheritsched = PTHREAD_EXPLICIT_SCHED,
    .detachstate  = PTHREAD_CREATE_JOINABLE,
    .stackaddr    = NULL,
    .stacksize    = 2048,
    .guardsize    = PTHREAD_GUARD_DEFAULT,
};

void* player_process(FAR void* arg)
{
    int ret = 0;
    struct player_t player = { 0 };
    ret = player_init(&player);
    if (ret < 0) {
        printf("ERROR: Failed to init player %d\n", ret);
        if (player.fd_dev > 0) {
            close(player.fd_dev);
            player.fd_dev = -1;
        }
        if (player.fd_file > 0) {
            close(player.fd_file);
            player.fd_dev = -1;
        }
        return NULL;
    }

    printf("%s\n", player.running ? "Playing..." : "Not running");

    ssize_t size;
    struct audio_msg_s msg;
    unsigned int prio;
    while (player.running) {
        size = mq_receive(player.mq, (char *)&msg, sizeof(msg), &prio);
        if (size != sizeof(msg)) {
            continue;
        }

        switch (msg.msg_id) {
            case AUDIO_MSG_DEQUEUE:
                if (player.streaming) {
                    ret = player_readbuffer(&player, msg.u.ptr);
                    if (ret < 0) {
                        player.streaming = false;
                    } else {
                        ret = player_enqueuebuffer(&player, msg.u.ptr);
                        if (ret < 0) {
                            player.streaming = false;
                            player.failed = true;
                        }
                    }
                }
                break;
            case AUDIO_MSG_STOP:
                ret = ioctl(player.fd_dev, AUDIOIOC_STOP, 0);
                if (ret < 0) {
                    printf("ERROR: failed ioctl AUDIOIOC_STOP\n");
                }
                player.streaming = false;
                break;
            case AUDIO_MSG_COMPLETE:
                ret = ioctl(player.fd_dev, AUDIOIOC_STOP, 0);
                if (ret < 0) {
                    printf("ERROR: failed ioctl AUDIOIOC_STOP\n");
                }
                player.running = false;
                break;
            default:
                break;
        }
    }

    printf("Exit\n");
    return NULL;
}

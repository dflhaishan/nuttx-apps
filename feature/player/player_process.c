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

extern struct player_t player;

void* player_process(FAR void* arg)
{
    int ret = 0;
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

    player.dec_ops = player_decopsfromformat(file_fmt);
    if (!player.dec_ops) {
        printf("ERROR: Can not find dec ops for file_fmt: %d\n", file_fmt);
        close(player.fd_file);
        close(player.fd_dev);
        return NULL;        
    }

    ret = pthread_mutex_init(&player.mutex, NULL);
    if (ret < 0) {
        printf("ERROR: init mutex failed\n");
        close(player.fd_file);
        close(player.fd_dev);
        return NULL; 
    }

    uint32_t samprate = 0;
    uint8_t chans = 0, bps = 0;
    if (player.dec_ops->pre_parse) {
        ret = player.dec_ops->pre_parse(player.fd_file, &samprate, &chans, &bps);
        if (ret != 0) {
            printf("player dec_ops pre_parse return %d\n", ret);
        }
    }

    ret = ioctl(player.fd_dev, AUDIOIOC_RESERVE, 0);
    if (ret < 0) {
        printf("ERROR: Failed to reserve device: %d\n", ret);
        close(player.fd_file);
        close(player.fd_dev);
        return NULL;
    }

    caps.ac_len = sizeof(caps);
    caps.ac_type = AUDIO_TYPE_OUTPUT;
    caps.ac_subtype = AUDIO_TYPE_QUERY;
    if (ioctl(player.fd_dev, AUDIOIOC_GETCAPS, (unsigned long)&caps) == caps.ac_len) {
        if ((caps.ac_channels >> 4) && (chans < (caps.ac_channels >> 4))) {
            printf("ERROR: ac_channels invalid %d\n", caps.ac_channels);
            close(player.fd_file);
            close(player.fd_dev);            
            return NULL;
        }
    }

    struct audio_caps_desc_s cap_desc = { 0 };
    int chmap = 0;
    if (chans && samprate && bps) {
        cap_desc.caps.ac_len = sizeof(struct audio_caps_s);
        cap_desc.caps.ac_type = AUDIO_TYPE_OUTPUT;
        cap_desc.caps.ac_channels = chans;
        cap_desc.caps.ac_chmap = chmap;
        cap_desc.caps.ac_controls.hw[0] = samprate;
        cap_desc.caps.ac_controls.b[3] = samprate >> 16;
        cap_desc.caps.ac_controls.b[2] = bps;
        cap_desc.caps.ac_subtype = file_fmt;

        ret = ioctl(player.fd_dev, AUDIOIOC_CONFIGURE, (unsigned long)&cap_desc);
        if (ret < 0) {
            printf("ERROR: failed ioctl AUDIOIOC_CONFIGURE\n");
        }
    }

    struct ap_buffer_info_s  buf_info = { 0 };
    ret = ioctl(player.fd_dev, AUDIOIOC_GETBUFFERINFO, (unsigned long)&buf_info);
    if (ret < 0) {
        /* Driver doesn't report its buffer size.  Use our default. */
        buf_info.nbuffers = 2;
        buf_info.buffer_size = 8192;
    }

    struct mq_attr mq_attr = {
        .mq_maxmsg = buf_info.nbuffers + 8,
        .mq_msgsize = sizeof(struct audio_msg_s),
        .mq_curmsgs = 0,
        .mq_flags = 0,
    };

    player.mq = mq_open(PLAYER_MQPATH, O_RDWR | O_CREAT, 0644, &mq_attr);
    if (player.mq < 0) {
        printf("ERROR: open %s failed\n", PLAYER_MQPATH);
        close(player.fd_file);
        close(player.fd_dev);
        return NULL; 
    }

    ret = ioctl(player.fd_dev, AUDIOIOC_REGISTERMQ, (unsigned long)player.mq);
    if (ret < 0) {
        printf("ERROR: failed ioctl AUDIOIOC_REGISTERMQ\n");
        close(player.fd_file);
        close(player.fd_dev);
        return NULL; 
    }

    struct ap_buffer_s *buffers[buf_info.nbuffers];
    memset(buffers, 0, sizeof(buffers));
    struct audio_buf_desc_s buf_desc = { 0 };
    int i;
    for (i = 0; i < buf_info.nbuffers; i++) {
        buf_desc.numbytes = buf_info.buffer_size;
        buf_desc.u.pbuffer = &buffers[i];

        ret = ioctl(player.fd_dev, AUDIOIOC_ALLOCBUFFER, (unsigned long)&buf_desc);
        if (ret != sizeof(buf_desc)) {
            printf("ERROR: could not allocate buffer %d\n", i);
            close(player.fd_file);
            close(player.fd_dev);
            return NULL;
        }
    }

    bool streaming = true;
    bool running = true;
    bool failed = false;
    for (i = 0; i < buf_info.nbuffers; i++) {
        ret = player_readbuffer(buffers[i]);
        if (ret < 0) {
            printf("ERROR: player dec_ops fill data failed %d\n", i);
            streaming = false;
            if (i == 0) {
                running = false;
            }
        } else {
            ret = player_enqueuebuffer(buffers[i]);
            if (ret < 0) {
                streaming = false;
                failed = true;
                break;
            }
        }
    }

    if (running && !failed) {
        ret = ioctl(player.fd_dev, AUDIOIOC_START, 0);
        if (ret < 0) {
            printf("ERROR: failed ioctl AUDIOIOC_START\n");
            failed = true;
        }
    }

    if (running && !failed) {
        player.state = PLAYER_STATE_PLAYING;
        player_setvolume(player.volume);
        player_setbalance(player.balance);
        player_setbass(player.bass);
        player_settreble(player.treble);
    }

    printf("%d buffers queued, running=%d streaming=%d failed=%d\n", i, running, streaming, failed);

    printf("%s\n", running ? "Playing..." : "Not running");

    ssize_t size;
    struct audio_msg_s msg;
    unsigned int prio;
    while (running) {
        size = mq_receive(player.mq, (char *)&msg, sizeof(msg), &prio);
        if (size != sizeof(msg)) {
            continue;
        }

        switch (msg.msg_id) {
            case AUDIO_MSG_DEQUEUE:
                if (streaming) {
                    ret = player_readbuffer(msg.u.ptr);
                    if (ret < 0) {
                        streaming = false;
                    } else {
                        ret = player_enqueuebuffer(msg.u.ptr);
                        if (ret < 0) {
                            streaming = false;
                            failed = true;
                        }
                    }
                }
                break;
            case AUDIO_MSG_STOP:
                ret = ioctl(player.fd_dev, AUDIOIOC_STOP, 0);
                if (ret < 0) {
                    printf("ERROR: failed ioctl AUDIOIOC_STOP\n");
                }
                streaming = false;
                break;
            case AUDIO_MSG_COMPLETE:
                ret = ioctl(player.fd_dev, AUDIOIOC_STOP, 0);
                if (ret < 0) {
                    printf("ERROR: failed ioctl AUDIOIOC_STOP\n");
                }
                running = false;
                break;
            default:
                break;
        }
    }

    printf("Exit\n");
    return NULL;
}

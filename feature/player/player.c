#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <nuttx/audio/audio.h>
#include "player.h"

#define PLAYER_DEVPATH "/dev/audio/vs1053d0"
#define PLAYER_MQPATH "/music_mq"
#define MUSIC_FILEPATH "/mnt/sd/teers.mp3"

struct player_t player = { 0 };

static const struct file_fmt_t file_fmt[] = {
    /*ext   format          getsubformat            dec_ops*/
    {"mp3", AUDIO_FMT_MP3,  player_getmp3subformat, player_getmp3dec_ops},
};

int player_init(struct player_t **pplayer)
{
    int ret = 0;
    *pplayer = &player;

    player.state   = PLAYER_STATE_IDLE;

    player.fd_dev = open(PLAYER_DEVPATH, O_RDWR);
    if (player.fd_dev < 0) {
        printf("ERROR: Failed to open file %s\n", PLAYER_DEVPATH);
        return -errno;
    }

    player.fd_file = open(MUSIC_FILEPATH, O_RDONLY);
    if (player.fd_file < 0) {
        printf("ERROR: Failed to open file %s\n", MUSIC_FILEPATH);
        return -errno;
    }

    if (pthread_mutex_init(&player.mutex, NULL) < 0) {
        printf("ERROR: init mutex failed\n");
        return -errno;
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
        return -errno; 
    }
    ret = ioctl(player.fd_dev, AUDIOIOC_REGISTERMQ, (unsigned long)player.mq);
    if (ret < 0) {
        printf("ERROR: failed ioctl AUDIOIOC_REGISTERMQ\n");
        return -errno; 
    }

    struct audio_caps_s caps = {
        .ac_len = sizeof(caps),
        .ac_type = AUDIO_TYPE_QUERY,
        .ac_subtype = AUDIO_TYPE_QUERY,
    };
    ret = ioctl(player.fd_dev, AUDIOIOC_GETCAPS, (unsigned long)&caps);
    if (ret != caps.ac_len) {
        printf("ERROR: Failed ioctl AUDIOIOC_GETCAPS\n");
        return -EINVAL;
    }

    player.format = caps.ac_format.b[0] | (caps.ac_format.b[1] << 8);
    player.type = caps.ac_controls.b[0];

    printf("player format:0x%02x type:0x%02x\n", player.format, player.type);

    int file_format = AUDIO_FMT_UNDEF;
    int file_subformat = AUDIO_FMT_UNDEF;
    file_format = player_fmtfromextension(MUSIC_FILEPATH, &file_subformat);
    if (file_format == AUDIO_FMT_UNDEF) {
        printf("ERROR: music file format error\n");
        return -EINVAL;            
    }
    if ((player.format & (1 << (file_format - 1))) == 0 || (player.type & AUDIO_TYPE_OUTPUT) == 0) {
        printf("ERROR: Format not supported by device: %d\n", file_format);
        return -EINVAL;        
    }

    player.dec_ops = player_decopsfromformat(file_format);
    if (!player.dec_ops) {
        printf("ERROR: Can not find dec ops for file_format: %d\n", file_format);
        return -EINVAL;        
    }

    uint32_t samprate = 0;
    uint8_t chans = 0, bps = 0;
    if (player.dec_ops->pre_parse) {
        ret = player.dec_ops->pre_parse(player.fd_file, &samprate, &chans, &bps);
        if (ret != 0) {
            printf("player dec_ops pre_parse return %d\n", ret);
            return -EINVAL;
        }
    }

    ret = ioctl(player.fd_dev, AUDIOIOC_RESERVE, 0);
    if (ret < 0) {
        printf("ERROR: Failed to reserve device: %d\n", ret);
        return -errno;
    }

    caps.ac_len = sizeof(caps);
    caps.ac_type = AUDIO_TYPE_OUTPUT;
    caps.ac_subtype = AUDIO_TYPE_QUERY;
    ret = ioctl(player.fd_dev, AUDIOIOC_GETCAPS, (unsigned long)&caps);
    if (ret != caps.ac_len) {
        printf("ERROR: Failed ioctl AUDIOIOC_GETCAPS\n");
        return -EINVAL;

    }
    if ((caps.ac_channels >> 4) && (chans < (caps.ac_channels >> 4))) {
        printf("ERROR: ac_channels invalid %d\n", caps.ac_channels);           
        return -EINVAL;
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
        cap_desc.caps.ac_subtype = file_format;

        ret = ioctl(player.fd_dev, AUDIOIOC_CONFIGURE, (unsigned long)&cap_desc);
        if (ret < 0) {
            printf("ERROR: failed ioctl AUDIOIOC_CONFIGURE\n");
            return -errno;
        }
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
            return -EINVAL;
        }
    }

    player.streaming = true;
    player.running = true;
    player.failed = false;

    for (i = 0; i < buf_info.nbuffers; i++) {
        ret = player_readbuffer(buffers[i]);
        if (ret < 0) {
            printf("ERROR: player dec_ops fill data failed %d\n", i);
            player.streaming = false;
            if (i == 0) {
                player.running = false;
            }
        } else {
            ret = player_enqueuebuffer(buffers[i]);
            if (ret < 0) {
                player.streaming = false;
                player.failed = true;
                break;
            }
        }
    }

    if (player.running && !(player.failed)) {
        ret = ioctl(player.fd_dev, AUDIOIOC_START, 0);
        if (ret < 0) {
            printf("ERROR: failed ioctl AUDIOIOC_START\n");
            player.failed = true;
        }
    }

    if (player.running && !(player.failed)) {
        player.state = PLAYER_STATE_PLAYING;
        player_setvolume(400);
        player_setbalance(500);
        player_setbass(50);
        player_settreble(50);
    }

    printf("%d buffers queued, running=%d streaming=%d failed=%d\n",
                i, player.running, player.streaming, player.failed);

    return ret;
}

int player_fmtfromextension(const char *pfilename, int *subfmt)
{
  FAR const char *pext;
  uint8_t         x;
  uint8_t         c;

  /* Find the file extension, if any */

  x = strlen(pfilename) - 1;
  while (x > 0)
    {
      /* Search backward for the first '.' */

      if (pfilename[x] == '.')
        {
          /* First '.' found.  Now compare with known extensions */

          pext = &pfilename[x + 1];
          for (c = 0; c < (sizeof(file_fmt) / sizeof(struct file_fmt_t)); c++)
            {
              /* Test for extension match */
              if (strcasecmp(pext, file_fmt[c].ext) == 0)
                {
                  /* Test if we have a sub-format detection routine */

                  if (subfmt && file_fmt[c].getsubformat)
                    {
                      *subfmt = file_fmt[c].getsubformat();
                    }

                  /* Return the format for this extension */
                  return file_fmt[c].format;
                }
            }
        }

      /* Stop if we find a '/' */

      if (pfilename[x] == '/')
        {
          break;
        }

      x--;
    }
  return AUDIO_FMT_UNDEF;
}

struct dec_ops_t *player_decopsfromformat(int format)
{
    struct dec_ops_t * dec_ops = NULL;

    for (int i = 0; i < (sizeof(file_fmt) / sizeof(struct file_fmt_t)); i++) {
        if (format == file_fmt[i].format) {
            if (file_fmt[i].getdec_ops) {
                dec_ops = file_fmt[i].getdec_ops();
            }
            break;
        }
    }

    return dec_ops;
}

int player_setvolume(uint16_t volume)
{
    struct audio_caps_desc_s cap_desc = { 0 };
    int ret = 0;
    pthread_mutex_lock(&player.mutex);

    if (player.state == PLAYER_STATE_PLAYING) {
        cap_desc.caps.ac_len = sizeof(struct audio_caps_s);
        cap_desc.caps.ac_type = AUDIO_TYPE_FEATURE;
        cap_desc.caps.ac_format.hw = AUDIO_FU_VOLUME;
        cap_desc.caps.ac_controls.hw[0] = volume;

        ret = ioctl(player.fd_dev, AUDIOIOC_CONFIGURE, (unsigned long)&cap_desc);
        if (ret < 0) {
            printf("ERROR: failed ioctl AUDIOIOC_CONFIGURE\n");
            pthread_mutex_unlock(&player.mutex);
            return -1;
        }
    }

    player.volume = volume;
    pthread_mutex_unlock(&player.mutex);
    printf("player volume: %d\n", volume);
    return 0;
}

int player_setbalance(uint16_t balance)
{
    struct audio_caps_desc_s cap_desc = { 0 };
    int ret = 0;
    pthread_mutex_lock(&player.mutex);

    if (player.state == PLAYER_STATE_PLAYING) {
        cap_desc.caps.ac_len = sizeof(struct audio_caps_s);
        cap_desc.caps.ac_type = AUDIO_TYPE_FEATURE;
        cap_desc.caps.ac_format.hw = AUDIO_FU_BALANCE;
        cap_desc.caps.ac_controls.hw[0] = balance;

        ret = ioctl(player.fd_dev, AUDIOIOC_CONFIGURE, (unsigned long)&cap_desc);
        if (ret < 0) {
            printf("ERROR: failed ioctl AUDIOIOC_CONFIGURE\n");
            pthread_mutex_unlock(&player.mutex);
            return -1;
        }
    }

    player.balance = balance;
    pthread_mutex_unlock(&player.mutex);
    printf("player balance: %d\n", balance);
    return 0;    
}

int player_settreble(uint8_t treble)
{
    struct audio_caps_desc_s cap_desc = { 0 };
    int ret = 0;
    pthread_mutex_lock(&player.mutex);

    if (player.state == PLAYER_STATE_PLAYING) {
        cap_desc.caps.ac_len = sizeof(struct audio_caps_s);
        cap_desc.caps.ac_type = AUDIO_TYPE_FEATURE;
        cap_desc.caps.ac_format.hw = AUDIO_FU_TREBLE;
        cap_desc.caps.ac_controls.b[0] = treble;

        ret = ioctl(player.fd_dev, AUDIOIOC_CONFIGURE, (unsigned long)&cap_desc);
        if (ret < 0) {
            printf("ERROR: failed ioctl AUDIOIOC_CONFIGURE\n");
            pthread_mutex_unlock(&player.mutex);
            return -1;
        }
    }

    player.treble = treble;
    pthread_mutex_unlock(&player.mutex);
    printf("player treble: %d\n", treble);
    return 0;    
}

int player_setbass(uint8_t bass)
{
    struct audio_caps_desc_s cap_desc = { 0 };
    int ret = 0;
    pthread_mutex_lock(&player.mutex);

    if (player.state == PLAYER_STATE_PLAYING) {
        cap_desc.caps.ac_len = sizeof(struct audio_caps_s);
        cap_desc.caps.ac_type = AUDIO_TYPE_FEATURE;
        cap_desc.caps.ac_format.hw = AUDIO_FU_BASS;
        cap_desc.caps.ac_controls.b[0] = bass;

        ret = ioctl(player.fd_dev, AUDIOIOC_CONFIGURE, (unsigned long)&cap_desc);
        if (ret < 0) {
            printf("ERROR: failed ioctl AUDIOIOC_CONFIGURE\n");
            pthread_mutex_unlock(&player.mutex);
            return -1;
        }
    }

    player.bass = bass;
    pthread_mutex_unlock(&player.mutex);
    printf("player bass: %d\n", bass);
    return 0;     
}

int player_readbuffer(struct ap_buffer_s *apb)
{
    int ret = 0;

    ret = player.dec_ops->fill_data(player.fd_file, apb);
    if (ret < 0) {
        printf("ERROR: player readbuffer failed\n");
    }

    return ret;
}

int player_enqueuebuffer(struct ap_buffer_s *apb)
{
    struct audio_buf_desc_s buf_desc = { 0};
    int ret = 0;

    buf_desc.numbytes = apb->nbytes;
    buf_desc.u.buffer = apb;

    ret = ioctl(player.fd_dev, AUDIOIOC_ENQUEUEBUFFER, (unsigned long)&buf_desc);
    if (ret < 0) {
        printf("ERROR: player enqueuebuffer failed\n");
        return -1;
    }

    return 0;
}
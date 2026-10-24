#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <nuttx/audio/audio.h>
#include "player.h"

struct player_t player = {
    .fd_dev  = -1,
    .fd_file = -1,
    .format  = 0,
    .type    = 0,
    .dec_ops = NULL,
    .mq      = -1,
    .state   = PLAYER_STATE_IDLE,
    .volume  = 400,
    .balance = 500,
    .treble = 50,
    .bass = 50,
};

static const struct file_fmt_t file_fmt[] = {
    /*ext   format          getsubformat            dec_ops*/
    {"mp3", AUDIO_FMT_MP3,  player_getmp3subformat, player_getmp3dec_ops},
};

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
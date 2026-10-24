#include <stdio.h>
#include <nuttx/audio/audio.h>
#include "player.h"

#define ID3V2_BIT_MASK 0x7F

const static uint16_t g_mpa_freq_tab[3] =
{
  44100, 48000, 32000
};

const static uint16_t g_mpa_bitrate_tab[2][3][15] =
{
  {
    {
      0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448
    },
    {
      0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384
    },
    {
      0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320
    }
  },
  {
    {
      0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256
    },
    {
      0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160
    },
    {
      0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160
    }
  }
};

static int nxplayer_check_mpeg(uint32_t header)
{
  /* header */

  if ((header & 0xffe00000) != 0xffe00000)
    {
      return -EINVAL;
    }

  /* version check */

  if ((header & (3 << 19)) == 1 << 19)
    {
      return -EINVAL;
    }

  /* layer check */

  if ((header & (3 << 17)) == 0)
    {
      return -EINVAL;
    }

  /* bit rate */

  if ((header & (0xf << 12)) == 0xf << 12)
    {
      return -EINVAL;
    }

  /* frequency */

  if ((header & (3 << 10)) == 3 << 10)
    {
      return -EINVAL;
    }

  return 0;
}

static int nxplayer_parse_mpeg(uint32_t header, FAR uint32_t *samplerate,
                               FAR uint8_t *chans, FAR uint8_t *bps)
{
  int sample_rate;
  int frame_size;
  int padding;
  int mpeg25;
  int sr_idx;
  int br_idx;
  int layer;
  int mode;
  int lsf;
  int ret;

  ret = nxplayer_check_mpeg(header);
  if (ret < 0)
    {
      return ret;
    }

  if (header & (1 << 20))
    {
      lsf = (header & (1 << 19)) ? 0 : 1;
      mpeg25 = 0;
    }
  else
    {
      lsf = 1;
      mpeg25 = 1;
    }

  layer   = 4 - ((header >> 17) & 3);
  br_idx  = (header >> 12) & 0xf;
  sr_idx  = (header >> 10) & 3;
  padding = (header >> 9) & 1;
  mode    = (header >> 6) & 3;

  if (sr_idx >= (sizeof(g_mpa_freq_tab) / sizeof(uint16_t)) || br_idx >= 0xf)
    {
      return -EINVAL;
    }

  sample_rate = g_mpa_freq_tab[sr_idx] >> (lsf + mpeg25);

  if (br_idx != 0)
    {
      frame_size = g_mpa_bitrate_tab[lsf][layer - 1][br_idx];

      switch (layer)
        {
          case 1:
            frame_size = (frame_size * 12000) / sample_rate;
            frame_size = (frame_size + padding) * 4;
            break;

          case 2:
            frame_size = (frame_size * 144000) / sample_rate;
            frame_size += padding;
            break;

          default:
          case 3:
            frame_size = (frame_size * 144000) / (sample_rate << lsf);
            frame_size += padding;
            break;
        }
    }
  else
    {
      /* if no frame size computed, signal it */

      return -EINVAL;
    }

  if (samplerate)
    {
      *samplerate = sample_rate;
    }

  if (chans)
    {
      *chans = mode == 3 ? 1 : 2;
    }

  if (bps)
    {
      *bps = 16;
    }

  return frame_size;
}

static int player_mp3dec_pre_parse(int fd, uint32_t *samplerate,
                        uint8_t *chans, uint8_t *bps)
{
  uint32_t mpa_header;
  uint8_t buffer[10];
  off_t position;
  int ret;

  ret = read(fd, buffer, sizeof(buffer));
  if (ret < sizeof(buffer))
    {
      return -ENODATA;
    }

  if (!memcmp(buffer, "ID3", 3))
    {
      position = (buffer[6] & ID3V2_BIT_MASK) * 0x200000 +
                 (buffer[7] & ID3V2_BIT_MASK) * 0x4000 +
                 (buffer[8] & ID3V2_BIT_MASK) * 0x80 +
                 (buffer[9] & ID3V2_BIT_MASK) +
                 sizeof(buffer);

      lseek(fd, position, SEEK_SET);
      read(fd, buffer, 4);
    }
  else
    {
      position = 0;
    }

  mpa_header = buffer[0] << 24 |
               buffer[1] << 16 |
               buffer[2] << 8  |
               buffer[3];

  ret = nxplayer_parse_mpeg(mpa_header, samplerate, chans, bps);
  if (ret < 0)
    {
      return ret;
    }

  lseek(fd, position, SEEK_SET);
  return OK;
}

static int player_mp3dec_fill_data(int fd, struct ap_buffer_s *apb)
{
  /* Read data into the buffer. */

  apb->nbytes  = read(fd, apb->samp, apb->nmaxbytes);
  apb->curbyte = 0;
  apb->flags   = 0;

  while (0 < apb->nbytes && apb->nbytes < apb->nmaxbytes)
    {
      int n   = apb->nmaxbytes - apb->nbytes;
      int ret = read(fd, &apb->samp[apb->nbytes], n);

      if (0 >= ret)
        {
          break;
        }

      apb->nbytes += ret;
    }

  if (apb->nbytes < apb->nmaxbytes)
    {
      /* Set a flag to indicate that this is the final buffer in the stream */

      apb->flags |= AUDIO_APB_FINAL;

      return -ENODATA;
    }

  /* Return OK to indicate that the buffer should be passed through to the
   * audio device.  This does not necessarily indicate that data was read
   * correctly.
   */
  return OK;
}

static struct dec_ops_t player_mp3dec_ops = {
    .pre_parse = player_mp3dec_pre_parse,
    .fill_data = player_mp3dec_fill_data
};

int player_getmp3subformat(void)
{
    return AUDIO_SUBFMT_PCM_MP3;
}

struct dec_ops_t *player_getmp3dec_ops(void)
{
    return &player_mp3dec_ops;
}
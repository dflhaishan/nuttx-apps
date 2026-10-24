#include <string.h>
#include <stdio.h>
#include <nuttx/audio/audio.h>
#include "player.h"

struct player_t player = {
    .fd_dev  = -1,
    .fd_file = -1,
    .format  = 0,
    .type    = 0
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

int player_get_dec_ops(struct player_t *pplayer, int format)
{
    int ret = 0;
    int i;
    for (i = 0; i < (sizeof(file_fmt) / sizeof(struct file_fmt_t)); i++) {
        if (format == file_fmt[i].format) {
            if (file_fmt[i].getdec_ops) {
                pplayer->dec_ops = file_fmt[i].getdec_ops();
            }
            break;
        }
    }
    if (!pplayer->dec_ops) {
        ret = -1;
    }
    return ret;
}
/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <lvgl/lvgl.h>
#include "lv_music/music_func.h"
/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * hello_main
 ****************************************************************************/

pthread_t ui_thread;
pthread_attr_t ui_attr = {
    .priority     = PTHREAD_DEFAULT_PRIORITY,
    .policy       = SCHED_NORMAL,
    .inheritsched = PTHREAD_EXPLICIT_SCHED,
    .detachstate  = PTHREAD_CREATE_JOINABLE,
    .stackaddr    = NULL,
    .stacksize    = 10240,
    .guardsize    = PTHREAD_GUARD_DEFAULT,
};
mqd_t ui_mq;

void* ui_process(FAR void *arg)
{
    lv_nuttx_dsc_t info;
    lv_nuttx_result_t result;

    if (lv_is_initialized()) {
        LV_LOG_ERROR("LVGL already initialized! aborting.");
        return NULL;
    }

    lv_init();

    lv_nuttx_dsc_init(&info);

#ifdef CONFIG_LV_USE_NUTTX_LCD
    info.fb_path = "/dev/lcd0";
#endif
    lv_nuttx_init(&info, &result);
    if (result.disp == NULL) {
        LV_LOG_ERROR("lv_demos initialization failure!");
        return NULL;
    }

    lv_apps_creat();

    struct mq_attr ui_mq_attr = {
        .mq_maxmsg = 3,
        .mq_msgsize = 4,
        .mq_flags = 0,
        .mq_curmsgs = 0
    };

    ui_mq = mq_open("/btn_mq", O_RDWR | O_CREAT, 0666, &ui_mq_attr);
    if (ui_mq < 0) {
        printf("ERROR: failed to open btn_mq\n");
        return NULL;
    }

    while (1) {
        uint32_t idle;
        idle = lv_timer_handler();

        /* Minimum sleep of 1ms */

        idle = idle ? idle : 1;
        usleep(idle * 1000);
    }

    return NULL;
}
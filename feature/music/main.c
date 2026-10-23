/****************************************************************************
 * apps/examples/hello/hello_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <lvgl/lvgl.h>
#include "lv_music/music_func.h"
/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * hello_main
 ****************************************************************************/

static void* thread_num(FAR void *arg)
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

    while (1) {
        uint32_t idle;
        idle = lv_timer_handler();

        /* Minimum sleep of 1ms */

        idle = idle ? idle : 1;
        usleep(idle * 1000);
    }

    return NULL;
}

int main(int argc, FAR char *argv[])
{
    pthread_t thread1;
    pthread_attr_t attr1 = {
        .priority     = PTHREAD_DEFAULT_PRIORITY,
        .policy       = SCHED_NORMAL,
        .inheritsched = PTHREAD_EXPLICIT_SCHED,
        .detachstate  = PTHREAD_CREATE_JOINABLE,
        .stackaddr    = NULL,
        .stacksize    = 20480,
        .guardsize    = PTHREAD_GUARD_DEFAULT,
    };

    int ret;
    ret = pthread_create(&thread1, &attr1, thread_num, NULL);
    if (ret != 0) {
        printf("ERROR, pthread_create, ret %d\n", ret);
        ASSERT(false);
        return EXIT_FAILURE;
    }

    while (1) {
        usleep(1000 * 1000);
    }
    return 0;
}

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
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/input/buttons.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/
#define BUTTONS_SIGNO 32
/****************************************************************************
 * hello_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
    int fd_btn = open("/dev/buttons", O_RDONLY| O_NONBLOCK);
    if (fd_btn < 0) {
        printf("ERROR: Failed to open file\n");
        return -1;
    }

    btn_buttonset_t btn_supported = 0;
    int ret = ioctl(fd_btn, BTNIOC_SUPPORTED, &btn_supported);
    if (ret < 0) {
        printf("ERROR: failed to ioctl BTNIOC_SUPPORTED\n");
        close(fd_btn);
        return -1;
    }
    printf("btn supported 0x%08lx\n", btn_supported);

    struct btn_notify_s btn_events;
    btn_events.bn_press = btn_supported;
    btn_events.bn_release = btn_supported;
    btn_events.bn_event.sigev_notify = SIGEV_SIGNAL;
    btn_events.bn_event.sigev_signo = BUTTONS_SIGNO;

    ret = ioctl(fd_btn, BTNIOC_REGISTER, &btn_events);
    if (ret < 0) {
        printf("ERROR: failed to ioctl BTNIOC_REGISTER\n");
        close(fd_btn);
        return -1;
    }

    struct siginfo value;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, BUTTONS_SIGNO);
    while (1) {
        ret = sigwaitinfo(&set, &value);
        if (ret < 0) {
            printf("ERROR: failed to sigwaitinfo\n");
            return -1;
        }

        btn_buttonset_t sample = (btn_buttonset_t)value.si_value.sival_int;
        printf("btn sample %08lx\n", sample);
    }

    return 0;
}

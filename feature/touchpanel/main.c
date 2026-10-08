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
#include <poll.h>
#include <nuttx/input/touchscreen.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/
#define TOUCH_PANEL_PATH "/dev/input0"
/****************************************************************************
 * hello_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
    int tp_fd = open(TOUCH_PANEL_PATH, O_RDONLY);
    struct pollfd pfd = { .fd = tp_fd, .events = POLLIN };
    while (1) {
        int ret = poll(&pfd, 1, -1); // 阻塞直到事件
        if (ret > 0 && (pfd.revents & POLLIN)) {
            struct touch_sample_s sample;
            int n = read(tp_fd, &sample, sizeof(sample));
            if (n == sizeof(sample)) {
                printf("touch x=%d y=%d flags=0x%x\n",
                    sample.point[0].x, sample.point[0].y,
                    sample.point[0].flags);
            }
        }
    }
    return 0;
}

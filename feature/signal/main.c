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
#include "main.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * hello_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
    int ret;

    ret = pthread_create(&btn_thread, &btn_attr, btn_process, NULL);
    if (ret != 0) {
        printf("ERROR, btn_thread created failed, ret %d\n", ret);
        ASSERT(false);
        return EXIT_FAILURE;
    }

    ret = pthread_create(&led_thread, &led_attr, led_process, NULL);
    if (ret != 0) {
        printf("ERROR, led_thread created failed, ret %d\n", ret);
        ASSERT(false);
        return EXIT_FAILURE;
    }



    while (1) {
        usleep(1000);
    }

    return 0;
}

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

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * hello_main
 ****************************************************************************/

static void* thread_num(FAR void *arg)
{
    while (1) {
        printf("thread%d\n", *(int *)arg);

        usleep(1000 * 1000);
    }

    return NULL;
}

int main(int argc, FAR char *argv[])
{
    pthread_t thread1, thread2;
    pthread_attr_t attr1 = {
        .priority     = PTHREAD_DEFAULT_PRIORITY,
        .policy       = SCHED_NORMAL,
        .inheritsched = PTHREAD_EXPLICIT_SCHED,
        .detachstate  = PTHREAD_CREATE_JOINABLE,
        .stackaddr    = NULL,
        .stacksize    = 1024,
        .guardsize    = PTHREAD_GUARD_DEFAULT,
    };

    pthread_attr_t attr2 = {
        .priority     = PTHREAD_DEFAULT_PRIORITY,
        .policy       = SCHED_NORMAL,
        .inheritsched = PTHREAD_EXPLICIT_SCHED,
        .detachstate  = PTHREAD_CREATE_JOINABLE,
        .stackaddr    = NULL,
        .stacksize    = 1024,
        .guardsize    = PTHREAD_GUARD_DEFAULT,
    };

    int num1 = 1, num2 = 2;

    int ret;
    ret = pthread_create(&thread1, &attr1, thread_num, &num1);
    if (ret != 0) {
        printf("ERROR, pthread_create, ret %d\n", ret);
        ASSERT(false);
        return EXIT_FAILURE;
    }

    ret = pthread_create(&thread2, &attr2, thread_num, &num2);
    if (ret != 0) {
        printf("ERROR, pthread_create, ret %d\n", ret);
        ASSERT(false);
        return EXIT_FAILURE;
    }

    while (1) {
        printf("Hello, World!!\n");
        usleep(1000 * 1000);
    }
    return 0;
}

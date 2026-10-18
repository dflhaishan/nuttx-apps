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
#include <nuttx/lcd/lcd_dev.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/
#define OLED_PATH "/dev/lcd1"
/****************************************************************************
 * hello_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
    int oled_fd = open(OLED_PATH, O_RDWR);
    int ret = 0;
    
    if (oled_fd < 0) {
        printf("open file %s failed %d\r\n", OLED_PATH, ret);
        return -1;
    }

    ret = ioctl(oled_fd, LCDDEVIO_SETPOWER, 1);
    if (ret < 0) {
        printf("ioctl LCDDEVIO_SETPOWER failed %d\r\n", ret);
        close(oled_fd);
        return -1;
    }

    struct fb_videoinfo_s info;
    ret = ioctl(oled_fd, LCDDEVIO_GETVIDEOINFO, &info);
    if (ret < 0) {
        printf("ioctl LCDDEVIO_GETVIDEOINFO failed %d\r\n", ret);
        close(oled_fd);
        return -1;        
    }

    printf("fmt %d, xres %d, yres %d, nplanes %d\r\n",
                info.fmt, info.xres, info.yres, info.nplanes);

    uint8_t buf[16 * 64];
    memset(buf, 0, sizeof(buf));
    struct lcddev_area_s lcd_area = 
    { 
        .row_start = 0,
        .row_end = info.yres - 1,
        .col_start = 0,
        .col_end = info.xres - 1,
        .stride = 16,
        .data = buf
    };
    ret = ioctl(oled_fd, LCDDEVIO_PUTAREA, &lcd_area);
    if (ret < 0) {
        printf("ioctl LCDDEVIO_PUTAREA failed %d\r\n", ret);
        close(oled_fd);
        return -1;           
    }

    memset(buf, 0x0f, sizeof(buf));
    ret = ioctl(oled_fd, LCDDEVIO_PUTAREA, &lcd_area);
    if (ret < 0) {
        printf("ioctl LCDDEVIO_PUTAREA failed %d\r\n", ret);
        close(oled_fd);
        return -1;           
    }

    close(oled_fd);
    return 0;
}

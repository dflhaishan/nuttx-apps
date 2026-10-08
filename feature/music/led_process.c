#include <nuttx/config.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/leds/userled.h>
#include <semaphore.h>
#include <mqueue.h>

#define LED_DEVPATH "/dev/userleds"
#define LEDS_SIGNO 32

extern sem_t btn_sem;
extern mqd_t btn_mq;

pthread_t led_thread;
pthread_attr_t led_attr = {
    .priority     = PTHREAD_DEFAULT_PRIORITY,
    .policy       = SCHED_NORMAL,
    .inheritsched = PTHREAD_EXPLICIT_SCHED,
    .detachstate  = PTHREAD_CREATE_JOINABLE,
    .stackaddr    = NULL,
    .stacksize    = 1024,
    .guardsize    = PTHREAD_GUARD_DEFAULT,
};

void* led_process(FAR void* arg)
{
    int fd_led = open(LED_DEVPATH, O_WRONLY);
    if (fd_led < 0) {
        printf("ERROR: Failed to open file %s\n", LED_DEVPATH);
        return NULL;
    }

    userled_set_t led_supported = 0;
    int ret = ioctl(fd_led, ULEDIOC_SUPPORTED, &led_supported);
    if (ret < 0) {
        printf("ERROR: failed to ioctl BTNIOC_SUPPORTED\n");
        close(fd_led);
        return NULL;
    }
    printf("btn supported 0x%08lx\n", led_supported);

    struct userled_s led1 = {
        .ul_led = 0,
        .ul_on = false
    };
    int btn_msg;

    while (1) {
        // ret = sem_wait(&btn_sem);
        ret = mq_receive(btn_mq, (char *)&btn_msg, sizeof(btn_msg), NULL);
        if (ret < 0) {
            continue;
        }
        printf("btn_msg %d\n", btn_msg);
        led1.ul_on = !(led1.ul_on);
        ret = ioctl(fd_led, ULEDIOC_SETLED, &led1);
        if (ret < 0) {
            printf("ERROR: Failed to ioctl ULEDIOC_SETLED\n");
            close(fd_led);
            return NULL;
        }
    }
}
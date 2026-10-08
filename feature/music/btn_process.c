#include <nuttx/config.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/input/buttons.h>
#include <semaphore.h>
#include <mqueue.h>

#define BTN_DEVPATH "/dev/buttons"
#define BUTTONS_SIGNO 32

pthread_t btn_thread;
pthread_attr_t btn_attr = {
    .priority     = PTHREAD_DEFAULT_PRIORITY,
    .policy       = SCHED_NORMAL,
    .inheritsched = PTHREAD_EXPLICIT_SCHED,
    .detachstate  = PTHREAD_CREATE_JOINABLE,
    .stackaddr    = NULL,
    .stacksize    = 1024,
    .guardsize    = PTHREAD_GUARD_DEFAULT,
};

sem_t btn_sem;

mqd_t btn_mq;

void* btn_process(FAR void* arg)
{
    int fd_btn = open(BTN_DEVPATH, O_RDONLY| O_NONBLOCK);
    if (fd_btn < 0) {
        printf("ERROR: Failed to open file %s\n", BTN_DEVPATH);
        return NULL;
    }

    btn_buttonset_t btn_supported = 0;
    int ret = ioctl(fd_btn, BTNIOC_SUPPORTED, &btn_supported);
    if (ret < 0) {
        printf("ERROR: failed to ioctl BTNIOC_SUPPORTED\n");
        close(fd_btn);
        return NULL;
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
        return NULL;
    }

    ret = sem_init(&btn_sem, 0, 0);
    if (ret < 0) {
        printf("ERROR: failed to btn sem init\n");
        close(fd_btn);
        return NULL;
    }

    struct mq_attr btn_mq_attr = {
        .mq_maxmsg = 3,
        .mq_msgsize = 4,
        .mq_flags = 0,
        .mq_curmsgs = 0
    };
    btn_mq = mq_open("/btn_mq", O_RDWR | O_CREAT, 0666, &btn_mq_attr);
    if (btn_mq < 0) {
        printf("ERROR: failed to open btn_mq\n");
        close(fd_btn);
        return NULL;
    }

    int btn_msg = 0;

    struct siginfo value;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, BUTTONS_SIGNO);

    while (1) {
        ret = sigwaitinfo(&set, &value);
        if (ret < 0) {
            printf("ERROR: failed to sigwaitinfo\n");
            return NULL;
        }

        btn_buttonset_t sample = (btn_buttonset_t)value.si_value.sival_int;
        printf("btn sample %08lx\n", sample);
        if (sample & 0x1) { //key0 pressed
            // sem_post(&btn_sem);
            mq_send(btn_mq, (const char *)&btn_msg, sizeof(btn_msg), 0);
            btn_msg++;
        }
    }
}
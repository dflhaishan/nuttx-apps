#ifndef MAIN_H
#define MAIN_H

extern pthread_t btn_thread;
extern pthread_attr_t btn_attr;

extern pthread_t led_thread;
extern pthread_attr_t led_attr;

void* btn_process(FAR void* arg);
void* led_process(FAR void* arg);

#endif
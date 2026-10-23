#ifndef MAIN_H
#define MAIN_H

#include <pthread.h>

extern pthread_t ui_thread;
extern pthread_attr_t ui_attr;

void* ui_process(FAR void* arg);

#endif
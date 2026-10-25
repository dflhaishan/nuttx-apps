#ifndef MAIN_H
#define MAIN_H

extern pthread_t player_thread;
extern pthread_attr_t player_attr;

extern pthread_t ui_thread;
extern pthread_attr_t ui_attr;

void* player_process(FAR void* arg);
void* ui_process(FAR void *arg);
#endif
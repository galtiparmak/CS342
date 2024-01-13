#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdarg.h>

#define MAXR 100 // max num of resource types supported
#define MAXP 100 // max num of threads supported

int rm_init(int p_count, int r_count, int r_exist[], int avoid);
int rm_thread_started(int tid);
int rm_thread_ended();
int rm_claim(int claim[]); // only for avoidance
int rm_request(int request[]);
int rm_release(int release[]);
int rm_detection();
void rm_print_state(char headermsg[]);
void* threadfunc1(void* arg);
void* threadfunc2(void* arg);

#endif /* RESOURCEMANAGER_H */
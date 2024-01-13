#ifndef SCHEDULING_H
#define SCHEDULING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

typedef struct {
    int pid;
    double burst_length;
    clock_t start_time;
    clock_t arrival_time;
    clock_t finish_time;
    long remaining_time;
    long turnaround_time;
    long waiting_time;
    int proccessorId;
} process_t;

typedef struct node {
    process_t *process;
    struct node *next;
} node_t;

typedef struct {
	int id;
    node_t *head;
    node_t *tail;
} ready_queue_t;

typedef struct {
    int id;
    ready_queue_t ready_queue;
    pthread_mutex_t queue_lock;
} processor_t;

typedef struct {
    processor_t processor;
    char* algo;
    int q;
    char* inFileName;
    char* outFileName;
    int outmode;
} threadBlock;

void queue_init(ready_queue_t *queue);
void enqueue(ready_queue_t *queue, process_t **process);
process_t* dequeue(ready_queue_t *queue);
void* run_processor(threadBlock* arg);
void readFile(processor_t* processor, char* inFileName);
void fcfs_run(process_t* process);
process_t *get_shortest_job(ready_queue_t *queue);
void execute_process(int cpu, ready_queue_t** ready_queue, char* algo, char* outFileName);

#endif /* SCHEDULING_H */

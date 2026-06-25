#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define REQ_NONE 0
#define REQ_ALLOCATE 1
#define REQ_RELEASE 2
#define REQ_QUIT 3

#define STAT_EXITED 0
#define STAT_ACTIVE 1

extern int M, N, R;
extern int *TOTAL;
extern int *AVAILABLE;
extern int **ALLOCATION;
extern int **REQUEST;
extern int **RELEASE;
extern int REQFROM;
extern int REQTYPE;
extern int NACTIVE;
extern int *STATUS;

extern int *RQ;
extern int rq_size;

extern pthread_mutex_t RMTX;
extern pthread_mutex_t SMTX;
extern pthread_cond_t SCND;
extern pthread_mutex_t AMTX;
extern pthread_cond_t ACND;
extern pthread_mutex_t *WMTX;
extern pthread_cond_t *WCND;

void* worker_thread(void* arg);
void* dlhandler_thread(void* arg);

#endif

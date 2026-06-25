#include <stdio.h>
#include <pthread.h>
#include <string.h>

#ifndef GLOBAL
#define GLOBAL


#define MAXSIZE 100
#define BUFFSIZE 1024
#define MAXTHREAD 16

char* BAR_FILE = "demobar.txt";
char* FOO_FILE = "demofoo.txt";

typedef struct{
    int vector[MAXSIZE];
    int end_idx;
} vector;

int push_back(vector* vec, int value){
    int end_idx = vec->end_idx;
    if(end_idx < MAXSIZE){
        vec->vector[end_idx] = value;
        vec->end_idx++;
        return end_idx - 1;
    }
    return -1;
}

typedef struct {
    int n_workers;
    int n_parts;

    vector dependencies_graph[MAXSIZE];
    vector prerequistes_graph[MAXSIZE];
    int part_worker_map[MAXSIZE];
    vector workers_assignment[MAXSIZE];
} manufacture_info;

typedef enum {
    PART_PENDING,
    PART_DONE,
} part_status;

typedef enum {
    WORKER_START,
    WORKER_WORKING,
    WORKER_WAITING,
    WORKER_DONE,
} worker_status;

typedef enum {
    FOO,
    BAR,
} car;

void printTab(int n){
    for(int i = 0; i < n; i++){
        printf("\t");
    }
    fflush(stdout);
}



//Global Variables
manufacture_info foo_info;
manufacture_info bar_info;

part_status PSTAT[MAXSIZE];     // Status of each part
worker_status WSTAT[MAXTHREAD]; // Status of each worker
int WINFO[MAXTHREAD];           // If worker is working or waiting, array contains the part that it is for
int worker_id[MAXTHREAD];

pthread_t workers[MAXTHREAD];
pthread_cond_t cnd[MAXTHREAD];
pthread_mutex_t mtx;
pthread_barrier_t bop, eop;

int M = 0;
int f = 0; int b = 0;

char print_buffer[BUFFSIZE];

void initBuffer() {
    memset(print_buffer, ' ', BUFFSIZE - 1);
    print_buffer[BUFFSIZE - 1] = '\0';
}

void insertBuffer(int worker_id, const char* c){
     int start = 8 + worker_id * 9;
     if (start + strlen(c) < BUFFSIZE) {
         memcpy(print_buffer + start, c, strlen(c));
     }
}

void printBuffer(){
    print_buffer[8 + M * 9] = 0;
    printf("%s\n", print_buffer);
    memset(print_buffer, ' ', BUFFSIZE - 1);
}

car current_car;

#endif /* ifndef GLOBAL */

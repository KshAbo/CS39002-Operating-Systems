#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sem.h>
#include <wait.h>

#define S 100
#define N 100
#define CPU_BURST_CNT 11
#define IO_BURST_CNT 10

static const char* ANSI_BLACK = "\033[0;30m";
static const char* ANSI_RED = "\033[0;31m";
static const char* ANSI_GREEN = "\033[0;32m";
static const char* ANSI_YELLOW = "\033[0;33m";
static const char* ANSI_BLUE = "\033[0;34m";
static const char* ANSI_PURPLE = "\033[0;35m";
static const char* ANSI_CYAN = "\033[0;36m";
static const char* ANSI_WHITE = "\033[0;37m";
static const char* ANSI_RESET = "\033[0;0m";

const char* FILE_NAME = "bursts.txt";

const int BUFF_SIZE = 2048;

const int KEY1 = 101;
const int KEY2 = 102;
const int KEY3 = 103;
const int KEY4 = 104;

const int UPPER_DELTA = 50;
const int LOWER_DELTA = 5;



/////////////////////////////////
//  ____       _
// |  _ \  ___| |__  _   _  __ _
// | | | |/ _ \ '_ \| | | |/ _` |
// | |_| |  __/ |_) | |_| | (_| |
// |____/ \___|_.__/ \__,_|\__, |
//                         |___/
/////////////////////////////////

void debugLog(const char* format, ...){
#ifdef DEBUG

    va_list args;

    va_start(args, format);

    printf("%s", ANSI_YELLOW);
    vprintf(format, args);
    printf("%s", ANSI_RESET);

    va_end(args);
#endif /* ifdef DEBUG */
}

void debugSuccess(const char* format, ...){
#ifdef DEBUG

    va_list args;

    va_start(args, format);

    printf("%s", ANSI_GREEN);
    vprintf(format, args);
    printf("%s", ANSI_RESET);

    va_end(args);
#endif /* ifdef DEBUG */
}

void debugError(const char* format, ...){
#ifdef DEBUG

    va_list args;

    va_start(args, format);

    printf("%s", ANSI_RED);
    vprintf(format, args);
    printf("%s", ANSI_RESET);

    va_end(args);
#endif /* ifdef DEBUG */
}


////////////////////////////////////////
//  ____
// |  _ \ _ __ ___   ___ ___  ___ ___
// | |_) | '__/ _ \ / __/ _ \/ __/ __|
// |  __/| | | (_) | (_|  __/\__ \__ \
// |_|   |_|  \___/ \___\___||___/___/
////////////////////////////////////////
typedef enum {
    READY,
    RUNNING,
    IO,
    EXITED,
} process_state;

typedef struct {
    int serial;
    pid_t pid;
    int priority;
    process_state state;

} process_control_block;

const int PRIORITY_0_QUANTUM = 10;
const int PRIORITY_1_QUANTUM = 5;
const int PRIORITY_2_QUANTUM = 2;


/////////////////////////////////
//   ___
//  / _ \ _   _  ___ _   _  ___
// | | | | | | |/ _ \ | | |/ _ \
// | |_| | |_| |  __/ |_| |  __/
//  \__\_\\__,_|\___|\__,_|\___|
/////////////////////////////////

typedef struct {
    int queue[S];
    int size;
    int back_idx;
    int front_idx;
} circular_queue;

void initQueue(circular_queue* cq){
    cq->front_idx = 0;
    cq->back_idx = 0;
    cq->size = 0;
}

int queueEmpty(circular_queue* cq){
    return (cq->size == 0) ? 1 : 0;
}

int queuePush(circular_queue* cq, int value){
    if(cq->front_idx == cq->back_idx && cq->size == S){
        debugLog("Queue is full\n");
        return -1;
    }

    int back_idx = (cq->back_idx + 1) % S;
    cq->queue[cq->back_idx] = value;
    cq->back_idx = back_idx;
    cq->size++;
    return 1;
}

int queuePop(circular_queue* cq){
    if(cq->front_idx == cq->back_idx && cq->size == 0){
        debugLog("Queue is empty\n");
        return -1;
    }

    int ret = cq->queue[cq->front_idx];
    int front_idx = (cq->front_idx + 1) % S;
    cq->front_idx = front_idx;
    cq->size--;
    return ret;
}


/////////////////////////////////
//  ____                             _
// / ___|  ___ _ __ ___   __ _ _ __ | |__   ___  _ __ ___
// \___ \ / _ \ '_ ` _ \ / _` | '_ \| '_ \ / _ \| '__/ _ \
//  ___) |  __/ | | | | | (_| | |_) | | | | (_) | | |  __/
// |____/ \___|_| |_| |_|\__,_| .__/|_| |_|\___/|_|  \___|
//                            |_|
//  _   _      _
// | | | | ___| |_ __   ___ _ __
// | |_| |/ _ \ | '_ \ / _ \ '__|
// |  _  |  __/ | |_) |  __/ |
// |_| |_|\___|_| .__/ \___|_|
//              |_|
/////////////////////////////////

void semWait(int semid){
    struct sembuf sop;
    sop.sem_flg = SEM_UNDO;
    sop.sem_num = 0;
    sop.sem_op = -1;
    semop(semid, &sop, 1);
}

void semWaitForZero(int semid){
    struct sembuf sop;
    sop.sem_flg = SEM_UNDO;
    sop.sem_num = 0;
    sop.sem_op = 0;
    semop(semid, &sop, 1);
}

void semSignal(int semid){
    struct sembuf sop;
    sop.sem_flg = SEM_UNDO;
    sop.sem_num = 0;
    sop.sem_op = 1;
    semop(semid, &sop, 1);
}

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

static const char* ANSI_BLACK = "\033[0;30m";
static const char* ANSI_RED = "\033[0;31m";
static const char* ANSI_GREEN = "\033[0;32m";
static const char* ANSI_YELLOW = "\033[0;33m";
static const char* ANSI_BLUE = "\033[0;34m";
static const char* ANSI_PURPLE = "\033[0;35m";
static const char* ANSI_CYAN = "\033[0;36m";
static const char* ANSI_WHITE = "\033[0;37m";
static const char* ANSI_RESET = "\033[0;0m";


const int PROJID0 = 100;
const int PROJID1 = 101;
const int PROJID2 = 102;
const int PROJID3 = 103;

const int BUFF_SIZE = 2048;


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

void semWait(int semid, int sem_num){
    struct sembuf sop;
    sop.sem_flg = SEM_UNDO;
    sop.sem_num = sem_num;
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

void semSignal(int semid, int sem_num){
    struct sembuf sop;
    sop.sem_flg = SEM_UNDO;
    sop.sem_num = sem_num;
    sop.sem_op = 1;
    semop(semid, &sop, 1);
}

/////////////////////////////////
//  _   _
// | | | | ___  _   _ ___  ___
// | |_| |/ _ \| | | / __|/ _ \
// |  _  | (_) | |_| \__ \  __/
// |_| |_|\___/ \__,_|___/\___|
/////////////////////////////////

typedef enum house_state{
    EMPTY,
    DEMON_INSIDE,
    NOMAD_INSIDE,
} house_state;

typedef struct house{
    house_state state;
    int count;
    int wait_count;
} house;

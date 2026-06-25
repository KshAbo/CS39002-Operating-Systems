#include "shared.c"
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

typedef struct {
    int semid; // mtx, cnd
    int count_shmid;
} cond_t;

cond_t cond_create(key_t token1, key_t token2){
    int semid = semget(token1, 2, 0666 | IPC_CREAT | IPC_EXCL);
    int count_shmid = shmget(token2, sizeof(int), 0666 | IPC_CREAT | IPC_EXCL);

    cond_t ret_cond = {semid, count_shmid};
    return ret_cond;
}

void cond_init(cond_t cond){
    semctl(cond.semid, 0, SETVAL, 1);
    semctl(cond.semid, 1, SETVAL, 0);

    int* count_ptr = shmat(cond.count_shmid, 0, 0);
    *count_ptr = 0;
}

void cond_lock(cond_t cond){
    semWait(cond.semid, 0);
}

void cond_unlock(cond_t cond){
    semSignal(cond.semid, 0);
}

void cond_wait(cond_t cond){
    // mtx should be locked before this
    int* count_ptr = shmat(cond.count_shmid, 0, 0);
    *count_ptr += 1;

    semSignal(cond.semid, 0);

    sleep(1);

    semWait(cond.semid, 1);

    semWait(cond.semid, 0);
    *count_ptr -= 1;

    shmdt(count_ptr);
}

void cond_signal(cond_t cond){
    int* count_ptr = shmat(cond.count_shmid, 0, 0);

    if(*count_ptr > 0){
        semSignal(cond.semid, 1);
    }

    shmdt(count_ptr);
}

void cond_broadcast(cond_t cond){
    int* count_ptr = shmat(cond.count_shmid, 0, 0);

    if(*count_ptr > 0){
        struct sembuf sop;
        sop.sem_flg = SEM_UNDO;
        sop.sem_num = 1;
        sop.sem_op = *count_ptr;
        semop(cond.semid, &sop, 1);
    }

    shmdt(count_ptr);
}

void cond_destroy(cond_t cond){
    semctl(cond.semid, 0, IPC_RMID, 0);
    shmctl(cond.count_shmid, IPC_RMID, 0);
}

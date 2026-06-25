#include "cond.c"
#include <sys/shm.h>

int H_shmid = -1;
int CV_shmid = -1;


void quit(){
    if(H_shmid != -1) shmctl(H_shmid, IPC_RMID, 0);
    if(CV_shmid != -1) {
        cond_t* CV_ptr = shmat(CV_shmid, 0, 0);
        cond_destroy(*CV_ptr);
        shmctl(CV_shmid, IPC_RMID, 0);
    }
    exit(0);
}

void initIpc(){
    H_shmid = shmget(PROJID0, sizeof(house), 0666 | IPC_CREAT | IPC_EXCL);
    CV_shmid = shmget(PROJID1, sizeof(cond_t), 0666 | IPC_CREAT | IPC_EXCL);

    cond_t* CV_ptr = shmat(CV_shmid, 0, 0);
    house* H_ptr = shmat(H_shmid, 0, 0);

    H_ptr->state = EMPTY;
    H_ptr->wait_count = 0;
    H_ptr->count = 0;

    *CV_ptr = cond_create(PROJID2, PROJID3);
    cond_init(*CV_ptr);
}


void handleInterrupt(int signal){
    quit();
}

int main(int argc, char *argv[]){
    signal(SIGINT, handleInterrupt);
    initIpc();
    printf("Manager running...\n");
    while(1);
    return 0;
}

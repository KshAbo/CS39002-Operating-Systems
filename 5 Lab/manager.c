#include "shared.c"
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

// SHARED VARIABLES
int queue_shm_id = -1;
int pcb_table_shm_id = -1;
int timer_shm_id = -1;

int queue_sem_id = -1;
int pcb_sem_id = -1;
int timer_sem_id = -1;
int sync_sem_id = -1;


void initIpc(){

    queue_shm_id = shmget((key_t)KEY1, sizeof(circular_queue), 0666 | IPC_CREAT | IPC_EXCL);

    pcb_table_shm_id = shmget((key_t)KEY2, sizeof(process_control_block) * N, 0666 | IPC_CREAT | IPC_EXCL);

    //timer[0] = global t
    //timer[1] = serial number of running process else -1
    //timer[2] = time for sending next interrupt
    //timer[3] = PID of manager
    //timer[4] = PID of timer
    timer_shm_id = shmget((key_t)KEY3, sizeof(int) * 5, 0666 | IPC_CREAT | IPC_EXCL);

    queue_sem_id = semget((key_t)KEY1, 1, 0666 | IPC_CREAT | IPC_EXCL);
    pcb_sem_id = semget((key_t)KEY2, 1, 0666 | IPC_CREAT | IPC_EXCL);
    timer_sem_id = semget((key_t)KEY3, 1, 0666 | IPC_CREAT | IPC_EXCL);
    sync_sem_id = semget((key_t)KEY4, 1, 0666 | IPC_CREAT | IPC_EXCL);

    int* timer_ptr = shmat(timer_shm_id, NULL, 0);
    timer_ptr[3] = getpid();

    semctl(queue_sem_id, 0, SETVAL, 1);
    semctl(pcb_sem_id, 0, SETVAL, 1);
    semctl(timer_sem_id, 0, SETVAL, 1);
    semctl(sync_sem_id, 0, SETVAL, 0);
}

void quit(){
    if(queue_shm_id != -1)
        shmctl(queue_shm_id, IPC_RMID, NULL);
    if(pcb_table_shm_id != -1)
        shmctl(pcb_table_shm_id, IPC_RMID, NULL);
    if(timer_shm_id != -1)
        shmctl(timer_shm_id, IPC_RMID, NULL);

    if(queue_sem_id != -1)
        semctl(queue_sem_id, 0, IPC_RMID, NULL);
    if(pcb_sem_id != -1)
        semctl(pcb_sem_id, 0, IPC_RMID, NULL);
    if(timer_sem_id != -1)
        semctl(timer_sem_id, 0, IPC_RMID, NULL);
    if(sync_sem_id != -1)
        semctl(sync_sem_id, 0, IPC_RMID, NULL);

    exit(0);
}

void interruptHandler(int signal){
    quit();
}

int main(int argc, char *argv[]){

    signal(SIGINT, interruptHandler);
    initIpc(); 

    debugLog("[manager] This is the logging print\n");
    debugSuccess("[manager] This is the success print\n");
    debugError("[manager] This is the error print\n");

    while(1);

    return 0;
}

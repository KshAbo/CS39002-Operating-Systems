#include "shared.c"
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
/////////////////////////

circular_queue* rq_ptr = NULL;
process_control_block* pcb_table_ptr = NULL;
int* timer_ptr = NULL;


void handleTimeout(){
    if(timer_ptr[2] != -1 && timer_ptr[1] != -1 && timer_ptr[0] == timer_ptr[2]){
        debugLog("[timer :: handleTimeout] Interrupt Signal sent to Process %d\n", pcb_table_ptr[timer_ptr[1]].serial);
        kill(pcb_table_ptr[timer_ptr[1]].pid, SIGUSR1);
    }
}

void timerLoop(){


    debugLog("[timer] I am running at the start atleast\n");

    semWait(sync_sem_id);
    semWait(timer_sem_id);
    *timer_ptr = 0;
    semSignal(timer_sem_id);
    semSignal(sync_sem_id);

    debugLog("[timer] sync_sem_id got released\n");

    while(1){

        usleep((UPPER_DELTA + LOWER_DELTA) * 1000);

        semWait(timer_sem_id);
        *timer_ptr += 1;
        semSignal(timer_sem_id);

        handleTimeout();

        semWait(sync_sem_id);

        usleep(LOWER_DELTA * 1000);

        semSignal(sync_sem_id);

    }
}

void initIpc(){
    queue_shm_id = shmget((key_t)KEY1, sizeof(circular_queue), 0666 | IPC_CREAT);
    pcb_table_shm_id = shmget((key_t)KEY2, sizeof(process_control_block) * N, 0666 | IPC_CREAT);
    timer_shm_id = shmget((key_t)KEY3, sizeof(int) * 5, 0666 | IPC_CREAT);

    queue_sem_id = semget((key_t)KEY1, 1, 0666 | IPC_CREAT);
    pcb_sem_id = semget((key_t)KEY2, 1, 0666 | IPC_CREAT);
    timer_sem_id = semget((key_t)KEY3, 1, 0666 | IPC_CREAT);
    sync_sem_id = semget((key_t)KEY4, 1, 0666 | IPC_CREAT);

    rq_ptr = shmat(queue_shm_id, NULL, 0);
    pcb_table_ptr = shmat(pcb_table_shm_id, NULL, 0);
    timer_ptr = shmat(timer_shm_id, NULL, 0);

    timer_ptr[0] = 0;        // Global Time
    timer_ptr[1] = -1;       // Running Process Serial
    timer_ptr[2] = -1;       // Next Interrupt Time
    timer_ptr[4] = getpid(); // Timer PID
}

void quit(){
    if(queue_shm_id != -1)
        shmdt(rq_ptr);
    if(pcb_table_shm_id != -1)
        shmdt(pcb_table_ptr);
    if(timer_shm_id != -1)
        shmdt(timer_ptr);

    // if(queue_sem_id != -1)
    //     semctl(queue_sem_id, 0, IPC_RMID, NULL);
    // if(pcb_sem_id != -1)
    //     semctl(pcb_sem_id, 0, IPC_RMID, NULL);
    // if(timer_sem_id != -1)
    //     semctl(timer_sem_id, 0, IPC_RMID, NULL);
    // if(sync_sem_id != -1)
    //     semctl(sync_sem_id, 0, IPC_RMID, NULL);

    exit(0);
}

void interruptHandler(int signal){
    quit();
}

int main(int argc, char *argv[]){

    signal(SIGINT, interruptHandler);

    initIpc();

    timerLoop();

    return 0;
}

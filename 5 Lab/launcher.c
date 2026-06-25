#include "shared.c"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int manager_pid = -1;
int timer_pid = -1;

int n_proc = 0;
int local_time = 0;

int children_pids[N];
int child_count = 0;

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

    manager_pid = *(timer_ptr + 3);
    timer_pid = *(timer_ptr + 4);

}

void quit(){

    if(timer_ptr[3] != -1) kill(timer_ptr[3], SIGINT);
    if(timer_ptr[4] != -1) kill(timer_ptr[4], SIGINT);

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

void tick_clock() {
    usleep(UPPER_DELTA * 1000);
    
    int next_time = local_time + 1;

    semWaitForZero(sync_sem_id); 
    
    local_time = next_time;
}

void forkProcess(int serial, char* line_buffer) {
    pid_t pid = fork();

    if (pid == 0) {
        
        char serial_str[10];
        sprintf(serial_str, "%d", serial);

        char* args[4 + CPU_BURST_CNT + IO_BURST_CNT + 1];
        int arg_idx = 0;

        args[arg_idx++] = "./process";
        args[arg_idx++] = serial_str;

        char* token = strtok(line_buffer, " \n");
        while (token != NULL && arg_idx < 26) {
            args[arg_idx++] = token;
            token = strtok(NULL, " \n");
        }
        args[arg_idx] = NULL;

        execv("./process", args);
        
        debugError("Execv failed");
        exit(1);
    } else {
        children_pids[child_count++] = pid;
        debugLog("[launcher] Forked Process %d at time %d\n", serial, local_time);
    }
}

void launcherLoop() {

    printf("[%d] Launcher Ready\n", local_time);

    char line[BUFF_SIZE];
    char line_copy[BUFF_SIZE];
    
    int next_arrival = -1;
    int process_serial_counter = 0;
    int valid_process_pending = 0;

    FILE* file_ptr;
    file_ptr = fopen(FILE_NAME, "r");
    if (!file_ptr) { 
        debugError("Failed to open file %s\n", FILE_NAME);
        quit();
    }

    debugLog("[launcher :: launcherLoop] Reading file %s\n", FILE_NAME);

    if (fgets(line, BUFF_SIZE, file_ptr)) {
        if (strncmp(line, "-1", 2) != 0) {
            strcpy(line_copy, line);
            char* token = strtok(line_copy, " ");
            if (token) {
                next_arrival = atoi(token);
                valid_process_pending = 1;
            }
        }
    }

    while (1) {
        while (valid_process_pending && local_time == next_arrival) {
            forkProcess(process_serial_counter++, line);

            valid_process_pending = 0;
            if (fgets(line, BUFF_SIZE, file_ptr)) {
                 if (strncmp(line, "-1", 2) != 0) { 
                    strcpy(line_copy, line);
                    char* token = strtok(line_copy, " ");
                    if (token) {
                        next_arrival = atoi(token);
                        valid_process_pending = 1;
                    }
                 }
            }
        }

        if (!valid_process_pending) {
            int status;
            pid_t res = waitpid(-1, &status, WNOHANG);
            break; 
        }

        tick_clock();
    }
    
    fclose(file_ptr);
    
    debugLog("[launcher] Input file exhausted. Waiting for children to finish...\n");
    
    while (child_count > 0) {
        wait(NULL);
        child_count--;
    }
    
    printf("\t\t[Launcher] All processes exited\n");
    quit();
}


int main(int argc, char *argv[]){
    
    signal(SIGINT, interruptHandler);

    initIpc();

    launcherLoop();

    return 0;
}

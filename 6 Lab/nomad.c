#include "cond.c"
#include <stdlib.h>
#include <sys/shm.h>
#include <unistd.h>

int H_shmid = -1;
int CV_shmid = -1;

house* H_ptr = 0;
cond_t* CV_ptr = 0;

int n = 10;

int nomad_id = -1;

void print_nomad(const char* format, ...) {
    char buffer[256];
    
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    
    printf("%100s", buffer);
}

void nomadLoop(){

    while(1){
        cond_lock(*CV_ptr);
        while(H_ptr->state == DEMON_INSIDE){ 
            print_nomad("Nomad %d arrives (D_CNT = %d, N_CNT = 0, state = D_INSIDE)\n", nomad_id, H_ptr->count);
            H_ptr->wait_count++;
            print_nomad("Nomad %d waits (NW_CNT = %d)\n", nomad_id, H_ptr->wait_count);
            cond_wait(*CV_ptr);
            H_ptr->wait_count--;
        }
        if(H_ptr->state == EMPTY){
            print_nomad("Nomad %d arrives (D_CNT = 0, N_CNT = 0, state = EMPTY)\n", nomad_id);
        }else if(H_ptr->state == NOMAD_INSIDE){
            print_nomad("Nomad %d arrives (D_CNT = 0, N_CNT = %d, state = N_INSIDE)\n", nomad_id, H_ptr->count);
        }
        H_ptr->count++; 
        if(H_ptr->count == 1) {
            H_ptr->state = NOMAD_INSIDE;
            print_nomad("Nomad %d enters [house empty] (D_CNT = 0, N_CNT = %d, state = N_INSIDE)\n", nomad_id, H_ptr->count);
            if(H_ptr->wait_count > 0) cond_broadcast(*CV_ptr);
        }else{
            print_nomad("Nomad %d enters [other nomads present] (D_CNT = 0, N_CNT = %d, state = N_INSIDE)\n", nomad_id, H_ptr->count);
        }
        cond_unlock(*CV_ptr);

        int time_delay = 500 + rand() % 501;
        usleep(time_delay * 1000);

        cond_lock(*CV_ptr);
        H_ptr->count--;
        if(H_ptr->count == 0) {
            H_ptr->state = EMPTY;
            printf("Nomad %d leaves (D_CNT = 0, N_CNT = %d, state = EMPTY)\n", nomad_id, H_ptr->count);
            if(H_ptr->wait_count > 0){
                cond_broadcast(*CV_ptr);
            }
        }else{
            printf("Nomad %d leaves (D_CNT = 0, N_CNT = %d, state = N_INSIDE)\n", nomad_id, H_ptr->count);
        }
        cond_unlock(*CV_ptr);

        time_delay = 1 + rand() % 5;
        sleep(time_delay);
    }

}

void quit(){
    if(H_ptr != 0) shmdt(H_ptr);
    if(CV_ptr != 0) shmdt(CV_ptr);
    exit(0);
}

void initIpc(){
    H_shmid = shmget(PROJID0, sizeof(house), 0666 | IPC_CREAT);
    CV_shmid = shmget(PROJID1, sizeof(cond_t), 0666 | IPC_CREAT);
    H_ptr = shmat(H_shmid, 0, 0);
    CV_ptr = shmat(CV_shmid, 0, 0);
}


void handleInterrupt(int signal){
    quit();
}

int main(int argc, char *argv[]){

    if(argc == 2){
        n = atoi(argv[1]);
    }

    signal(SIGINT, handleInterrupt);
    initIpc();

    for(int i = 0; i < n; i++){
        int pid = fork();
        if(pid == 0){
            nomad_id = i;
            nomadLoop();
        }
    }

    for(int i = 0; i < n; i++){
        wait(NULL);
    }

    printf("All nomad process left, closing\n");
    quit();

    return 0;
}

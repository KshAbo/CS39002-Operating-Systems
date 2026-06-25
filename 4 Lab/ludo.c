#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>

const char* KEY = "./ludo.c";
const int PROJ_ID1 = 0;
const int PROJ_ID2 = 1;

const char* FILENAME = "ludo.txt";
const int BUFF_SIZE = 2048;

const char* ACK = "ACK";

int SAVED_STDOUT = -1;
int SAVED_STDIN = -1;

int n;

int shm_board_id = -1;
int shm_players_id = -1;

int pipe_p[2];

int pid_xbp;
int pid_xpp;
int pid_bp;
int pid_pp;

int* shm_board;
int* shm_players;

int delay = 0;


void quit(){

    if (shm_board_id != -1)
        shmctl(shm_board_id, IPC_RMID, 0);
    if (shm_players_id != -1)
        shmctl(shm_players_id, IPC_RMID, 0);

    exit(0);

}

int processBoardFile(char* filename){
    /* 
    * Initialize shared memory for board
    **/

    FILE *file_ptr = fopen(filename, "r");
    if(file_ptr == NULL){
        printf("Can't open file: %s\n", filename);
        exit(1);
    }

    char line[BUFF_SIZE];
    while (fgets(line, BUFF_SIZE, file_ptr)) {
        char* type = strtok(line, " "); 
        if(strcmp(type, "S") == 0 || strcmp(type, "L") == 0){
            int first = atoi(strtok(NULL, " "));
            int second = atoi(strtok(NULL, " "));
            *(shm_board + first) = second - first;
        }else if(strcmp(type, "E") == 0)
            break;
    }
    return 0;
}

void printShm(int* start, int count){
    /* 
    * DEBUGGING: print the contents of shared memory
    **/

    for(int i = 0; i < count; i += 1){
        printf("%d ", *(start + i));
    }
    printf("\n");
}

void initShm(){
    /* 
    * Initialization of Shared Memory
    **/

    shm_board_id = shmget(ftok(KEY, PROJ_ID1), sizeof(int) * 101, 0666 | IPC_CREAT | IPC_EXCL);
    shm_board = (int*)shmat(shm_board_id, NULL, 0);
    memset(shm_board, 0, sizeof(int) * 101);

    shm_players_id = shmget(ftok(KEY, PROJ_ID2), sizeof(int) * (n + 1), 0666 | IPC_CREAT | IPC_EXCL);
    shm_players = (int*)shmat(shm_players_id, NULL, 0);
    memset(shm_players, 0, sizeof(int) * (n + 1));
    shm_players[n] = n;
}

int init(int argc, char* argv[]){
    /* 
    * Initialization of Coordinator Process
    **/

    if(argc != 2){
        printf("USAGE\n\t./ludo <number>\n");
        printf("\tor\n");
        printf("\tmake run n=<number>\n");
        exit(1);
    }

    SAVED_STDIN = dup(STDIN_FILENO);
    SAVED_STDOUT = dup(STDOUT_FILENO);

    n = atoi(argv[1]);

    initShm();

    processBoardFile((char*)FILENAME);

#ifdef DEBUG
    // printShm(shm_board, 101);
    // printShm(shm_players, n + 1);
#endif /* ifdef DEBUG */

    if(pipe(pipe_p) != 0)
        quit();

    char narg[100], pfarg[100];
    sprintf(narg, "%d", n);
    sprintf(pfarg, "%d", pipe_p[1]);

    pid_xbp = fork();
#ifdef DEBUG
    printf("xbp: %d\n", pid_xbp);
#endif /* ifdef DEBUG */
    if(pid_xbp == 0){
#ifdef DEBUG
        printf("Board Terminal should pop up\n");
#endif /* ifdef DEBUG */
        execl("/usr/bin/xterm", "board", "-T", "Board", "-fs", "15", "-geometry", "150x24+50+100", "-bg", "#003300", "-fg", "#FFF", "-e", "./board", narg, pfarg, NULL);
    }

    dup2(pipe_p[0], STDIN_FILENO);
    scanf("%d", &pid_bp);

#ifdef DEBUG
    printf("PID of BP: %d\n", pid_bp);
    void *shm_board_ptr, *shm_players_ptr;
    scanf("%p", &shm_board_ptr);
    scanf("%p", &shm_board_ptr);
    printf("shm_board: %p\n", shm_board);
    printf("shm_board_ptr: %p\n", shm_board_ptr);
    printf("shm_players: %p\n", shm_players);
    printf("shm_players_ptr: %p\n", shm_players_ptr);
#endif /* ifdef DEBUG */


    char brdarg[100];
    sprintf(brdarg, "%d", pid_bp);

    pid_xpp = fork();
#ifdef DEBUG
    printf("xpp: %d\n", pid_xpp);
#endif /* ifdef DEBUG */
    if(pid_xpp == 0){
#ifdef DEBUG
        printf("Player Terminal should pop up\n");
#endif /* ifdef DEBUG */
        execl("/usr/bin/xterm", "player", "-T", "Players", "-fs", "15", "-geometry", "150x24+1000+100", "-bg", "#000033", "-fg", "#FFF", "-e", "./players", narg, pfarg, brdarg, NULL);
    }

    dup2(pipe_p[0], STDIN_FILENO);
    scanf("%d", &pid_pp);

    char ack_msg[100];
    dup2(pipe_p[0], STDIN_FILENO);
    scanf("%s", ack_msg);
#ifdef DEBUG
    printf("ack_msg: %s\n", ack_msg);
#endif /* ifdef DEBUG */
    if(strcmp(ack_msg, ACK) != 0)
        quit();
#ifdef DEBUG
    printf("BP Initialization Acknowledgment message received\n");
#endif /* ifdef DEBUG */

    return 0;
}


void handleInterrupt(int sig){
    quit();
}

int play(){
    /* 
    * Play the move, returns 0 after a successful play
    * returns -1 if all players reached the end
    **/

    kill(pid_pp, SIGUSR1);

    char ack_msg[100];
    dup2(pipe_p[0], STDIN_FILENO);
    scanf("%s", ack_msg);
    if(strcmp(ack_msg, ACK) != 0){
        printf("Wrong Acknowledgment message\n");
        return 1;
    }
    if(shm_players[n] == 0) return -1;
    return 0;
}

void terminateGame(){
    /* 
    * Terminates the game and sends the signal to end child processes
    **/

    kill(pid_pp, SIGUSR2);
    waitpid(pid_xpp, NULL, 0);
    kill(pid_bp, SIGUSR2);
    waitpid(pid_xbp, NULL, 0);
    quit();
}

void autoplay(){
    printf("Starting autoplay...\n");
    if(delay == 0) delay = 1000;

    while(play() == 0)
        usleep(delay * 1000);

    printf("Hit return to end the game...\n");

    dup2(SAVED_STDIN, STDIN_FILENO);
    char c;
    while((c = getchar()) && c != '\n' && c != EOF);

    terminateGame();
}

void handleUserInput(){
    /* 
    * Handle user command
    **/

    char usr_input[BUFF_SIZE];
    printf("$ ");
    fflush(stdout);

    // Scan for user command
    dup2(SAVED_STDIN, STDIN_FILENO);
    scanf("%s", usr_input);

    if(strcmp(usr_input, "next") == 0 || strcmp(usr_input, "n") == 0){
        if(play() != 0){
            dup2(SAVED_STDIN, STDIN_FILENO);
            printf("Hit return to end the game...\n");
            char c;
            while((c = getchar()) && c != '\n' && c != EOF);
            terminateGame();
        }
    }else if(strcmp(usr_input, "quit") == 0 || strcmp(usr_input, "q") == 0){
        terminateGame();
    }else if(strcmp(usr_input, "delay") == 0){
        scanf("%d", &delay);
    }else if(strcmp(usr_input, "autoplay") == 0 || strcmp(usr_input, "a") == 0){
        autoplay();
    }
}

int main(int argc, char *argv[]){

    signal(SIGINT, handleInterrupt);

    init(argc, argv);

    while(1){
        handleUserInput();
    }
    return 0;
}

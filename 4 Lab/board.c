#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/shm.h>


const char* KEY = "./ludo.c";
const int PROJ_ID1 = 0;
const int PROJ_ID2 = 1;

const char* FILENAME = "ludo.txt";
const int BUFF_SIZE = 1024;

int shm_board_id = -1;
int shm_players_id = -1;

int* shm_board;
int* shm_players;

char* snake_ladder_info[10];

const char* ACK = "ACK";

int fd_write;
int n;

void quit(){

    for(int i = 0; i < 10; i++)
        free(snake_ladder_info[i]);

    shmdt(shm_board);
    shmdt(shm_players);

    exit(0);
}

void printFinished(){
    /* 
    * Prints finished players at the top of the board 
    **/
    for(int i = 0; i < n; i++){
        if(shm_players[i] == 100) printf("%c ", 'A' + i);
    }
    printf("\n");
}

void printStarted(){
    /* 
    * Prints players at position 0
    **/
    for(int i = 0; i < n; i++){
        if(shm_players[i] == 0) printf("%c ", 'A' + i);
    }
    printf("\n");
}

void printBoard(){
    printFinished();
    char* edge = "+---+---+---+---+---+---+---+---+---+---+\n";

    printf("%s", edge);
    for(int i = 100; i >= 1; i -= 10) {
        
        char line[BUFF_SIZE];
        line[0] = '\0';
        int start = ((i / 10) % 2) ? i - 9 : i;
        int offset = ((i / 10) % 2) ? 1 : -1;
        for(int k = 0; k < 10; k++){
            int j = start + k * offset;
            char cell_value[100] = "\0";
            for(int l = 0; l < n; l++){
                if(shm_players[l] == j && j != 100) {
                    char player = 'A' + l;
                    sprintf(cell_value, " %c ", player);
                    break;
                }
            }
            if(cell_value[0] == '\0'){
                sprintf(cell_value, "%3d", j);
            }

            strcat(line, "|");
            strcat(line, cell_value);
        }

        strcat(line, "|\t\t");
        strcat(line, snake_ladder_info[(i - 1) / 10]);
        printf("%s", line);
        printf("%s", edge);
    }
    printStarted();
}

int initShm(){
    /* 
    * Initialize shared memory connection 
    **/

    shm_board_id = shmget(ftok(KEY, PROJ_ID1), sizeof(int) * 101, 0666);
    shm_board = shmat(shm_board_id, NULL, 0666);
    shm_players_id = shmget(ftok(KEY, PROJ_ID2), sizeof(int) * (n + 1), 0666);
    shm_players = shmat(shm_players_id, NULL, 0666);
    return 0;
}

void sendAck(){
    /* 
    * Send Acknowledgement to Coordinator Process
    **/

    dprintf(fd_write, "%s ", ACK);
}

void initSnakeLadderInfo(){
    /* 
    * Initialize the information that gets printed at the side of the board
    **/

    for(int i = 0; i < 10; i++){
        snake_ladder_info[i] = (char*)malloc(sizeof(char) * BUFF_SIZE);
        snake_ladder_info[i][0] = '\0';
    }

    FILE* file_ptr = fopen(FILENAME, "r");
    if(file_ptr == NULL){
        printf("Can't open file: %s\n", FILENAME);
        quit();
    }

#ifdef DEBUG
    printf("%s successfully opened\n", FILENAME);
#endif /* ifdef DEBUG */

    char line[BUFF_SIZE];
    while(fgets(line, BUFF_SIZE, file_ptr)){
        if(strcmp(line, "E\n") == 0) {
#ifdef DEBUG
            printf("Successfully read the file %s\n", FILENAME);
            sleep(1);
#endif /* ifdef DEBUG */
            break;
        }

        char* type = strtok(line, " ");

        char* first_c = strtok(NULL, " ");
        int first = atoi(first_c);

        char* second_c = strtok(NULL, " ");
        int second = atoi(second_c);

        char info[BUFF_SIZE];
#ifdef DEBUG
        printf("%s(%2d -> %2d) ", type, first, second);
        fflush(stdout);
        sleep(1);
#endif /* ifdef DEBUG */
        sprintf(info, "%s(%2d -> %2d) ", type, first, second);
        int row = first / 10;
        if (row >= 0 && row < 10) {
            strcat(snake_ladder_info[row], info);
        }
    }

    for(int i = 0; i < 10; i++){
#ifdef DEBUG
        printf("Successfully attached newline to row  %d\n", i);
        sleep(1);
#endif /* ifdef DEBUG */

        strcat(snake_ladder_info[i], "\n");
    }
}

int init(int argc, char* argv[]){
    /* 
    * Initialize Board Process
    **/

    if(argc != 3){
        printf("USAGE\n\t./board <number> <number>");
        exit(0);
    }

    n = atoi(argv[1]);
    fd_write = atoi(argv[2]);

    initShm();

    dprintf(fd_write, "%d ", getpid());
#ifdef  DEBUG
    dprintf(fd_write, "%p ", shm_board);
    dprintf(fd_write, "%p ", shm_players);
    printf("Board: Sent my pid %d\n", getpid());
#endif /* ifdef DEBUG */
    sleep(1);

    initSnakeLadderInfo();
    printBoard();

    sendAck();

    return 0;
}

void handleSigUsr1(int sig){
    printBoard();
    sendAck();
}

void handleSigUsr2(int sig){
    quit();
}

int main(int argc, char *argv[]){

    signal(SIGUSR1, handleSigUsr1);
    signal(SIGUSR2, handleSigUsr2);

    init(argc, argv);

    while(1)
        pause();

    return 0;
}

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/shm.h>
#include <wait.h>

const char* KEY = "./ludo.c";
const int PROJ_ID1 = 0;
const int PROJ_ID2 = 1;

const int BUFF_SIZE = 1024;

int fd_write;
int n;

int shm_board_id = -1;
int shm_players_id = -1;

int* shm_board;
int* shm_players;

int child_idx_flag = -1;

int pid_bp;
int* pid_players;

int current_player_idx = 0;

void quit(){
    shmdt(shm_board);
    shmdt(shm_players);

    exit(0);
}

void playerLoop(){
    /* 
    * Player processes enter this loop
    **/

#ifdef DEBUG
    printf("Player %c enters game loop\n", 'A' + child_idx_flag);
#endif /* ifdef DEBUG */
    while(1)
        pause();
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

int init(int argc, char* argv[]){
    /* 
    * Initialize Player-Parent Process
    **/

    if(argc != 4){
        printf("USAGE\n\t./players <number> <number> <number>\n");
        quit();
    }

    n = atoi(argv[1]);
    fd_write = atoi(argv[2]);
    pid_bp = atoi(argv[3]);

    initShm();

    dprintf(fd_write, "%d ", getpid());

    pid_players = (int*)malloc(sizeof(int) * n);

    for(int i = 0; i < n; i++){
        child_idx_flag = i;
        int pid = fork();    
        if(pid == 0){
            srand(getpid());
            playerLoop();
        }
        else pid_players[i] = pid;
    }
    child_idx_flag = -1;
#ifdef DEBUG
    printf("Player Parent initialization complete\n");
#endif /* ifdef DEBUG */
    
    return 0;
}

void parentHandleMove(){
    /* 
    * Move handler for Player-Parent process
    **/


#ifdef DEBUG
    printf("Player parent enters handle move\n");
#endif /* ifdef DEBUG */

    //First set the previous pid_player[i] to -1 if it reached to cell 100
    int prev_player_idx = (current_player_idx - 1 + n) % n;

#ifdef DEBUG
    printf("prev_player_idx: %d\n", prev_player_idx);
    sleep(1);
#endif /* ifdef DEBUG */

    if(shm_players[prev_player_idx] == 100) pid_players[prev_player_idx] = -1;

    //Find the next player that will play round-robin fashion
    for(int i = 0; i < n; i++){

#ifdef DEBUG
        printf("current_player_idx: %d\n", current_player_idx);
        sleep(1);
#endif /* ifdef DEBUG */

        // All players have reached the end
        if(pid_players[current_player_idx] == -1){
            current_player_idx = (current_player_idx + 1) % n;
        }
        else break;
    }

    if(pid_players[current_player_idx] == -1) return;

#ifdef DEBUG
    printf("Player parent chooses Player %c\n", 'A' + current_player_idx);
#endif /* ifdef DEBUG */

    kill(pid_players[current_player_idx], SIGUSR1);

    current_player_idx = (current_player_idx + 1) % n;

}

int checkOccupied(int pos){
    /* 
    * Checks if a cell is occupied by a player
    **/

    int occupied_flag = -1;
    for(int i = 0; i < n; i++){
        if(pos == shm_players[i]){
            occupied_flag = i;
            break;
        }
    }

    return occupied_flag;
}

void playerHandleMove(){
    /* 
    * Move handler for Player process
    **/

    // Move Calculation
    int moves[3] = {0};
    int total_moves = 0;
    int cnt_6 = 0;
    while(1){
        int move = rand() % 6 + 1;
        if(move == 6){
            cnt_6 = (cnt_6 + 1) % 3;
            if(cnt_6 == 0) {
                total_moves = 0;
                moves[0] = 0;
                moves[1] = 0;
                moves[2] = 0;
            }
            else {
                moves[(cnt_6 - 1 + 3) % 3] += move;
                total_moves += move;
            }
        }else{
            moves[cnt_6] += move;
            total_moves += move;
            break;
        }
    }

    char* border = "****************************************\n";
    printf("%s", border); 
    char die_roll[BUFF_SIZE];
    sprintf(die_roll, "%d", moves[0]);
    for(int i = 1; i < cnt_6 + 1; i++){
        strcat(die_roll, " + ");
        char num[10];
        sprintf(num, "%d", moves[i]);
        strcat(die_roll, num);
    }
    printf("Player %c: %s\n", 'A' + child_idx_flag, die_roll);

    // Move traversal
    if(shm_players[child_idx_flag] + total_moves < 100){
        int occupied = checkOccupied(shm_players[child_idx_flag] + total_moves);
        if(occupied != -1){
            printf("Move not permitted (cell already occupied by %c)\n", 'A' + occupied);
        }else{
            shm_players[child_idx_flag] += total_moves;
            printf("Player %c moves to cell %d\n", 'A' + child_idx_flag, shm_players[child_idx_flag]);
            int cell_val = shm_board[shm_players[child_idx_flag]];
            while(cell_val != 0){
                int to_be_pos = shm_players[child_idx_flag] + cell_val;
                if(cell_val > 0){
                    printf("Ladder at cell %d. Jump to %d\n", shm_players[child_idx_flag], to_be_pos);
                    occupied = checkOccupied(to_be_pos);
                }else if(cell_val < 0){
                    printf("Snake at cell %d. Jump to %d\n", shm_players[child_idx_flag], to_be_pos);
                    occupied = checkOccupied(to_be_pos);
                }
                if(occupied != -1){
                    printf("Move not permitted (cell already occupied by %c)\n", 'A' + occupied);
                    break;
                }else{
                    shm_players[child_idx_flag] += shm_board[shm_players[child_idx_flag]];
                    printf("Player %c moves to cell %d\n", 'A' + child_idx_flag, shm_players[child_idx_flag]);
                    cell_val = shm_board[shm_players[child_idx_flag]];
                }
            }
        }


    }else if(shm_players[child_idx_flag] + total_moves == 100){

        shm_players[child_idx_flag] += total_moves;
        printf("Player %c moves to cell %d\n", 'A' + child_idx_flag, shm_players[child_idx_flag]);
        shm_players[n]--;
        printf("Player %c exits with rank %d\n", 'A' + child_idx_flag, n - shm_players[n]);
        kill(pid_bp, SIGUSR1);
        quit();

    }else if(shm_players[child_idx_flag] + total_moves > 100){
        printf("Move not permitted (cannot go beyont 100)\n");
    }

    kill(pid_bp, SIGUSR1);

}

void handleSigUsr1(int sig){
    if(child_idx_flag == -1) // Parent handles signal
        parentHandleMove();
    else    // Else child handles signal
        playerHandleMove();
}

void terminatePlayers(){
    /* 
    * Process termination for still running player processes
    **/

    for(int i = 0; i < n; i++){
        if(pid_players[i] != -1){
            kill(pid_players[i], SIGKILL);
            waitpid(pid_players[i], NULL, 0);
            pid_players[i] = -1;
        }
    }
}

void handleSigUsr2(int sig){
    if(child_idx_flag == -1){
        terminatePlayers();
        quit();
    }
}

int main(int argc, char *argv[]){

    signal(SIGUSR1, handleSigUsr1);
    signal(SIGUSR2, handleSigUsr2);

    init(argc, argv);

    while(1) {
#ifdef DEBUG
        printf("Player Parent ready to handle input from CP\n");
#endif /* ifdef DEBUG */
        pause();
    }

    return 1;
}

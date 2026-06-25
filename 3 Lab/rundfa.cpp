//////////////////////////////////
// Name: Kshetrimayum Abo
// Roll Number: 23CS30029
//////////////////////////////////

#include <csignal>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>

int n;
int s;
char* file_name = strdup("dfa.txt");
char* state_type_arr;
int** state_transition_table;
int pipe_c[2];
int** pipe_s;
pid_t* state_pid;
int child_flag = -1;

int* state_transitions;
char state_type;

const int BUFF_SIZE = 1024;
const int SAVED_STDOUT = dup(STDOUT_FILENO);
const int SAVED_STDIN = dup(STDIN_FILENO);

const char TRANSITION = '>';
const char EOI = '$';
const char INVALID = '#';
const char OK = '^';
const char ACCEPT = '+';
const char REJECT = '-';
const char QUIT = '!';


void quit(){
    free(state_type_arr);

    for(int i = 0; i < n; i++)
        free(state_transition_table[i]);
    free(state_transition_table);

    for(int i = 0; i < n; i++)
        free(pipe_s[i]);
    free(pipe_s);

    for(int i = 0; i < n; i++){
        if(state_pid[i] != -1){
            int status;
            kill(state_pid[i], SIGKILL);
            waitpid(state_pid[i], &status, 0);
            state_pid[i] = -1;
        }
    }

    free(state_pid);

    free(state_transitions);

    free(file_name);
}

int init(int argc, char const *argv[]){
    if(argc == 2){
        free(file_name);
        file_name = strdup(argv[1]);
    }

    FILE *fptr = fopen(file_name, "r");
    if(fptr == NULL){
        printf("Error opening file: File %s doesn't exists\n", file_name);
        return -1;
    }

    char line[BUFF_SIZE];
    fgets(line, BUFF_SIZE, fptr);
    s = atoi(line);
    fgets(line, BUFF_SIZE, fptr);
    n = atoi(line);


    state_type_arr = (char*)malloc(n * sizeof(char));
    state_transition_table = (int**)malloc(n * sizeof(char*));

    while(fgets(line, BUFF_SIZE, fptr)){
        int state;
        char* token;
        token = strtok(line, " ");

        state = atoi(token);
        token = strtok(NULL, " ");
        state_type_arr[state] = *token;

        state_transition_table[state] = (int*)malloc(s * sizeof(char));

        int cnt = 0;
        token = strtok(NULL, " ");

        while (token != NULL){
            state_transition_table[state][cnt++] = atoi(token);
            token = strtok(NULL, " ");
        };
    }

    if(pipe(pipe_c) == -1)
        return -1;

    pipe_s = (int**)malloc(sizeof(int*) * n);
    for(int i = 0; i < n; i++){
        pipe_s[i] = (int*)malloc(sizeof(int) * 2);
        if(pipe(pipe_s[i]) == -1)
            return -1;
    }


    state_pid = (pid_t*)malloc(sizeof(pid_t) * n);
    memset(state_pid, -1, n * sizeof(pid_t));

    pid_t p;
    for (int i = 0; i < n; i++){
        p = fork();
        if(p == 0) {
            child_flag = i;
            break;
        }
        else if(p == -1)
            return -1;
        
        state_pid[i] = p;

#ifdef _VERBOSE
        char print_buffer[100];
        if(state_type_arr[i] == 'F'){
            sprintf(print_buffer, "+++ Final state %d created\n", i);
        }else if(state_type_arr[i] == 'N'){
            sprintf(print_buffer, "+++ Non-final state %d created\n", i);
        }
        dup2(SAVED_STDOUT, STDOUT_FILENO);
        printf("%100s", print_buffer);
        fflush(stdout);
#endif // _VERBOSE
    }

    if(child_flag == -1) {
        sleep(1);
        char print_buffer[100];
        sprintf(print_buffer, "+++ Coordinator: %d state processes are created\n", n);
        dup2(SAVED_STDOUT, STDOUT_FILENO);
        printf("%100s", print_buffer);
        fflush(stdout);

#ifdef _VERBOSE
        sprintf(print_buffer, "+++ Coordinator: Going to user loop\n", n);
        dup2(SAVED_STDOUT, STDOUT_FILENO);
        printf("%100s", print_buffer);
        fflush(stdout);
#endif // _VERBOSE
    }

    return 0;
}

void initCommunication(){
    if(child_flag == -1){
        for(int i = 0; i < n; i++){
            dup2(pipe_s[i][1], STDOUT_FILENO);

            printf(" %c", state_type_arr[i]);
            fflush(stdout);

            for(int j = 0; j < s; j++){
                printf("%d ", state_transition_table[i][j]);
                fflush(stdout);
            }

        }
    }
    else{
        state_transitions = (int*)malloc(sizeof(int) * s);

        dup2(pipe_s[child_flag][0], STDIN_FILENO);
        scanf(" %c", &state_type);
        // printf("State %d type: %c\n", child_flag, state_type);
        // fflush(stdout);

        for (int j = 0; j < s; j++){
            scanf("%d", &state_transitions[j]);
            // printf("State %d:%d: %d\n", child_flag, j, state_transitions[j]);
            // fflush(stdout);
        }
    }
}

void handleInterrupt(int sig){

    char print_buffer[100];
    sprintf(print_buffer, "+++ Coordinator going to terminate all state processes\n");
    dup2(SAVED_STDOUT, STDOUT_FILENO);
    printf("\n");
    printf("%100s", print_buffer);
    fflush(stdout);

    for(int i = 0; i < n; i++){
#ifdef _VERBOSE
    sprintf(print_buffer, "+++ State %d going to quit\n", i);
    dup2(SAVED_STDOUT, STDOUT_FILENO);
    printf("%100s", print_buffer);
    fflush(stdout);
#endif // _VERBOSE
        dup2(pipe_s[i][1], STDOUT_FILENO);
        printf("%c", QUIT);
        fflush(stdout);
        int status;
        waitpid(state_pid[i], &status, 0);
        state_pid[i] = -1;
    }

#ifdef _VERBOSE
    sprintf(print_buffer, "+++ Coordinator: Bye\n");
    dup2(SAVED_STDOUT, STDOUT_FILENO);
    printf("%100s", print_buffer);
    fflush(stdout);
#endif // _VERBOSE
#ifndef _VERBOSE
    sprintf(print_buffer, "+++ Coordinator: All state processes terminated. Bye.\n");
    dup2(SAVED_STDOUT, STDOUT_FILENO);
    printf("%100s", print_buffer);
    fflush(stdout);
#endif // !_VERBOSE

    quit();
    exit(0);
}

void userLoop(){
    signal(SIGINT, handleInterrupt);
    while(1){
        char input_str[BUFF_SIZE];
        int str_len;

        sleep(1);
        dup2(SAVED_STDOUT, STDOUT_FILENO);
        printf("Enter input string: ");
        fflush(stdout);
        dup2(SAVED_STDIN, STDIN_FILENO);
        scanf("%s", input_str);
        str_len = strlen(input_str);

        // dup2(SAVED_STDOUT, STDOUT_FILENO);
        // printf("String: %s\nLength: %d\n", input_str, str_len);
        // fflush(stdout);

        int cur_state = 0;
        int cur_idx = 0;

        dup2(pipe_s[cur_state][1], STDOUT_FILENO);
        printf("%c", TRANSITION);
        fflush(stdout);

        while(cur_idx <= str_len){
            // dup2(SAVED_STDOUT, STDOUT_FILENO);
            // printf("C: At State %d\n", cur_state);
            // fflush(stdout);

            // Take the state from where we are going to send our character
            dup2(pipe_c[0], STDIN_FILENO);
            scanf("%d", &cur_state);

            // dup2(SAVED_STDOUT, STDOUT_FILENO);
            // printf("C: Received state %d\n", cur_state);
            // fflush(stdout);


            if(cur_idx == str_len){

                // dup2(SAVED_STDOUT, STDOUT_FILENO);
                // printf("C: Reached End of Input. Sending EOI\n");
                // fflush(stdout);

                //Send the End of Input (EOI) character as we are at the end of input now
                dup2(pipe_s[cur_state][1], STDOUT_FILENO);
                printf("%c ", EOI);
                fflush(stdout);

                // dup2(SAVED_STDOUT, STDOUT_FILENO);
                // printf("C: Sent character: %c to State %d\n", EOI, cur_state);
                // fflush(stdout);
                break;
            }

            // If not End of Input, send the current character in the input that we are reading
            dup2(pipe_s[cur_state][1], STDOUT_FILENO);
            printf(" %c", input_str[cur_idx]);
            fflush(stdout);

            // dup2(SAVED_STDOUT, STDOUT_FILENO);
            // printf("C: Sent character: %c to State %d\n", input_str[cur_idx], cur_state);
            // fflush(stdout);

            // Break if invalid input
            if(input_str[cur_idx] - 'a' < 0 || input_str[cur_idx] - 'a' >= s) break;

            cur_idx++;
        }

        // dup2(SAVED_STDOUT, STDOUT_FILENO);
        // printf("C: Due to invalid input, starting again\n");
        // fflush(stdout);
    }
}

void stateLoop(){
    // printf("State %d loop entered\n", child_flag);
    signal(SIGINT, SIG_IGN);
    while(1){

        // Transition / Quit / EOI signal
        dup2(pipe_s[child_flag][0], STDIN_FILENO);
        char input;
        scanf(" %c", &input);


        if(input == QUIT){
            // printf("Terminating state: %d\n", child_flag);
            // fflush(stdout);
            quit();
            exit(0);
        }else if(input == TRANSITION){

            // Send state to Coordinator
            dup2(pipe_c[1], STDOUT_FILENO);
            printf("%d ", child_flag);
            fflush(stdout);

            // dup2(SAVED_STDOUT, STDOUT_FILENO);
            // printf("S%d: Sent state %d to C\n", child_flag, child_flag);
            // fflush(stdout);

            // Take character from Coordinator
            char transition_response;
            dup2(pipe_s[child_flag][0], STDIN_FILENO);
            scanf(" %c", &transition_response);

            // dup2(SAVED_STDOUT, STDOUT_FILENO);
            // printf("S%d: Received character %c from C\n", child_flag, transition_response);
            // fflush(stdout);

            if(transition_response == QUIT){
                // dup2(SAVED_STDOUT, STDOUT_FILENO);
                // printf("Terminating state: %d\n", child_flag);
                // fflush(stdout);
                quit();
                exit(0);
            }
            else if(transition_response == EOI){
                if(state_type == 'F'){
                    dup2(SAVED_STDOUT, STDOUT_FILENO);
                    printf(" %d ACCEPT\n", child_flag);
                    fflush(stdout);
                }else{
                    dup2(SAVED_STDOUT, STDOUT_FILENO);
                    printf(" %d REJECT\n", child_flag);
                    fflush(stdout);
                }
            }
            else if(transition_response - 'a' >= s || transition_response - 'a' < 0){ // Handle invalid input
                dup2(SAVED_STDOUT, STDOUT_FILENO);
                printf("INVALID INPUT SYMBOL: %c\n", transition_response);
                fflush(stdout);
                dup2(pipe_c[1], STDOUT_FILENO);
                printf("%c", INVALID);
                fflush(stdout);
            }else{ //Handle valid input
                int next_state = state_transitions[transition_response - 'a'];

                dup2(SAVED_STDOUT, STDOUT_FILENO);
                printf("%3d -- %c --> ", child_flag, transition_response);
                fflush(stdout);


                // Send TRANSITION character to the state that we will be going to next
                dup2(pipe_s[next_state][1], STDOUT_FILENO);
                printf("%c", TRANSITION);
                fflush(stdout);
            }
        }
    }
}


void printDfa(){
    for(int i = 0; i < n; i++){
        dup2(SAVED_STDOUT, STDOUT_FILENO);
        printf("%d %c ", i, state_type_arr[i]);
        for(int j = 0; j < s; j++)
            printf("%d ", state_transition_table[i][j]);
        printf("\n");
    }
}


int main(int argc, char const *argv[]){

    if(init(argc, argv) == -1){
        perror("Error in Initialization\n");
        quit();
        return 1;
    };
    initCommunication();

    if(child_flag == -1){
        userLoop();
    }else{
        stateLoop();
    }

    quit();
    return 0;
}

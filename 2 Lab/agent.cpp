#include <algorithm>
#include <bits/stdc++.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <numeric>
#include <ostream>
#include <random>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
using namespace std;

////////////////////////////
// Global State Variables
////////////////////////////
int t = 0;
int n = 0;
queue<int> q_id;
map<int, int> customer_id_pid;
int customer_id = 0;
int flag = 0;

////////////////////////////
// Handlers
////////////////////////////
void handleSigUsr1(int signal){
    
    ifstream require("require.txt");

    string s;
    getline(require, s);

    istringstream val(s);

    string text;

    val >> text;
    customer_id = stoi(text);

    val >> text;
    int r = stoi(text);

    if(r <= t){
        t -= r;
        kill(customer_id_pid[customer_id], SIGUSR1);
        sleep(1);
    }else {
        kill(customer_id_pid[customer_id], SIGUSR2);
        sleep(1);
    }

    q_id.push(customer_id);

}

void handleSigUsr2(int signal){
}

int main(int argc, char *argv[]){

    ////////////////////
    // Init
    ////////////////////
    if(argc != 3){
        printf("Inappropriate number of arguments\n");
        printf("arg1: number of tickets available\n");
        printf("arg2: number of customers\n");
        return 1;
    }

    signal(SIGUSR1, handleSigUsr1);
    signal(SIGUSR2, handleSigUsr2);

    mt19937 rng(getpid());

    t = atoi(argv[1]);
    n = atoi(argv[2]);


    vector<int> ids(n);
    iota(ids.begin(), ids.end(), 1);

    int parent_pid = getpid();

    for(int x : ids){
        int pid = fork();
        if(pid > 0){        // Parent process
            customer_id_pid[x] = pid;
        }else if(pid == 0){ // Child process
            char* argv[3];
            char num_buf[32];
            sprintf(num_buf, "%d", x);
            argv[0] = (char*)"./customer";
            argv[1] = num_buf;
            argv[2] = NULL;
            execv("./customer", argv);
        }
    }
    
    sleep(2);

    if(parent_pid == getpid()) shuffle(ids.begin(), ids.end(), rng);
    for(int x : ids) q_id.push(x);

    ///////////////////////////
    // Main Loop
    ///////////////////////////


    while(!q_id.empty() && t != 0){

        queue<int> print_q;
        vector<int> print_vec;
        while(!q_id.empty()){
            int id = q_id.front();    
            print_q.push(id);
            print_vec.push_back(id);
            q_id.pop();
        }
        for(int& x : print_vec) q_id.push(x);

        cout << "Agent: Queue = ( ";
        while(!print_q.empty()){
            cout << print_q.front() << " ";
            print_q.pop();
        }
        cout << ") Available = " << t << endl;
        
        kill(customer_id_pid[q_id.front()], SIGUSR1);
        q_id.pop();

        pause();
    }
    if(t == 0){
        cout << "Agent terminates ";
        while(!q_id.empty()){
            customer_id = q_id.front();
            q_id.pop();
            printf("%d ", customer_id);
        }
        cout << endl;
        cout << "Agent: Booking session over (no more tickets available)" << endl;
    }
    else if(q_id.empty()) cout << "Agent: Booking session over (no more customers available)" << endl;
    
    return 0;
}

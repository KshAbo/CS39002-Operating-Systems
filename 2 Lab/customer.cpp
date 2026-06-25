#include <bits/stdc++.h>
#include <csignal>
#include <fstream>
#include <random>
#include <unistd.h>
using namespace std;

////////////////////////////
// Global State Variables
////////////////////////////
std::mt19937 rng(getpid());
std::uniform_int_distribution<int> dist(1, 4);

int successful_req = 0;
int customer_id = 0;
int flag = 0;

////////////////////////////
// Handlers
////////////////////////////
void handleSigUsr1(int signal){
    if(flag == 0){ // Handle first part request

        int r = 0;
        if(successful_req == 0){
            r = dist(rng);
        }else if(successful_req >= 1){
            std::bernoulli_distribution p(0.5);
            if(!(successful_req == 1 && p(rng))){
                kill(getppid(), SIGUSR2);
                printf("\t\t\tCustomer %5d leaves the booking system\n", customer_id);
                exit(0);
            }
        }
        ofstream require("require.txt");
        if(require.is_open()){
            require << customer_id << " " << r << endl;
            printf("\t\t\tCustomer %5d: Request for %d tickets\n", customer_id, r);
        }else{
            cerr << "require.txt can't be opened" << endl;
            exit(1);
        }
        require.close();
        kill(getppid(), SIGUSR1);
        flag = 1;
    }else if(flag == 1){ // Handle second part of request
        successful_req++;
        flag = 0;
        printf("\t\t\tCustomer %5d: Booking %d successful\n", customer_id, successful_req);
    }
}

void handleSigUsr2(int signal){
    flag = 0;
    printf("\t\t\tCustomer %5d: Booking %d failed\n", customer_id, successful_req + 1);
}


int main (int argc, char *argv[]) {
    if(argc != 2){
        printf("Inappropriate number of arguments\n");
        printf("arg1: customer ID\n");
        exit(1);
    }

    customer_id = atoi(argv[1]);
    printf("\t\t\tCustomer %5d joins the booking system\n", customer_id);

    signal(SIGUSR1, handleSigUsr1);
    signal(SIGUSR2, handleSigUsr2);

    while(true)
        pause();

    return 0;
}

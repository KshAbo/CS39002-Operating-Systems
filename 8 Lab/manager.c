#include "global.h"

// Initialize Global Variables
int M = 8, N = 16, R = 32;
int *TOTAL;
int *AVAILABLE;
int **ALLOCATION;
int **REQUEST;
int **RELEASE;
int REQFROM;
int REQTYPE;
int NACTIVE;
int *STATUS;
int *RQ;
int rq_size = 0;

// Synchronization Objects
pthread_mutex_t RMTX = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t SMTX = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t SCND = PTHREAD_COND_INITIALIZER;
pthread_mutex_t AMTX = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ACND = PTHREAD_COND_INITIALIZER;
pthread_mutex_t *WMTX;
pthread_cond_t *WCND;


#include "worker.c"
#include "dlhandler.c"

void printVector(int* vec, int size) {
    printf("[");
    for(int i = 0; i < size; i++) {
        printf("%2d%s", vec[i], (i == size - 1) ? "" : ", ");
    }
    printf("]\n");
}

void printRq() {
    printf("Workers waiting: (");
    for(int i = 0; i < rq_size; i++) {
        printf("%d%s", RQ[i], (i == rq_size - 1) ? "" : ", ");
    }
    printf(")\n");
}

int canGrant(int worker_id) {
    for(int j = 0; j < M; j++) {
        if(REQUEST[worker_id][j] > AVAILABLE[j]) return 0;
    }
    return 1;
}

void grantRequest(int worker_id) {
    for(int j = 0; j < M; j++) {
        AVAILABLE[j] -= REQUEST[worker_id][j];
        ALLOCATION[worker_id][j] += REQUEST[worker_id][j];
        REQUEST[worker_id][j] = 0;
    }
}

void serveRq() {
    int i = 0;
    while(i < rq_size) {
        int w = RQ[i];
        if(canGrant(w)) {
            grantRequest(w);
            printf("Worker %d granted pending request\n", w);
            printf("AVAILABLE = "); printVector(AVAILABLE, M);
            
            for(int j = i; j < rq_size - 1; j++) RQ[j] = RQ[j+1];
            rq_size--;
            
            pthread_mutex_lock(&WMTX[w]);
            pthread_cond_signal(&WCND[w]);
            pthread_mutex_unlock(&WMTX[w]);
        } else {
            i++;
        }
    }
}

int main(int argc, char *argv[]) {
    if(argc >= 4) {
        M = atoi(argv[1]);
        N = atoi(argv[2]);
        R = atoi(argv[3]);
    }
    
    srand(time(NULL));
    
    // Allocate Memory
    TOTAL = (int*)malloc(M * sizeof(int));
    AVAILABLE = (int*)malloc(M * sizeof(int));
    ALLOCATION = (int**)malloc(N * sizeof(int*));
    REQUEST = (int**)malloc(N * sizeof(int*));
    RELEASE = (int**)malloc(N * sizeof(int*));
    STATUS = (int*)malloc(N * sizeof(int));
    RQ = (int*)malloc(N * sizeof(int));
    WMTX = (pthread_mutex_t*)malloc(N * sizeof(pthread_mutex_t));
    WCND = (pthread_cond_t*)malloc(N * sizeof(pthread_cond_t));
    
    printf("TOTAL = ");
    for(int i = 0; i < M; i++) {
        TOTAL[i] = 8 + rand() % 25; // Random range [8, 32]
        AVAILABLE[i] = TOTAL[i];
    }
    printVector(TOTAL, M);
    
    for(int i = 0; i < N; i++) {
        ALLOCATION[i] = (int*)calloc(M, sizeof(int));
        REQUEST[i] = (int*)calloc(M, sizeof(int));
        RELEASE[i] = (int*)calloc(M, sizeof(int));
        STATUS[i] = STAT_ACTIVE;
        pthread_mutex_init(&WMTX[i], NULL);
        pthread_cond_init(&WCND[i], NULL);
    }
    
    NACTIVE = N;
    REQTYPE = REQ_NONE;
    
    pthread_t *workers = (pthread_t*)malloc(N * sizeof(pthread_t));
    for(long i=0; i<N; i++) {
        pthread_create(&workers[i], NULL, workerThread, (void*)i);
    }
    
    pthread_t dl_thread;
    pthread_create(&dl_thread, NULL, dlhandlerThread, NULL);
    
    pthread_mutex_lock(&SMTX);
    while(1) {
        pthread_cond_wait(&SCND, &SMTX);
        
        if (REQTYPE == REQ_QUIT) {
            break;
        }
        else if (REQTYPE == REQ_RELEASE) {
            printf("Worker %d makes release request\n", REQFROM);
            for(int j = 0; j < M; j++) {
                AVAILABLE[j] += RELEASE[REQFROM][j];
                ALLOCATION[REQFROM][j] -= RELEASE[REQFROM][j];
                RELEASE[REQFROM][j] = 0;
            }
            printf("AVAILABLE = "); printVector(AVAILABLE, M);
            
            pthread_mutex_lock(&AMTX);
            pthread_cond_signal(&ACND);
            pthread_mutex_unlock(&AMTX);
            
            printRq();
            serveRq();
        }
        else if (REQTYPE == REQ_ALLOCATE) {
            printf("Worker %d makes allocation request\n", REQFROM);
            if (canGrant(REQFROM)) {
                grantRequest(REQFROM);
                printf("Worker %d granted request\n", REQFROM);
                printf("AVAILABLE = "); printVector(AVAILABLE, M);
                
                pthread_mutex_lock(&AMTX);
                pthread_cond_signal(&ACND);
                pthread_mutex_unlock(&AMTX);
                
                pthread_mutex_lock(&WMTX[REQFROM]);
                pthread_cond_signal(&WCND[REQFROM]);
                pthread_mutex_unlock(&WMTX[REQFROM]);
            } else {
                printf("Worker %d has to wait\n", REQFROM);
                RQ[rq_size++] = REQFROM;
                printRq();
                
                pthread_mutex_lock(&AMTX);
                pthread_cond_signal(&ACND);
                pthread_mutex_unlock(&AMTX);
            }
        }
        REQTYPE = REQ_NONE;
    }
    pthread_mutex_unlock(&SMTX);
    
    printf("All workers left\n");
    
    for(int i = 0; i < N; i++) {
        pthread_join(workers[i], NULL);
    }
    pthread_join(dl_thread, NULL);
    
    return 0;
}

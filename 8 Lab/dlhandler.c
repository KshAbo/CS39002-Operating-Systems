#include "global.h"

void* dlhandlerThread(void* arg) {
    int *Work = (int*)malloc(M * sizeof(int));
    int *Finish = (int*)malloc(N * sizeof(int));
    
    while(1) {
        sleep(1);
        
        pthread_mutex_lock(&RMTX);
        
        if (NACTIVE == 0) {
            printf("dlhandler finds NACTIVE = (0)\n");
            REQTYPE = REQ_QUIT;
            pthread_mutex_lock(&SMTX);
            pthread_cond_signal(&SCND);
            pthread_mutex_unlock(&SMTX);
            pthread_mutex_unlock(&RMTX);
            break;
        }
        
        while(1) {
            printf("Deadlock detection in progress\n");
            
            for(int j = 0; j < M; j++) Work[j] = AVAILABLE[j];
            for(int i = 0; i < N; i++) Finish[i] = (STATUS[i] == STAT_EXITED) ? 1 : 0;
            
            int finish_seq[N];
            int f_count = 0;
            
            int possible;
            do {
                possible = 0;
                for(int i = 0; i < N; i++) {
                    if (!Finish[i]) {
                        int can_finish = 1;
                        for(int j = 0; j < M; j++) {
                            if (REQUEST[i][j] > Work[j]) { can_finish = 0; break; }
                        }
                        if (can_finish) {
                            for(int j = 0; j < M; j++) Work[j] += ALLOCATION[i][j];
                            Finish[i] = 1;
                            finish_seq[f_count++] = i;
                            possible = 1;
                        }
                    }
                }
            } while(possible);
            
            int deadlock = 0;
            for(int i = 0; i < N; i++) {
                if (!Finish[i]) { deadlock = 1; break; }
            }
            
            printf("Finish sequence:");
            for(int k = 0; k < f_count; k++) {
                printf(" %d", finish_seq[k]);
            }
            printf("\n");
            
            if (!deadlock) {
                printf("No deadlock detected\n");
                break;
            }
            
            printf("Deadlock detected\n");
            printf("Allocation status:");
            
            int max_alloc = -1;
            int victim = -1;
            
            for(int i = 0; i < N; i++) {
                if (!Finish[i]) {
                    int sum = 0;
                    for(int j = 0; j < M; j++) sum += ALLOCATION[i][j];
                    printf(" %d:%d", i, sum);
                    
                    if (sum > max_alloc) {
                        max_alloc = sum;
                        victim = i;
                    }
                }
            }
            printf("\n");
            
            printf("Preempting resources from worker %2d with %2d resources\n", victim, max_alloc);
            
            // Release victim's resources
            for(int j = 0; j < M; j++) {
                AVAILABLE[j] += ALLOCATION[victim][j];
                ALLOCATION[victim][j] = 0;
            }
            
            int q = 0;
            while(q < rq_size) {
                int w = RQ[q];
                int can_grant_req = 1;
                for(int j = 0; j < M; j++) {
                    if (REQUEST[w][j] > AVAILABLE[j]) { can_grant_req = 0; break; }
                }
                
                if (can_grant_req) {
                    // Print pending request vector before granting
                    printf("Worker %2d granted pending request\t[", w);
                    for(int j = 0; j < M; j++) {
                        printf("%2d%s", REQUEST[w][j], (j == M - 1) ? " ]\n" : ", ");
                    }
                    
                    // Grant the resources
                    for(int j = 0; j < M; j++) {
                        AVAILABLE[j] -= REQUEST[w][j];
                        ALLOCATION[w][j] += REQUEST[w][j];
                        REQUEST[w][j] = 0;
                    }
                    
                    // Print updated AVAILABLE vector
                    printf("\tAVAILABLE = [");
                    for(int j = 0; j < M; j++) {
                        printf("%2d%s", AVAILABLE[j], (j == M - 1) ? " ]\n" : ", ");
                    }
                    
                    for(int j = q; j < rq_size - 1; j++) RQ[j] = RQ[j+1];
                    rq_size--;
                    
                    pthread_mutex_lock(&WMTX[w]);
                    pthread_cond_signal(&WCND[w]);
                    pthread_mutex_unlock(&WMTX[w]);
                } else {
                    q++;
                }
            }
        }
        pthread_mutex_unlock(&RMTX);
    }
    
    free(Work);
    free(Finish);
    return NULL;
}

#include "global.h"

int genallocationreq ( int TOT[], int ALLOC[], int REQ[], int m )
{
   int j, allowed, status = 0;

   for (j=0; j<m; ++j) {
      allowed = TOT[j] - ALLOC[j];
      if (allowed > 3) allowed = 3;
      if ( (allowed > 0) && (rand() % 2) ) {
         REQ[j] = rand() % (allowed + 1);
         if (REQ[j]) status = 1;
      } else {
         REQ[j] = 0;
      }
   }
   return status;
}

int genreleasereq ( int ALLOC[], int REL[], int m )
{
   int j, status = 0;

   for (j=0; j<m; ++j) {
      if ( (ALLOC[j] > 0) && (rand() % 2) ) {
         REL[j] = rand() % (1 + ALLOC[j]);
         if (REL[j]) status = 1;
      } else {
         REL[j] = 0;
      }
   }
   return status;
}

void* workerThread(void* arg) {
    long id = (long)arg;
    
    for(int r = 0; r < R; r++) {
        usleep((200 + rand() % 301) * 1000); // Sleep for 200-500ms (2-5 req/sec)
        
        int type, valid = 0;
        do {
            type = (r == 0) ? REQ_ALLOCATE : ((rand() % 100 < 50) ? REQ_ALLOCATE : REQ_RELEASE);
            if (type == REQ_ALLOCATE) {
                // Pass the specific row for this worker's allocation and request arrays
                valid = genallocationreq(TOTAL, ALLOCATION[id], REQUEST[id], M);
                if (!valid && r > 0) type = REQ_RELEASE; // Fallback to release if allocation impossible
            }
            if (type == REQ_RELEASE && r > 0) {
                // Pass the specific row for this worker's allocation and release arrays
                valid = genreleasereq(ALLOCATION[id], RELEASE[id], M);
            }
        } while (!valid); // Ensure non-zero vector
        
        // Lock RMTX to sequentialize manager requests
        pthread_mutex_lock(&RMTX);
        REQFROM = id;
        REQTYPE = type;
        
        pthread_mutex_lock(&AMTX);
        if (type == REQ_ALLOCATE) {
            pthread_mutex_lock(&WMTX[id]);
        }
        
        pthread_mutex_lock(&SMTX);
        pthread_cond_signal(&SCND);
        pthread_mutex_unlock(&SMTX);
        
        // Wait for acknowledgment from manager
        pthread_cond_wait(&ACND, &AMTX);
        
        pthread_mutex_unlock(&RMTX);
        pthread_mutex_unlock(&AMTX);
        
        // Wait for allocation to be granted if necessary
        if (type == REQ_ALLOCATE) {
            pthread_cond_wait(&WCND[id], &WMTX[id]);
            pthread_mutex_unlock(&WMTX[id]);
        }
    }
    
    // (R+1)-th Request: Release all held resources
    int has_held = 0;
    for(int j = 0; j < M; j++) {
        RELEASE[id][j] = ALLOCATION[id][j];
        if (RELEASE[id][j] > 0) has_held = 1;
    }
    
    if (has_held) {
        printf("Worker %ld going to quit\nReleasing allocation\n", id);
        pthread_mutex_lock(&RMTX);
        REQFROM = id;
        REQTYPE = REQ_RELEASE;
        
        pthread_mutex_lock(&AMTX);
        pthread_mutex_lock(&SMTX);
        pthread_cond_signal(&SCND);
        pthread_mutex_unlock(&SMTX);
        
        pthread_cond_wait(&ACND, &AMTX);
        
        pthread_mutex_unlock(&RMTX);
        pthread_mutex_unlock(&AMTX);
    }
    
    pthread_mutex_lock(&RMTX);
    STATUS[id] = STAT_EXITED;
    NACTIVE--;
    pthread_mutex_unlock(&RMTX);
    
    return NULL;
}

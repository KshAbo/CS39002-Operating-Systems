#include "global.c"
#include <pthread.h>
#include <stdio.h>

// Worker ( w )
// {
//     While (there is/are more car(s) to produce) {
//         For each part p in the to-do list of w (in the sorted order, for that type of car), do {
//             Check if all the prerequisites for p are already completed.
//                 If not, wait until all the prerequisites of p are completed.
//                 Complete work on part p, and mark p as done.
//                 For each part q which depends on p (that is, for each edge p → q in the dependency graph), do {
//                     If not all prerequisites of q are done, do nothing.
//                         Otherwise, do {
//                             Let v be the worker to which part q is assigned.
//                                 If v is currently waiting for starting part q, wake v up.
//                         }
//                 }
//         }
//     }
// }

int checkPrerequisitesComplete(vector* prerequites){
    int end_idx = prerequites->end_idx;
    for(int i = 0; i < end_idx; i++){
        if(PSTAT[prerequites->vector[i]] == PART_PENDING) return 0;
    }
    return 1;
}


void worker(void* arg){
    int worker_id = *((int*)arg);
    

    while(1){

        pthread_mutex_lock(&mtx);
        WSTAT[worker_id] = WORKER_START;
        pthread_mutex_unlock(&mtx);


        pthread_barrier_wait(&bop);

        if(f + b <= 0){
            pthread_mutex_lock(&mtx);
            WSTAT[worker_id] = WORKER_DONE;
            pthread_mutex_unlock(&mtx);

            break;
        }


        int n_workers = (current_car == FOO) ? foo_info.n_workers : bar_info.n_workers;
        if(worker_id >= n_workers) {
            pthread_mutex_lock(&mtx);
            WSTAT[worker_id] = WORKER_DONE;
            pthread_mutex_unlock(&mtx);

            insertBuffer(worker_id, "All done");
            printBuffer();

            pthread_barrier_wait(&eop);
            continue;
        }

        pthread_mutex_lock(&mtx);
        WSTAT[worker_id] = WORKER_WORKING;
        vector* assignments = (current_car == FOO) ?        // Different worker parts depending on which car
                                &foo_info.workers_assignment[worker_id] : 
                                &bar_info.workers_assignment[worker_id];     
        pthread_mutex_unlock(&mtx);

        for(int i = 0; i < assignments->end_idx; i++){      // Going through each part of the worker

            pthread_mutex_lock(&mtx);

            int part_no = assignments->vector[i];
            vector* prereqs = (current_car == FOO) ? &foo_info.prerequistes_graph[part_no] : &bar_info.prerequistes_graph[part_no];

            // Going through all prerequisite parts for the current part 
            while (!checkPrerequisitesComplete(prereqs)) {
                WINFO[worker_id] = part_no; // Waiting for this specific part
                WSTAT[worker_id] = WORKER_WAITING;

                char buf[100];
                sprintf(buf, "Wait %d", part_no);
                insertBuffer(worker_id, buf);
                printBuffer();

                pthread_cond_wait(&cnd[worker_id], &mtx);
            }

            WSTAT[worker_id] = WORKER_WORKING;
            PSTAT[part_no] = PART_DONE;

            char buf[100];
            sprintf(buf, "Part %d", part_no);
            insertBuffer(worker_id, buf);

            // For each part that depends on the part that we just finished
            vector* depends = (current_car == FOO) ? &foo_info.dependencies_graph[part_no] : &bar_info.dependencies_graph[part_no];
            int depends_end_idx = depends->end_idx;
            for(int j = 0; j < depends_end_idx; j++){
                int depends_part = depends->vector[j];
                vector* prereqs = (current_car == FOO) ? &foo_info.prerequistes_graph[depends_part] : &bar_info.prerequistes_graph[depends_part];  
                int* part_worker_map = (current_car == FOO) ? foo_info.part_worker_map : bar_info.part_worker_map;
                if(checkPrerequisitesComplete(prereqs)){    // If all prerequisites are satisfied
                    int depends_worker_id = part_worker_map[depends_part];
                    if(WINFO[depends_worker_id] == depends_part && WSTAT[depends_worker_id] == WORKER_WAITING){
                        insertBuffer(depends_worker_id, "Wake up");
                        pthread_cond_signal(&cnd[depends_worker_id]);
                    }
                }else
                    continue;

            }
            printBuffer();

            pthread_mutex_unlock(&mtx);

        }

        pthread_mutex_lock(&mtx);
        WSTAT[worker_id] = WORKER_DONE;
        pthread_mutex_unlock(&mtx);

        insertBuffer(worker_id, "All done");
        printBuffer();

        pthread_barrier_wait(&eop);
    }
}

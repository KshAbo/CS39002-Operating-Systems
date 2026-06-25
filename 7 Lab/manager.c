#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "work.c"
#include "global.c"


void parseFile(const char* file_name, manufacture_info *car_type){

    FILE *file_ptr = fopen(file_name, "r");
    if(file_ptr == NULL){
        perror("File can't be opened");
        exit(1);
    }

    char line[BUFFSIZE];
    char* delim = " \n";
    fgets(line, BUFFSIZE, file_ptr);

    char* n_parts_c = strtok(line, delim);
    char* n_workers_c = strtok(NULL, delim);

    int n_workers = atoi(n_workers_c);
    int n_parts = atoi(n_parts_c);

    car_type->n_workers = n_workers;
    car_type->n_parts = n_parts;

    M = (M < n_workers) ? n_workers : M;

    for(int i = 0; i < n_parts; i++){

        fgets(line, BUFFSIZE, file_ptr);
        char* part_no_c = strtok(line, delim);
        int part_no = atoi(part_no_c);
        if(part_no != i){
            printf("Somethings wrong in part\n");
            exit(1);
        }

        char* worker_no_c = strtok(NULL, delim);
        int worker_no = atoi(worker_no_c);

        char* part_depend_c = strtok(NULL, delim);
        int part_depend = atoi(part_depend_c);

        vector* dependency_vec = &car_type->dependencies_graph[part_no];
        vector* worker_vec = &car_type->workers_assignment[worker_no];
        int* part_worker_map = car_type->part_worker_map;

        push_back(worker_vec, part_no);
        part_worker_map[part_no] = worker_no;

        while(part_depend != -1){

            push_back(dependency_vec, part_depend);

            vector* prereq_vec = &car_type->prerequistes_graph[part_depend];
            push_back(prereq_vec, part_no);

            part_depend_c = strtok(NULL, delim);
            part_depend = atoi(part_depend_c);

        }
    }

}

void parseDependency(){
    parseFile(FOO_FILE, &foo_info);
    parseFile(BAR_FILE, &bar_info);
}


void printInfo(manufacture_info* info){

    int n_parts = info->n_parts;
    int n_workers = info->n_workers;

    printf("\n");
    printTab(1);
    printf("Dependencies\n");
    for(int i = 0; i < n_parts; i++){
        int end_idx = info->dependencies_graph[i].end_idx;
        vector* vec = &info->dependencies_graph[i];
        printTab(1);
        printf("%d -> ", i);

        for(int j = 0; j < end_idx; j++){
            printf("%d ", vec->vector[j]);
        }
        printf("\n");
    }
    printf("\n");

    printTab(1);
    printf("Prerequisites\n");
    for(int i = 0; i < n_parts; i++){
        int end_idx = info->prerequistes_graph[i].end_idx;
        vector* vec = &info->prerequistes_graph[i];
        printTab(1);
        printf("%d <- ", i);

        for(int j = 0; j < end_idx; j++){
            printf("%d ", vec->vector[j]);
        }
        printf("\n");
    }
    printf("\n");

    printTab(1);
    printf("Worker assignment\n");
    for(int i = 0; i < n_workers; i++){
        int end_idx = info->workers_assignment[i].end_idx;
        vector* vec = &info->workers_assignment[i];
        printTab(1);
        printf("%d <- ", i);

        for(int j = 0; j < end_idx; j++){
            printf("%d ", vec->vector[j]);
        }
        printf("\n");
    }
    printf("\n");
}

void processCar(car type){

    pthread_mutex_lock(&mtx);
    current_car = type;
    pthread_mutex_unlock(&mtx);

    if(type == FOO){
        printf("+++ Production of a foocar begins\n");
    }else if(type == BAR){
        printf("+++ Production of a barcar begins\n");
    }

    printTab(1);
    for(int i = 0; i < M; i++){
        printf("WORKER %d ", i);
    }
    printf("\n");

    printTab(1);
    for(int i = 0; i < M; i++){
        printf("-------- ");
    }
    printf("\n");

    pthread_barrier_wait(&bop);

    pthread_barrier_wait(&eop);

}

void initStats(car type){
    int n_parts = 0;
    int n_workers = 0;

    pthread_mutex_lock(&mtx);
    if(type == FOO){
        n_parts = foo_info.n_parts;
        n_workers = foo_info.n_workers;
    }else if(type == BAR){
        n_parts = bar_info.n_parts;
        n_workers = bar_info.n_workers;
    }

    for(int i = 0; i < n_parts; i++){
        PSTAT[i] = PART_PENDING;
    }
    for(int i = 0; i < n_workers; i++){
        WSTAT[i] = WORKER_START;
    }
    for(int i = 0; i < n_workers; i++){
        WINFO[i] = -1;
    }
    pthread_mutex_unlock(&mtx);
}

void managerLoop(){
    while(f + b > 0){
        if(f > 0){
            initStats(FOO);
            processCar(FOO);
            pthread_mutex_lock(&mtx);
            f--;
            pthread_mutex_unlock(&mtx);
        }

        if(b > 0){
            initStats(BAR);
            processCar(BAR);
            pthread_mutex_lock(&mtx);
            b--;
            pthread_mutex_unlock(&mtx);
        }
    }

    printf("+++ All productions completed\n");
    pthread_barrier_wait(&bop);
}

int manager(int f_cars, int b_cars){

    f = f_cars;
    b = b_cars;

    parseDependency();
    printf("+++ Foocar\n");
    printInfo(&foo_info);
    printf("+++ Barcar\n");
    printInfo(&bar_info);

    pthread_mutex_init(&mtx, NULL);
    pthread_barrier_init(&bop, NULL, M + 1);
    pthread_barrier_init(&eop, NULL, M + 1);

    for(int i = 0; i < M; i++){
        worker_id[i] = i;
        if(pthread_create(&workers[i], NULL, (void*)worker, (void*)&worker_id[i]) != 0){
            perror("Thread could'nt be created");
        }
        if(pthread_cond_init(&cnd[i], NULL) != 0){
            perror("Condition variable couldn't be created");
        }
    }

    managerLoop();

    printTab(1);
    for(int i = 0; i < M; i++){
        printf("WORKER %d ", i);
    }
    printf("\n");

    printTab(1);
    for(int i = 0; i < M; i++){
        printf("-------- ");
    }
    printf("\n");

    for(int i = 0; i < M; i++){
        if(pthread_join(workers[i], NULL) != 0){
            perror("Thread couldn't join");
        }
        insertBuffer(i, "Quit");
        printBuffer();
        if(pthread_cond_destroy(&cnd[i]) != 0){
            perror("Conditional variable couldn't be destroyed");
        };
    }

    pthread_barrier_destroy(&eop);
    pthread_barrier_destroy(&bop);
    pthread_mutex_destroy(&mtx);

    return 0;
}

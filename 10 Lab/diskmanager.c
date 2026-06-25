#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "diskutils.c"

int vd_shm_id = -1;
unsigned char *vd_shm_ptr = NULL;

void createVirtualDisk(){
    vd_shm_id = shmget(ftok(KEY, KEY_ID), VD_SIZE, 0777 | IPC_CREAT | IPC_EXCL);
    if(vd_shm_id == -1){
        perror("shm");
        exit(1);
    }

    vd_shm_ptr = shmat(vd_shm_id, NULL, 0);
    if(vd_shm_ptr == NULL){
        perror("shm");
        exit(1);
    }

    unsigned int total_blocks = TOTAL_BLOCKS;
    unsigned int free_blocks = TOTAL_BLOCKS - 266; // Blocks 0 to 265 are reserved
    unsigned int root_dir_block = ROOT_BLOCK;

    // Initialize virtual disk to 0
    memset(vd_shm_ptr, 0, BLOCK_SIZE * TOTAL_BLOCKS);

    // Initialize Block 0
    memcpy(vd_shm_ptr, &total_blocks, sizeof(unsigned int));
    memcpy(vd_shm_ptr + sizeof(unsigned int), &free_blocks, sizeof(unsigned int));
    memcpy(vd_shm_ptr + 2 * sizeof(unsigned int), &root_dir_block, sizeof(unsigned int));

    // Initialize Block 1 - Block 8
    // The next block no for all will be 0
    memset(vd_shm_ptr + BLOCK_SIZE, 0, 8 * BLOCK_SIZE);

    // Set the first 266 blocks as occupied in Block 1
    memset(vd_shm_ptr + BLOCK_SIZE, 0xFF, 33);
    vd_shm_ptr[BLOCK_SIZE + 33] = 0x03;

    // Initialize Blocks 9-264 (FAT)
    memset(vd_shm_ptr + 9 * BLOCK_SIZE, 0, 256 * BLOCK_SIZE);

    // Initialize Block 265 (Root Directory)
    // Create the '.' and '..' entries
    struct metadata dot = {'d', ".", 2, ROOT_BLOCK};
    struct metadata dotdot = {'d', "..", 2, ROOT_BLOCK};
    memcpy(vd_shm_ptr + ROOT_BLOCK * BLOCK_SIZE, &dot, sizeof(struct metadata));
    memcpy(vd_shm_ptr + ROOT_BLOCK * BLOCK_SIZE + sizeof(struct metadata), &dotdot, sizeof(struct metadata));

    printf("Virtual Disk created and formatted successfully.\n");
    shmdt(vd_shm_ptr);
}

void removeVirtualDisk(){
    vd_shm_id = shmget(ftok(KEY, KEY_ID), VD_SIZE, 0777);
    if(vd_shm_id == -1){
        perror("shm");
        exit(1);
    }

    if(shmctl(vd_shm_id, IPC_RMID, NULL) == -1){
        perror("shm");
        exit(1);
    };
    printf("Virtual Disk removed successfully.\n");
}


int main(int argc, char const *argv[]){

    if(argc != 2){
        printf("Usage\n%s <create|remove>\n", argv[0]);
        return 1;
    }

    if(strcmp(argv[1], "create") == 0){
        createVirtualDisk();
    }else if(strcmp(argv[1], "remove") == 0){
        removeVirtualDisk();
    }else{
        printf("Invalid argument\n");
        return 1;
    }

    return 0;
}

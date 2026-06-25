#include <sys/shm.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define KEY "."
#define KEY_ID 0

#define BLOCK_SIZE 1024
#define TOTAL_BLOCKS 65536
#define ROOT_BLOCK 265
#define VD_SIZE BLOCK_SIZE * TOTAL_BLOCKS
#define NAME_LEN 23

struct metadata {
    char type;
    char name[NAME_LEN];
    unsigned int size;
    unsigned int firstblock;
};

unsigned char *D;           /* A pointer to the shared-memory segment storing VD */
unsigned int NBLOCKS;       /* The number of blocks on VD */
unsigned int NFREEBLOCKS;   /* The number of free blocks on VD */
unsigned int RBN;           /* The first-block number of the root directory */


int isallocated(int block_no){
    // Get the bit corresponding to block_no in the bitmap
    int byte_idx = block_no / 8;
    int bit_idx = block_no % 8;
    return D[BLOCK_SIZE + byte_idx] & (1 << bit_idx);
}

// Return 0 if no next block, else block no
int getnextblock(int block_no){
    int next_block;
    memcpy(&next_block, &D[9 * BLOCK_SIZE + block_no * sizeof(unsigned int)], sizeof(unsigned int));

    return next_block;
}

void setfree(int block_no){
    memset(&D[block_no * BLOCK_SIZE], 0, BLOCK_SIZE);

    // Set the bit to 0 in the bitmap
    int byte_idx = block_no / 8;
    int bit_idx = block_no % 8;
    D[BLOCK_SIZE + byte_idx] &= ~(1 << bit_idx);
}

int joindisk(){
    int vd_shm_id = shmget(ftok(KEY, KEY_ID), TOTAL_BLOCKS * BLOCK_SIZE, 0666);
    if(vd_shm_id == -1){
        perror("shm");
        return 1;
    }

    D = shmat(vd_shm_id, NULL, 0);
    if(D == NULL){
        perror("shm");
        return 1;
    }

    // Initialize the global variables
    memcpy(&NBLOCKS, D, sizeof(unsigned int));
    memcpy(&NFREEBLOCKS, D + sizeof(unsigned int), sizeof(unsigned int));
    memcpy(&RBN, D + 2 * sizeof(unsigned int), sizeof(unsigned int));

    return 0;
};

void leavedisk(){
    shmdt(D);
};

// Return -1 if no free block found, else a block number if found
int getfreeblock(){
    // Get the latest value of count of free blocks
    memcpy(&NFREEBLOCKS, D + sizeof(unsigned int), sizeof(unsigned int));
    if(NFREEBLOCKS == 0) return -1;

    // Randomly find a free block
    int free_block, byte_idx, bit_idx;
    do {
        free_block = rand() % TOTAL_BLOCKS;
        byte_idx = free_block / 8;
        bit_idx = free_block % 8;
    } while ((D[BLOCK_SIZE + byte_idx] & (1 << bit_idx)) != 0);

    // Set the block as allocated in the bitmap
    D[BLOCK_SIZE + byte_idx] |= (1 << bit_idx);

    // Decrement the count of free blocks
    NFREEBLOCKS -= 1;
    memcpy(D + sizeof(unsigned int), &NFREEBLOCKS, sizeof(unsigned int));

    return free_block;
};

void freeblock(int block_no){
    // Clear the block
    memset(&D[block_no * BLOCK_SIZE], 0, BLOCK_SIZE);

    // Set the bit to 0 in the bitmap
    int byte_idx = block_no / 8;
    int bit_idx = block_no % 8;
    D[BLOCK_SIZE + byte_idx] &= ~(1 << bit_idx);

    // Increment the count of free block
    memcpy(&NFREEBLOCKS, D + 4, 4);
    NFREEBLOCKS += 1;
    memcpy(D + 4, &NFREEBLOCKS, 4);
};

struct metadata *listdirectory(int block_no){
    if(!isallocated(block_no)) return NULL;

    struct metadata dir_metadata;
    memcpy(&dir_metadata, &D[block_no * BLOCK_SIZE], sizeof(struct metadata));

    if(dir_metadata.type != 'd') return NULL;

    int file_count = dir_metadata.size;

    struct metadata *list = (struct metadata*)malloc(sizeof(struct metadata) * file_count);

    int block = block_no;
    int file_count_block = BLOCK_SIZE / sizeof(struct metadata);

    int file_idx = 0;
    while(file_idx < file_count){
        for(int j = 0; j < file_count_block; j++){
            if(file_idx >= file_count) break;

            memcpy(&list[file_idx], &D[block * BLOCK_SIZE + j * sizeof(struct metadata)], sizeof(struct metadata));
            file_idx++;
        }
        block = getnextblock(block);
        if(block == 0) return list;
    }

    return list;
}

// Return block no of new directory else -1
int createdirectory(int parent_block, const char name[NAME_LEN]){

    // Try creating the new directory

    int new_block = getfreeblock();
    if(new_block == -1){
        printf("Error: No free block found\n");
        return -1;
    }

    struct metadata dot, dotdot;

    dot.firstblock = new_block;
    strcpy(dot.name, ".");
    dot.size = 2;
    dot.type = 'd';

    dotdot.firstblock = parent_block;
    strcpy(dotdot.name, "..");
    dotdot.size = 0;        // Saving it as zero as it doesn't really matter
    dotdot.type = 'd';

    memcpy(&D[new_block * BLOCK_SIZE], &dot, sizeof(struct metadata));
    memcpy(&D[new_block * BLOCK_SIZE + sizeof(struct metadata)], &dotdot, sizeof(struct metadata));


    // Save the new directory as an entry into the parent directory

    struct metadata new_entry;
    new_entry.type = 'd';
    strcpy(new_entry.name, name);
    new_entry.size = 2; 
    new_entry.firstblock = new_block;


    struct metadata parent;
    memcpy(&parent, &D[parent_block * BLOCK_SIZE], sizeof(struct metadata));

    int block = parent_block;
    int file_count_block = BLOCK_SIZE / sizeof(struct metadata);

    for(int i = 0; i < (parent.size - 1) / file_count_block; i++){ // (parent.size - 1) is done, so that if parent.size % file_count_block == 0, then it wouldn't return 0
        block = getnextblock(block);
    }

    if(parent.size % file_count_block != 0){
        memcpy(&D[block * BLOCK_SIZE + (parent.size % file_count_block) * sizeof(struct metadata)], &new_entry, sizeof(struct metadata)); 
    }else{ // A new block needs to be allocated to enter the new entry
        int new_parent_block = getfreeblock();
        if(new_parent_block == -1){
            freeblock(new_block);
            printf("Error: Failed to allocate a free block for parent directory\n");
            return -1;
        }

        // Save the new entry metadata into the parent directory
        memcpy(&D[9 * BLOCK_SIZE + block * sizeof(unsigned int)], &new_parent_block, sizeof(unsigned int));
        memcpy(&D[new_parent_block * BLOCK_SIZE], &new_entry, sizeof(struct metadata));

    }

    parent.size += 1;
    memcpy(&D[parent_block * BLOCK_SIZE], &parent, sizeof(struct metadata));

    return new_block;
}

// Returns starting block no of copied block else -1 if the disk is full or an error occurs.
int copyfile(int source_block) {
    // If the source has no blocks (empty file), return 0
    if (source_block == 0) return 0;

    memcpy(&NFREEBLOCKS, &D[sizeof(unsigned int)], sizeof(unsigned int));

    // Allocate the very first block for our new file copy
    int new_start_block = getfreeblock();
    if (new_start_block == -1) {
        printf("Error: No free space on virtual disk to start copy.\n");
        return -1;
    }

    int curr_src = source_block;
    int curr_dest = new_start_block;

    // Traverse the FAT chain of the source file
    while (curr_src != 0) {
        // Copy the actual 1024-byte data payload from source block to destination block
        memcpy(&D[curr_dest * BLOCK_SIZE], &D[curr_src * BLOCK_SIZE], BLOCK_SIZE);

        unsigned int next_src = getnextblock(curr_src);

        if (next_src != 0) {
            // The source has another block, so we need another block for our copy
            int next_dest = getfreeblock();
            if (next_dest == -1) {
                printf("Error: Disk ran out of space during file copy.\n");
                return -1;
            }

            // Link the current destination block to the next destination block in the FAT
            memcpy(&D[9 * BLOCK_SIZE + curr_dest * sizeof(unsigned int)], &next_dest, sizeof(unsigned int));

            // Move forward in the chain
            curr_src = next_src;
            curr_dest = next_dest;
        } else {
            // We reached the end of the source file's chain.
            unsigned int end_marker = 0;
            memcpy(&D[9 * BLOCK_SIZE + curr_dest * sizeof(unsigned int)], &end_marker, sizeof(unsigned int));
            break; // Exit the loop
        }
    }

    return new_start_block;
}

// Prints the contents of a file directly to the terminal
void printfile(int start_block, unsigned int file_size) {
    if (start_block == 0 || file_size == 0) {
        return; 
    }

    int curr_block = start_block;
    unsigned int remaining_bytes = file_size;

    // Traverse the FAT chain
    while (curr_block != 0 && remaining_bytes > 0) {
        
        // Determine how many bytes to read from the current block.
        unsigned int bytes_to_read = (remaining_bytes < BLOCK_SIZE) ? remaining_bytes : BLOCK_SIZE;

        // Print the chunk directly to standard output
        fwrite(&D[curr_block * BLOCK_SIZE], 1, bytes_to_read, stdout);

        remaining_bytes -= bytes_to_read;

        curr_block = getnextblock(curr_block);
    }
    
    printf("\n"); 
}

// Adds a new metadata entry to a parent directory.
// Returns 0 on success, -1 on failure.
int add_file_entry(int parent_block, const char *name, int size, int first_block, char type) {
    // Read the parent directory's '.' entry to get its current size
    struct metadata parent;
    memcpy(&parent, &D[parent_block * BLOCK_SIZE], sizeof(struct metadata));

    if (parent.type != 'd') {
        printf("Error: Parent block is not a directory.\n");
        return -1;
    }

    // Prepare the new entry
    struct metadata new_entry;
    new_entry.type = type;
    strncpy(new_entry.name, name, NAME_LEN - 1);
    new_entry.name[NAME_LEN - 1] = '\0'; // Ensure null-termination
    new_entry.size = size;
    new_entry.firstblock = first_block;

    // Traverse the parent's FAT chain to find the last block
    int block = parent_block;
    int entries_per_block = BLOCK_SIZE / sizeof(struct metadata); // 32

    for(int i = 0; i < (parent.size - 1) / entries_per_block; i++) {
        block = getnextblock(block);
    }

    // Insert the entry
    if(parent.size % entries_per_block != 0) {
        // There is space in the current block
        int offset = (parent.size % entries_per_block) * sizeof(struct metadata);
        memcpy(&D[block * BLOCK_SIZE + offset], &new_entry, sizeof(struct metadata)); 
    } else { 
        // The current block is full, allocate a new one
        int new_parent_block = getfreeblock();
        if(new_parent_block == -1) {
            printf("Error: Failed to allocate a free block for parent directory expansion\n");
            return -1;
        }

        // Link the old block to the new block in the FAT
        memcpy(&D[9 * BLOCK_SIZE + block * sizeof(unsigned int)], &new_parent_block, sizeof(unsigned int));
        
        // Write the entry at the start of the new block
        memcpy(&D[new_parent_block * BLOCK_SIZE], &new_entry, sizeof(struct metadata));
    }

    // Increment parent size and save it back
    parent.size += 1;
    memcpy(&D[parent_block * BLOCK_SIZE], &parent, sizeof(struct metadata));

    return 0;
}

// Searches for a file/directory by name inside the given directory block.
// If found, copies the data into 'result' and returns 1. Returns 0 if not found.
int find_entry(int dir_block, const char *name, struct metadata *result) {
    if(!isallocated(dir_block)) return 0;

    struct metadata parent;
    memcpy(&parent, &D[dir_block * BLOCK_SIZE], sizeof(struct metadata));

    if(parent.type != 'd') return 0;

    int file_count = parent.size;
    int block = dir_block;
    int entries_per_block = BLOCK_SIZE / sizeof(struct metadata);
    int entries_read = 0;

    while (block != 0 && entries_read < file_count) {
        for (int j = 0; j < entries_per_block && entries_read < file_count; j++) {
            struct metadata temp;
            memcpy(&temp, &D[block * BLOCK_SIZE + j * sizeof(struct metadata)], sizeof(struct metadata));
            
            // Check if the name matches
            if (strcmp(temp.name, name) == 0) {
                if (result != NULL) {
                    *result = temp;
                }
                return 1; // Found it!
            }
            entries_read++;
        }
        block = getnextblock(block);
    }

    return 0; // Not found
}

// Copies a file from the actual Hard Drive into the Virtual Disk.
// Returns the starting block number of the new file, or -1 on error.
int write_to_disk(unsigned int file_size, FILE *hd_file) {
    if (file_size == 0) return 0;

    // Verify we have enough space before starting
    memcpy(&NFREEBLOCKS, &D[4], sizeof(unsigned int));
    unsigned int blocks_needed = (file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    if (NFREEBLOCKS < blocks_needed) {
        printf("Error: Not enough free space on VD. Need %u blocks, have %u.\n", blocks_needed, NFREEBLOCKS);
        return -1;
    }

    int first_block = getfreeblock();
    if (first_block == -1) return -1;

    int curr_block = first_block;
    unsigned int remaining_bytes = file_size;

    while (remaining_bytes > 0) {
        unsigned int bytes_to_read = (remaining_bytes < BLOCK_SIZE) ? remaining_bytes : BLOCK_SIZE;

        // Read directly from the HD file into the Virtual Disk block
        fread(&D[curr_block * BLOCK_SIZE], 1, bytes_to_read, hd_file);
        remaining_bytes -= bytes_to_read;

        if (remaining_bytes > 0) {
            // Allocate next block and update FAT
            int next_block = getfreeblock();
            memcpy(&D[9 * BLOCK_SIZE + curr_block * sizeof(unsigned int)], &next_block, sizeof(unsigned int));
            curr_block = next_block;
        } else {
            // End of file, terminate the FAT chain
            unsigned int end_marker = 0;
            memcpy(&D[9 * BLOCK_SIZE + curr_block * sizeof(unsigned int)], &end_marker, sizeof(unsigned int));
        }
    }

    return first_block;
}

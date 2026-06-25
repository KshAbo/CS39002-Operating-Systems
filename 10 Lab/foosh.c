#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "diskutils.c"


// Shell State Variables
int CBN = 265;
char CWD[1024] = "/";

// Resolves a path string to a block number. Returns -1 if not found.
int resolve_path(const char *path) {
    if (strcmp(path, "/") == 0) return 265;
    
    int curr_block = (path[0] == '/') ? 265 : CBN;
    char temp_path[1024];
    strcpy(temp_path, path);
    
    // Tokenize the path by '/'
    char *token = strtok(temp_path, "/");
    while (token != NULL) {
        if (strcmp(token, ".") == 0) {
            token = strtok(NULL, "/");
            continue;
        }
        
        struct metadata meta;
        if (!find_entry(curr_block, token, &meta)) {
            return -1; // Path component not found
        }
        
        curr_block = meta.firstblock;
        token = strtok(NULL, "/");
    }
    
    return curr_block;
}

// Splits a path into its Parent Block Number and Target Name.
// e.g., "course/os/lab" -> parent_block = block of "course/os", name = "lab"
int get_parent_block_and_name(const char *path, int *parent_block, char *name) {
    char temp_path[1024];
    strcpy(temp_path, path);
    
    char *last_slash = strrchr(temp_path, '/');
    if (last_slash == NULL) {
        // No slashes, it's in the current directory
        *parent_block = CBN;
        strncpy(name, temp_path, NAME_LEN - 1);
        name[NAME_LEN - 1] = '\0';
    } else {
        *last_slash = '\0'; // Split string at the last slash
        char *parent_str = temp_path;
        
        if (strlen(parent_str) == 0) {
            *parent_block = 265; // Path was something like "/newdir"
        } else {
            *parent_block = resolve_path(parent_str);
        }
        
        strncpy(name, last_slash + 1, NAME_LEN - 1);
        name[NAME_LEN - 1] = '\0';
    }
    
    return (*parent_block != -1) ? 0 : -1;
}


void cmd_md(const char *path) {
    int parent_block;
    char name[NAME_LEN];
    
    if (get_parent_block_and_name(path, &parent_block, name) == -1) {
        printf("***Error: Invalid path for md\n");
        return;
    }
    
    struct metadata tmp;
    if (find_entry(parent_block, name, &tmp)) {
        printf("***Error: Directory or file already exists with that name\n");
        return;
    }
    
    createdirectory(parent_block, name);
}

void cmd_cd(const char *path) {
    if (path == NULL || strlen(path) == 0) {
        CBN = 265;
        strcpy(CWD, "/");
        return;
    }
    
    int target_block = resolve_path(path);
    if (target_block == -1) {
        printf("***Error: unable to change to directory %s\n", path);
        return;
    }
    
    // Verify it is actually a directory
    struct metadata dot;
    memcpy(&dot, &D[target_block * BLOCK_SIZE], sizeof(struct metadata));
    if (dot.type != 'd') {
        printf("***Error: %s is not a directory\n", path);
        return;
    }
    
    CBN = target_block;
    
    // Update the CWD string for the prompt
    if (path[0] == '/') {
        strcpy(CWD, path);
    } else if (strcmp(path, "..") == 0) {
        char *last_slash = strrchr(CWD, '/');
        if (last_slash != NULL && last_slash != CWD) {
            *last_slash = '\0'; // Strip last directory
        } else {
            strcpy(CWD, "/");
        }
    } else if (strcmp(path, ".") != 0) {
        if (strcmp(CWD, "/") != 0) strcat(CWD, "/");
        strcat(CWD, path);
    }
}

void cmd_ls(const char *path, int detailed) {
    int target_block = (path == NULL) ? CBN : resolve_path(path);
    
    if (target_block == -1) {
        printf("***Error: directory not found\n");
        return;
    }
    
    struct metadata *list = listdirectory(target_block);
    if (list == NULL) return;
    
    // Get total entry count from the target block's '.' entry
    struct metadata dot;
    memcpy(&dot, &D[target_block * BLOCK_SIZE], sizeof(struct metadata));
    int count = dot.size;
    
    if (detailed) {
        printf("Total %d entries\n", count);
        printf("TYPE\tNAME\t\t\tSIZE\tFIRST BLOCK\n");
    }
    
    for (int i = 0; i < count; i++) {
        if (detailed) {
            printf("%c\t%-20s\t%u\t%u\n", 
                list[i].type, 
                list[i].name, 
                list[i].size, 
                list[i].firstblock);
        } else {
            printf("%s%s\n", list[i].name, (list[i].type == 'd') ? "/" : "");
        }
    }
    free(list);
}

void cmd_cp(const char *src, const char *dest) {
    // We also fallback to standard fopen if it looks like an absolute linux path.
    int src_is_hd = (src[0] == '`' || src[0] == '~' || (src[0] == '/' && resolve_path(src) == -1));
    int dest_is_hd = (dest[0] == '`');
    
    if (src_is_hd) {
        // --- HD to VD ---
        const char *hd_path = (src[0] == '`') ? src + 1 : src;
        FILE *f = fopen(hd_path, "rb");
        if (!f) {
            printf("***Error: Unable to read input file %s\n", hd_path);
            return;
        }
        
        fseek(f, 0, SEEK_END);
        unsigned int file_size = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        int dest_parent;
        char dest_name[NAME_LEN];
        if (get_parent_block_and_name(dest, &dest_parent, dest_name) == -1) {
             printf("***Error: Destination path invalid\n");
             fclose(f);
             return;
        }
        
        // If dest is an existing directory, put the file inside it
        struct metadata check_dir;
        if (find_entry(dest_parent, dest_name, &check_dir) && check_dir.type == 'd') {
            dest_parent = check_dir.firstblock;
            const char *src_base = strrchr(hd_path, '/');
            strncpy(dest_name, src_base ? src_base + 1 : hd_path, NAME_LEN - 1);
            dest_name[NAME_LEN-1] = '\0';
        }
        
        int first_block = write_to_disk(file_size, f);
        fclose(f);
        
        if (first_block != -1) {
            add_file_entry(dest_parent, dest_name, file_size, first_block, 'f');
        }

    } else if (dest_is_hd) {
        // --- VD to HD ---
        int src_parent; char src_name[NAME_LEN];
        get_parent_block_and_name(src, &src_parent, src_name);
        
        struct metadata src_meta;
        if (!find_entry(src_parent, src_name, &src_meta) || src_meta.type != 'f') {
            printf("***Error: VD source file not found\n");
            return;
        }
        
        const char *hd_path = dest + 1; 
        FILE *f = fopen(hd_path, "wb");
        if (!f) {
            printf("***Error: Cannot create HD destination file\n");
            return;
        }
        
        int curr = src_meta.firstblock;
        unsigned int rem = src_meta.size;
        while (curr != 0 && rem > 0) {
            unsigned int to_read = (rem < BLOCK_SIZE) ? rem : BLOCK_SIZE;
            fwrite(&D[curr * BLOCK_SIZE], 1, to_read, f);
            rem -= to_read;
            curr = getnextblock(curr);
        }
        fclose(f);

    } else {
        // --- VD to VD ---
        int src_parent; char src_name[NAME_LEN];
        get_parent_block_and_name(src, &src_parent, src_name);
        
        struct metadata src_meta;
        if (!find_entry(src_parent, src_name, &src_meta) || src_meta.type != 'f') {
            printf("***Error: VD source file not found\n");
            return;
        }
        
        int dest_parent; char dest_name[NAME_LEN];
        get_parent_block_and_name(dest, &dest_parent, dest_name);
        
        struct metadata check_dir;
        if (find_entry(dest_parent, dest_name, &check_dir) && check_dir.type == 'd') {
            dest_parent = check_dir.firstblock;
            strcpy(dest_name, src_name);
        }
        
        int new_first = copyfile(src_meta.firstblock); 
        if (new_first != -1) {
            add_file_entry(dest_parent, dest_name, src_meta.size, new_first, 'f');
        }
    }
}

void cmd_prn(const char *path) {
    int parent_block;
    char name[NAME_LEN];
    if (get_parent_block_and_name(path, &parent_block, name) == -1) {
        printf("***Error: Invalid path\n");
        return;
    }
    
    struct metadata meta;
    if (!find_entry(parent_block, name, &meta) || meta.type != 'f') {
        printf("***Error: File not found or is a directory\n");
        return;
    }
    
    printfile(meta.firstblock, meta.size);
}


int main(int argc, char *argv[]) {
    // Initialize connection to Virtual Disk
    if (joindisk() != 0) {
        return 1;
    }
    
    // Print startup info matching sample output
    printf("+++ Number of blocks = %u\n", NBLOCKS);
    printf("+++ Number of free blocks = %u\n", NFREEBLOCKS);
    printf("+++ First block of the root directory = %u\n", RBN);
    
    char line[1024];
    char cmd[64], arg1[256], arg2[256];
    
    while (1) {
        printf("[foosh] VD:%s> ", CWD);
        if (fgets(line, sizeof(line), stdin) == NULL) break;
        
        line[strcspn(line, "\n")] = 0; // Strip newline
        
        // Parse up to 3 tokens
        int args = sscanf(line, "%s %s %s", cmd, arg1, arg2);
        if (args < 1) continue;
        
        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        } else if (strcmp(cmd, "md") == 0 || strcmp(cmd, "mkdir") == 0) {
            if (args >= 2) cmd_md(arg1);
            else printf("Usage: md <directory>\n");
        } else if (strcmp(cmd, "cd") == 0 || strcmp(cmd, "chdir") == 0) {
            cmd_cd((args >= 2) ? arg1 : NULL);
        } else if (strcmp(cmd, "ls") == 0) {
            cmd_ls((args >= 2) ? arg1 : NULL, 1);
        } else if (strcmp(cmd, "dir") == 0) {
            cmd_ls((args >= 2) ? arg1 : NULL, 0);
        } else if (strcmp(cmd, "cp") == 0 || strcmp(cmd, "copy") == 0) {
            if (args >= 3) cmd_cp(arg1, arg2);
            else printf("Usage: cp <src> <dest>\n");
        } else if (strcmp(cmd, "prn") == 0 || strcmp(cmd, "type") == 0) {
            if (args >= 2) cmd_prn(arg1);
            else printf("Usage: prn <file>\n");
        } else {
            printf("Unknown command: %s\n", cmd);
        }
    }
    
    // Detach before exiting
    leavedisk();
    printf("+++ Number of free blocks = %u\n", NFREEBLOCKS);
    return 0;
}

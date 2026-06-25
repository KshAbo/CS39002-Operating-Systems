#include <stdio.h>
#include <stdlib.h>

#define WIDTH 1920
#define HEIGHT 1080
#define PAGE_SIZE 4096
#define OUTPUT_OFFSET 10000000

long page_faults = 0;
long total_accesses = 0;

typedef struct {
    unsigned char R, G, B;
} pixel;

typedef struct node{
    struct node *prev, *next;
    int page_no;
} node;

node* PT[5000];

typedef struct {
    node *front, *back;
    int length, capacity;
} linked_list;

linked_list LP;

void initLP(linked_list *list, int f){
    list->front = list->back = NULL;
    list->length = 0;
    list->capacity = f;
    for(int i=0; i<5000; i++) PT[i] = NULL;
}

void handlePageFault(linked_list *list, int page_no) {
    node *n;
    if (list->length < list->capacity) {   
        n = (node*)malloc(sizeof(node));
        list->length++;
    } else {    // list is full, so replace remove the least recently used page
        n = list->back;
        PT[n->page_no] = NULL;
        
        // Remove from back
        list->back = n->prev;
        if (list->back) list->back->next = NULL;
    }
    
    // Update PT for new page
    n->page_no = page_no;
    PT[page_no] = n; 

    // Put at front
    n->next = list->front;
    n->prev = NULL;
    if (list->front) list->front->prev = n;
    list->front = n;
    if (!list->back) list->back = n; 
}

void pushNodeFront(linked_list *list, node* n){
    if (n == list->front) return;

    // Remove current
    if (n->prev) n->prev->next = n->next;
    if (n->next) n->next->prev = n->prev;
    if (n == list->back) list->back = n->prev;

    // Push to front
    n->next = list->front;
    n->prev = NULL;
    if (list->front) list->front->prev = n;
    list->front = n;
}


void accessPage(int page_no) {
    total_accesses++;
    if (PT[page_no] == NULL) {
        page_faults++;
        handlePageFault(&LP, page_no);
    } else {
        pushNodeFront(&LP, PT[page_no]);
    }
}

void accessPixel(int scheme, int r, int c, long offset) {
    long addrR, addrG, addrB, addrRP, addrGP, addrBP;
    long W = WIDTH;
    long IMG_SIZE_1 = HEIGHT * WIDTH;          // Size of one R/G/B component grid
    long PTR_ARRAY = WIDTH * 8;              // Size of dynamic row-pointer array

    switch(scheme) {
        case 1: // 3 Static Arrays
            accessPage((offset + r * W + c) / PAGE_SIZE);                                 // R
            accessPage((offset + IMG_SIZE_1 + r * W + c) / PAGE_SIZE);                    // G
            accessPage((offset + 2 * IMG_SIZE_1 + r * W + c) / PAGE_SIZE);                // B
            break;
        case 2: // 1 Static Array of char
            accessPage((offset + r * W * 3 + c * 3) / PAGE_SIZE); // R
            accessPage((offset + r * W * 3 + c * 3 + 1) / PAGE_SIZE); // G
            accessPage((offset + r * W * 3 + c * 3 + 2) / PAGE_SIZE); // B
            break;
        case 3: // 1 Static Array of Struct
            accessPage((offset + r * W * 4 + c * 4) / PAGE_SIZE);                         // Struct
            break;
        case 4: // 3 Dynamic Arrays
            addrRP = offset;
            addrGP = addrRP + PTR_ARRAY;
            addrBP = addrGP + PTR_ARRAY;
            
            // Data blocks follow the pointer blocks
            addrR  = addrBP + PTR_ARRAY;
            addrG  = addrR  + IMG_SIZE_1;
            addrB  = addrG  + IMG_SIZE_1;
            
            accessPage((addrRP + r * 8) / PAGE_SIZE); accessPage((addrR + r * W + c) / PAGE_SIZE);
            accessPage((addrGP + r * 8) / PAGE_SIZE); accessPage((addrG + r * W + c) / PAGE_SIZE);
            accessPage((addrBP + r * 8) / PAGE_SIZE); accessPage((addrB + r * W + c) / PAGE_SIZE);
            break;
        case 5: // 1 Dynamic Array of char
            accessPage((offset + r * 8) / PAGE_SIZE);                                     // RGBP ptr
            addrR = offset + PTR_ARRAY + r * W * 3 + c * 3;
            accessPage(addrR / PAGE_SIZE);                                                // R
            accessPage((addrR + 1) / PAGE_SIZE);                                          // G
            accessPage((addrR + 2) / PAGE_SIZE);                                          // B
            break;
        case 6: // 1 Dynamic Array of Struct
            accessPage((offset + r * 8) / PAGE_SIZE);                                     // RGBP ptr
            accessPage((offset + PTR_ARRAY + r * W * 4 + c * 4) / PAGE_SIZE);             // Struct data
            break;
    }
}

void blur(int width, int height, int radius, int scheme){
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            int rowstart = i - radius; if(rowstart < 0) rowstart = 0;
            int rowend = i + radius; if(rowstart >= height) rowend = height - 1;
            int colstart = j - radius; if(colstart < 0) colstart = 0;
            int colend = j + radius; if(colstart >= width) colend = width - 1;

            for(int row = rowstart; row <= rowend; row++){
                for(int col = colstart; col <= colend; col++){
                    if((row - i) * (row - i) + (col - j) * (col - j) <= radius * radius){
                        /* retrieve the (row, col)-th pixel of the image */
                        accessPixel(scheme, row, col, 0); // 0 offset = Input Image
                    }
                }
            }

            /* Write the (i, j)-th pixel to the output image */
            accessPixel(scheme, i, j, OUTPUT_OFFSET); // offset = Output Image
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: ./a.out <k> <f>\n");
        return 1;
    }

    int k = atoi(argv[1]);
    int f = atoi(argv[2]);

    char *scheme_names[] = {
        "",
        "Scheme 1: Three static arrays",
        "Scheme 2: One static array of char",
        "Scheme 3: One static array of struct",
        "Scheme 4: Three dynamic arrays",
        "Scheme 5: One dynamic array of char",
        "Scheme 6: One dynamic array of struct"
    };

    printf("+++ k=%d, f=%d\n\n", k, f);

    for (int scheme = 1; scheme <= 6; scheme++) {
        
        // Reset counters and list for each scheme
        total_accesses = 0;
        page_faults = 0;
        initLP(&LP, f);

        // Run simulation
        blur(WIDTH, HEIGHT, k, scheme);

        // Calculate timings
        double page_fault_rate = ((double)page_faults / total_accesses) * 100.0;
        double time_page_faults = (double)page_faults * 0.01;       // 10ms per fault
        double time_memory_accesses = (double)total_accesses * 1e-7; // 100ns per access
        double total_time = time_page_faults + time_memory_accesses;

        // Print output matching the PDF
        printf("+++ %s\n", scheme_names[scheme]);
        printf("Total number of memory accesses     %ld\n", total_accesses);
        printf("Total number of page faults         %ld\n", page_faults);
        printf("Page fault rate (percentage)        = %.2f\n", page_fault_rate);
        printf("Total memory access time            = %.2f sec\n\n", total_time);
    }

    return 0;
}

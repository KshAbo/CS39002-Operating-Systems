#include "manager.c"

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Usage:\n");
        printTab(1);
        printf("./foobar <foo> <bar>");
    }
    initBuffer();

    manager(atoi(argv[1]), atoi(argv[2]));

    return 0;
}

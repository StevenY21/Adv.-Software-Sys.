#include "/usr/include/bfd.h"
#include <unistd.h>
#include <stdlib.h>
#include "libobjdata.h"

int main(int argc, char *argv[]) {
    // only accepts filename
    if (argc != 2) {
        write(2, "Wrong arg size", 14);
        return -1;
    } 
    write(1, "testing getcopy for file: ", 27);
    write(1, argv[1], strlen(argv[1]));
    write(1, "\n", 1);
    int copy = get_copy(argv[1]); // creates .text copy here
    // return -1 means error, 0 means no error
    if (copy == -1) {
        write(2, "error with getting copy", 24);
        return -1;
    } else {
        return 0;
    }
}
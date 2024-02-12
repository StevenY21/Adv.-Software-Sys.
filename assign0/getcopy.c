#include "/usr/include/bfd.h"
#include <unistd.h>
#include <stdlib.h>
#include "libobjdata.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(2, "Wrong arg size", 14);
        return -1;
    } 
    int copy = get_copy(argv[1]);
    if (copy == -1) {
        write(2, "error with getting copy", 24);
        return -1;
    } else {
        return 0;
    }
}
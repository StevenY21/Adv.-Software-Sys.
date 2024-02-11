#include "/usr/include/bfd.h"
#include <unistd.h>
bfd *get_sections(const char *filename) {
    bfd_init();
    bfd *abfd =  bfd_openr(filename, NULL);
    if (abfd == NULL) {
        bfd_perror("Error when forming bfd");
        return NULL;
    }
    if(!bfd_check_format(abfd, bfd_object)){
        bfd_perror("Invalid file format");
        return NULL;
    }
    return abfd;
}
#include "/usr/include/bfd.h"
#include <unistd.h>
bfd *get_sections(const char *filename) {
    bfd_init();
    // getting and checking bfd
    bfd *abfd =  bfd_openr(filename, NULL);
    if (abfd == NULL) {
        bfd_perror("Error when forming bfd");
        return NULL;
    }
    if(!bfd_check_format(abfd, bfd_object)){
        bfd_perror("Invalid file format");
        return NULL;
    }
    // bfd itself points to section, so no need to specifically return section
    return abfd;
}
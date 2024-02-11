#include "/usr/include/bfd.h"
#include <stdlib.h>
asymbol **get_symbols(const char *filename) {
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
    long sym_storage = bfd_get_symtab_upper_bound (abfd);

    if (sym_storage == -1) {
        bfd_perror("Error occured when checking symbol space");
        return NULL;
    }

    if (sym_storage == 0) {
        bfd_perror("No symbols found");
        return NULL;
    }
    asymbol **sym_table = (asymbol **) malloc (sym_storage);
    return sym_table;
}
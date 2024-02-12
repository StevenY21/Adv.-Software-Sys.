#include "/usr/include/bfd.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
int get_copy(char *filename) {
    write(1, "creating .text copy from file: ",29 );
    write(1, filename, strlen(filename));
    bfd_init();
    bfd *abfd =  bfd_openr(filename, NULL);
    if (abfd == NULL) {
        bfd_perror("Error when forming bfd");
        return -1;
    }
    if(!bfd_check_format(abfd, bfd_object)){
        bfd_perror("Invalid file format");
        return -1;
        
    }
    asection *text_sec = bfd_get_section_by_name(abfd, ".text");
    if (text_sec==NULL) {
        bfd_perror(".text not found");
        return -1;
    }
    PTR loc;
    loc = malloc(text_sec->size);
    bool get_contents = bfd_get_section_contents(abfd, text_sec, loc, 0, text_sec->size);
    if (!get_contents) {
        bfd_perror("error with getting .text content");
    }

    int output = open("text-output", O_RDWR);
    if(output <0) {
        write(2, "cannot open text-output.txt", 28);
        return -1;
    }
    if (!write(output, loc, text_sec->size)) {
        write(2, "error with writing to text-output.txt", 38);
        return -1;
    }
    write(1, "\n.text copy created", 20);
    return 0;

}
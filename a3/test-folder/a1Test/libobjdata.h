#include "bfd.h"

bfd *get_sections(const char *filename);
asymbol **get_symbols(const char *filename);
int get_copy(char *filename);

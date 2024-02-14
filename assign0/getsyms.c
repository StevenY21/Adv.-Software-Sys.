#include "/usr/include/bfd.h"
#include <unistd.h>
//#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "libobjdata.h"
// converts int (unsigned long integer) to hex and to char and writes it to stdout
void ul_to_char(unsigned long num) {
    unsigned long in_ul = num;
    unsigned long base = 16;
    // the integers we're working with here are all base 16 as they form bytes
    char *hex[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                          "a", "b", "c", "d", "e", "f"};
    char s[16] = "";
    // maximum value of an unsigned integer
    unsigned long max_ul = 18446744073709551615;
    unsigned long counter = 0;
    // finds the most significant bit needed for the input
    while (max_ul / 16 > in_ul) {
        strcat(s, "0");
        counter++;
        max_ul = max_ul / 16;
    }
    unsigned long power = 16 - counter-1;
    // getting each digit from most to least significant bit
    while (power > 0) {
        if (in_ul == 0) {
            strcat(s, "0");
        }
        // pow could return a number a little different than expected
        // so we round
        unsigned long expo_test = round(pow(16, power));
        // if number divides fully by current base-16
        if (in_ul % expo_test == 0) {
            int quot = (int)(in_ul / expo_test);
            strcat(s,hex[quot]);
            in_ul = 0;
        } else {
            unsigned long rem = in_ul % expo_test;
            unsigned long diff = in_ul - rem;
            int quot = (int)(diff/expo_test);
            strcat(s,hex[quot]);
            in_ul = rem;
        }
        power = power - 1;
        
    }
    // for least-significant bit
    if (in_ul > 0) {
        strcat(s, hex[(int) in_ul]);
    } else {
        strcat(s, "0");
    }

    write(1,s,16);
    
}
// writes each symbols' name and virtual memory address to output
void write_symbols(asymbol **symbols, bfd *abfd) {
    bfd_init();
    long total_syms = bfd_canonicalize_symtab (abfd, symbols); // gets number of symbols
    char new_line = '\n';
    for (int i = 0; i < total_syms; i++) {
        write(1, "Name:", 5);
        write(1, symbols[i]->name, strlen(symbols[i]->name));
        write(1, " VMA:", 5);
        // vma for a symbol is the position of section the symbol's in plus the size of sumbol
        ul_to_char(symbols[i]->section->vma+symbols[i]->value);
        write(1, &new_line, 1);
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(2, "Wrong arg size", 14);
        return -1;
    }
    write(1, "testing getsyms for file: ", 27);
    write(1, argv[1], strlen(argv[1]));
    write(1, "\n", 1);
    bfd_init();
    bfd *abfd = get_sections(argv[1]);
    asymbol **sym_table = get_symbols(argv[1]);
    if (abfd == NULL || sym_table == NULL) {
        return -1;
    }
    write_symbols(sym_table, abfd); 
    return 0;
}
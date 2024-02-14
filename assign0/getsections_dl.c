//#include <stdio.h>
#include <stdlib.h>
#include<math.h>
#include "/usr/include/bfd.h"
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdint.h>

#define RDTSC(var)                                              \
  {                                                             \
    uint32_t var##_lo, var##_hi;                                \
    asm volatile("lfence\n\trdtsc" : "=a"(var##_lo), "=d"(var##_hi));     \
    var = var##_hi;                                             \
    var <<= 32;                                                 \
    var |= var##_lo;                                            \
  }

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
// turns a float into a string and writes it to stdout
void flt_to_str(float num) {
    char *dec[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    float input = num;
    // finds the number of zeroes after the decimal point
    // and insert them 
    int zeroes = floor(log10(input))*-1.0;
    char s[100] = "0.";
    for (int i = 0; i<zeroes-1; i++) {
        strcat(s, "0");
    }
    //grabs the significant figures in the float and put them in an int
    int int_input = input*round(pow(10, zeroes+2));
    int digits = floor(log10(int_input)) + 1;
    int power = digits - 1;
    // starts from the largest decimal place and goes to smallest
    // converting one digit at atime
    while (power > 0) {
        if (int_input == 0) {
            strcat(s, "");
        }
        //pow could create a value a little less or more than expected
        int expo_test = round(pow(10, power));
        // if no non-zero numbers after the curr digit
        if (int_input % expo_test == 0) {
            int quot = (int)(int_input / expo_test);
            strcat(s,dec[quot]);
            int_input = 0;
        } else {
            int rem = int_input % expo_test;
            int diff = int_input - rem;
            int quot = (int)(diff/expo_test);
            strcat(s,dec[quot]);
            int_input = rem;
        }
        power = power - 1;
    }
    // for the very last significant figure of the float
    if (int_input > 0) {
        strcat(s, dec[(int) int_input]);
    } else {
        strcat(s, "0");
    }
    write(1,s,100);
    write(1, "\n", 1);
}
// writes out the section's index, name, size, virtual memory address, and file position to stdout

void write_sections(bfd *abfd, asection *section, void *data) {
    char *dec[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    write(1, "Idx:", 5);
    write(1, dec[section->index], 2);
    write(1, " Name:", 7);
    write(1, section->name, strlen(section->name));
    write(1, " Size:", 7);
    ul_to_char(section->size);
    write(1, " VMA:", 6);
    ul_to_char(section->vma);
    write(1, " File Off:", 11);
    ul_to_char(section->filepos);
    char new_line = '\n';
    write(1, &new_line, 1);
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        write(2, "Wrong arg size", 14);
        return -1;
    } 
    int flag = 0; 
    char *filename = "";
    char *flagname = "";
    if (strcmp(argv[1],"RTLD_NOW")==0 || strcmp(argv[2],"RTLD_NOW")==0) {
        if (strcmp(argv[1],"RTLD_NOW")==0){
            filename = argv[2];
        } else {
            filename = argv[1];
        }
        flagname = "RTLD_NOW";
        flag = RTLD_NOW;
    } else if(strcmp(argv[1],"RTLD_LAZY")==0 || strcmp(argv[2],"RTLD_LAZY")==0) {
        if (strcmp(argv[1],"RTLD_LAZY")==0){
            filename = argv[2];
            
        } else {
            filename = argv[1];
        }
        flagname = "RTLD_LAZY";
        flag = RTLD_LAZY;
    } else {
        write(2, "Wrong inputs specified", 23);
        return -1;
    }
    write(1, "testing getsections_dl for file: ", 34);
    write(1, filename, strlen(filename));
    write(1, "\n", 1); 
    write(1, "with flag: ", 12);
    write(1, flagname, strlen(flagname));
    write(1, "\n", 1); 
    bfd_init();
    bfd *(*objsec_get_sections)(const char*);
    unsigned long long int start, finish;
    void * libObjdata;
    float total = 0.0;
    //49 runs here for testing, need to not close 50th to actually use it
    for (int i=0; i < 49; i++){
        RDTSC(start);
            libObjdata = dlopen("libobjdata.so", flag); 
        RDTSC(finish);
        int diff = finish-start;
        dlclose(libObjdata);
        write(1, "test run (seconds): ", 21);
        float run = (float)(diff)/ 2419196000.0;
        flt_to_str(run);
        total = total + run;
    }
    // 50th run used for actually using the library
    RDTSC(start);
            libObjdata = dlopen("libobjdata.so", flag); 
    RDTSC(finish);
    int diff = finish-start;
    write(1, "test run (seconds): ", 21);
    float run = (diff)/ 2419196000.0;
    flt_to_str(run);
    total = total + run;
    float avg = total / 50.0;
    write(1, "Overall Average (seconds): ", 28);
    flt_to_str(avg);
    //getting get_sections from the library
    *(void**)(&objsec_get_sections) = dlsym(libObjdata, "get_sections");
    bfd *abfd = objsec_get_sections(filename);
    if (abfd == NULL) {
        return -1;
    }
    bfd_map_over_sections(abfd, write_sections, NULL); // calls write_section for each section
    dlclose(libObjdata);
    return 0;
}
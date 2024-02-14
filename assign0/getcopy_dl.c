#include "/usr/include/bfd.h"
#include <unistd.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "libobjdata.h"
#include <string.h>
#include <math.h>

#define RDTSC(var)                                              \
  {                                                             \
    uint32_t var##_lo, var##_hi;                                \
    asm volatile("lfence\n\trdtsc" : "=a"(var##_lo), "=d"(var##_hi));\   
    var = var##_hi;                                             \
    var <<= 32;                                                 \
    var |= var##_lo;                                            \
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

int main(int argc, char *argv[]) {
    // accepts filename and flag
    if (argc != 3) {
        write(2, "Wrong arg size", 14);
        return -1;
    }
    int flag = 0; 
    char *filename = "";
    char *flagname = "";
    // checks if you put the flag first or the filename first
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
    write(1, "testing getcopy_dl for file: ", 30);
    write(1, filename, strlen(filename));
    write(1, "\n", 1); 
    write(1, "with flag: ", 12);
    write(1, flagname, strlen(flagname));
    write(1, "\n", 1); 

    int (*objcopy_get_copy)(const char*); // for function that will be acquired from shared library
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
        // the diff is in cycles, and my laptop processor processes 2.4 billion cycles in a second
        float run = (float)(diff)/ 2400000000.0;
        flt_to_str(run);
        total = total + run;
    }
    // last run that will be actually used by rest of program
    RDTSC(start);
            libObjdata = dlopen("libobjdata.so", flag); 
    RDTSC(finish);
    int diff = finish-start;
    write(1, "test run (seconds): ", 21);
    float run = (float)(diff)/ 2400000000.0;
    flt_to_str(run);
    total = total + run;
    //averaging out all 50 tests
    float avg = (float) total / 50.0;
    write(1, "Overall Average (seconds): ", 28);
    flt_to_str(avg); // writes avg to stdout
    *(void**)(&objcopy_get_copy) = dlsym(libObjdata, "get_copy");
    int copy = objcopy_get_copy(filename); // creates .text copy here
    dlclose(libObjdata);
    // -1 means error, 0 means success
    if (copy == -1) {
        write(2, "error with getting copy", 24);
        return -1;
    } else {
        return 0;
    }
}
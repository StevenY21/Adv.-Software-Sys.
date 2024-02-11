#include <stdio.h>
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
    asm volatile("lfence\n\trdtsc" : "=a"(var##_lo), "=d"(var##_hi));\   
    var = var##_hi;                                             \
    var <<= 32;                                                 \
    var |= var##_lo;                                            \
  }

void ul_to_char(unsigned long num) {
    unsigned long in_ul = num;
    unsigned long base = 16;
    char *hex[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                          "a", "b", "c", "d", "e", "f"};
    char s[16] = "";
    unsigned long max_ul = 18446744073709551615;
    /*if (size == 8) {
        max_ul = 4294967295;
        for (int i = 0; i < 8; i++) {
            strcat(s, "0")
        }
    } */
    unsigned long counter = 0;
    while (max_ul / 16 > in_ul) {
        strcat(s, "0");
        counter++;
        max_ul = max_ul / 16;
    }
    //write(1, s, 16);
    //printf(" counter %lu\n", counter);
    unsigned long power = 16 - counter-1;
    //printf("power %lu\n", power);
    while (power > 0) {
        //printf("test2\n");
        if (in_ul == 0) {
            strcat(s, "0");
        }
        unsigned long expo_test = round(pow(16, power));
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
    if (in_ul > 0) {
        strcat(s, hex[(int) in_ul]);
    } else {
        strcat(s, "0");
    }

    write(1,s,16);
    
}
void flt_to_str(float num) {
    char *dec[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    float input = num;
    int zeroes = floor(log10(input))*-1.0;
    char s[100] = "0.";
    
    for (int i = 0; i<zeroes-1; i++) {
        strcat(s, "0");
    }
    //grabs the non-zero numbers in the float and put them in an int
    int int_input = input*round(pow(10, zeroes+2));
    int digits = floor(log10(int_input)) + 1;
    int power = digits - 1;
    while (power > 0) {
        if (int_input == 0) {
            strcat(s, "");
        }
        int expo_test = round(pow(10, power));
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
    if (int_input > 0) {
        strcat(s, dec[(int) int_input]);
    } else {
        strcat(s, "0");
    }
    write(1, "RDTSC Avg(seconds): ", 21);
    write(1,s,100);
    write(1, "\n", 1);
}
void write_sections(bfd *abfd, asection *section, void *data) {
    // 0, 1, 2 = stdin, stdout, stderr
    //(void*)&
    char *dec[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    //vma is unsigned long int 
    //size is unsigned long int 
    //filepos is long int 
    write(1, "Idx:", 5);
    write(1, dec[section->index], 1);
    write(1, " Name:", 6);
    write(1, section->name, strlen(section->name));
    write(1, " Size:", 6);
    ul_to_char(section->size);
    write(1, " VMA:", 6);
    ul_to_char(section->vma);
    write(1, " LMA:", 5);
    ul_to_char(section->lma);
    write(1, " File Off:", 10);
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
    if (strcmp(argv[1],"RTLD_NOW")==0 || strcmp(argv[2],"RTLD_NOW")==0) {
        if (strcmp(argv[1],"RTLD_NOW")==0){
            filename = argv[2];
        } else {
            filename = argv[1];
        }
        flag = RTLD_NOW;
    } else if(strcmp(argv[1],"RTLD_LAZY")==0 || strcmp(argv[2],"RTLD_LAZY")==0) {
        if (strcmp(argv[1],"RTLD_LAZY")==0){
            filename = argv[2];
        } else {
            filename = argv[1];
        }
        flag = RTLD_LAZY;
    } else {
        write(2, "Wrong inputs specified", 23);
        return -1;
    }
    bfd_init();
    unsigned long long int start, finish;
    void * libObjdata;
    bfd *(*objsec_get_sections)(const char*);
    int total = 0;
    //49 runs here for testing, need to not close 50th to actually use it
    for (int i=0; i < 49; i++){
        RDTSC(start);
            libObjdata = dlopen("libobjdata.so", flag); 
        RDTSC(finish);
        int diff = finish-start;
        //printf("finish %llu and start %llu and diff %i", finish, start, diff);
        total = total+ diff;
        //printf("total %i\n", total);
        dlclose(libObjdata);
    }
    RDTSC(start);
            libObjdata = dlopen("libobjdata.so", flag); 
    RDTSC(finish);
    int diff = finish-start;
    //printf("finish %llu and start %llu and diff %i", finish, start, diff);
    total = total+ diff;
    //printf("total %i\n", total);
    //printf("final total %i\n", total);
    float avg = (float) total / 50.0;
    float time = (float)(avg)/ 2400000000.0;
    flt_to_str(time);
    //printf("time spent %g\n", time);
    *(void**)(&objsec_get_sections) = dlsym(libObjdata, "get_sections");
    bfd *abfd = objsec_get_sections(filename);
    if (abfd == NULL) {
        bfd_perror("invalid BFD, check if file is valid");
        return -1;
    }
    bfd_map_over_sections(abfd, write_sections, NULL);
    return 0;
}
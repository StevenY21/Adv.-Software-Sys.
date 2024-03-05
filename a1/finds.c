#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <bits/getopt_core.h>
//assignment link: https://www.cs.bu.edu/fac/richwest/cs410_spring_2024/assignments/a1/a1.html 
// goal
// arguments taken in will be the directory that needs to be traversed, option flags, and the string to look for, and possibly a wildcard unless they want it in the string
// traverse directory: https://codeforwin.org/c-programming/c-program-to-list-all-files-in-a-directory-recursively
// find occurence of word in file https://codeforwin.org/c-programming/c-program-find-occurrence-of-a-word-in-file
// need to look into how the hell to build my own regex
// flag -f [file type] will set the program to look for files with the specific file type
// flag -l will look for symbolics links, gotta print out symlink, the file it references, and the line in the file that has the string if string found
// need to look into how tf to do that : https://stackoverflow.com/questions/1569402/following-symbolic-links-in-c 
void findWord(FILE *fptr, const char *expression, char* path) {
    char str[1000];
    char *inStr;
    while ((fgets(str, 1000, fptr)) != NULL)
    {
        // Find first occurrence of expression in str
        inStr = strstr(str, expression);

        if (inStr != NULL)
        {
            printf("%s, %s\n",str, path);
        }
    }
}
void findFiles(char *base, char* str, char* f, bool l) {
    //printf("checking for %s in %s", str, base);
    char path[1000];
    struct dirent *dp;
    DIR *dir = opendir(base);
    // Unable to open directory
    if (dir == NULL) {
        return;
    }
    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            // Construct new path from our base path
            strcpy(path, base);
            strcat(path, "/");
            strcat(path, dp->d_name);
            //printf("found path: %s\n", path);
            struct stat statBuf;
            int statChk = lstat(path, &statBuf); // using lstat to determine if it's a file or link: https://pubs.opengroup.org/onlinepubs/9699919799/functions/stat.html
            char actualPath[1024]; //for getting the content of the symbolic link
            char * ptr;
            ssize_t len;
            if (S_ISLNK(statBuf.st_mode) && l == 1) { // if sym link and -l flag used
            // https://pubs.opengroup.org/onlinepubs/009696799/functions/realpath.html
                ptr = realpath(path, actualPath);
                if (ptr != NULL) { //based on https://pubs.opengroup.org/onlinepubs/9699919799/functions/readlink.html
                    //printf("found sym path %s\n", ptr);
                    if (f != NULL) {
                        if (strstr(ptr, f) != NULL) {
                            FILE *fptr = fopen(ptr, "r");
                            if (fptr != NULL){
                                findWord(fptr, str, ptr);
                                //printf("going here for %s\n", ptr); //test
                                findFiles(ptr, str, f, l);
                            } else {
                                printf("cannot open %s", ptr);
                            }
                        }
                    } else {
                        FILE *fptr = fopen(ptr, "r");
                        if (fptr != NULL){
                            findWord(fptr, str, ptr);
                            findFiles(ptr, str, f, l);
                        } else {
                            printf("cannot open %s", ptr);
                        }
                    }
                }
            } else if(S_ISREG(statBuf.st_mode)) { // if just a regular file
                if (f != NULL) {
                    if (strstr(dp->d_name, f) != NULL) {
                        FILE *fptr = fopen(path, "r");
                        if (fptr != NULL){
                            findWord(fptr, str, path);
                        }
                    }
                } else {
                    FILE *fptr = fopen(path, "r");
                    if (fptr != NULL){
                        findWord(fptr, str, path);
                    }
                }
                findFiles(path, str, f, l);
            }
        }
    }

    closedir(dir);
}
int main(int argc, char **argv) {
    // based on the manual on getopt: https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html 
    char* pFlag =NULL;
    char* fFlag = NULL;
    bool lFlag = false;
    char *sFlag = NULL;
    int index;
    int c;
    while ((c = getopt (argc, argv, "p:f:ls:")) != -1)
        switch (c)
        {
        case 'p':
            if (optarg != NULL) {
                pFlag = optarg;
            }
            break;
        case 'f':
            if (optarg != NULL) {
                fFlag = optarg;
            }
            break;
        case 'l':
            lFlag = true;
            break;
        case 's':
            if (optarg != NULL) {
                sFlag = optarg;
            }
            break;
        case '?':
            if (optopt == 'p')
            fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (optopt == 's')
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
            fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
            fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
            return -1;
        default:
            abort ();
        }
    printf ("p = %s, f = %s, l = %d, s = %s\n",
        pFlag, fFlag, lFlag, sFlag);
    for (index = optind; index < argc; index++)
        printf ("Non-option argument %s\n", argv[index]);
    if (fFlag != NULL) {
        if (strcmp(fFlag,"S") == 0) {
            fFlag = ".s";
        } else if (strcmp(fFlag,"c") == 0) {
            fFlag =".c";
        } else if (strcmp(fFlag,"h") == 0) {
            fFlag = ".h";
        } else {
            fprintf(stderr, "Invalid option arg for -f: %s", fFlag);
            return -1;
        }
    } 
    findFiles(pFlag, sFlag, fFlag, lFlag);
    return 0;


}
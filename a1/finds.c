#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <bits/getopt_core.h>
//assignment link: https://www.cs.bu.edu/fac/richwest/cs410_spring_2024/assignments/a1/a1.html 
// goals still need done: REGEX ONLY NOW
// using this to test regex: https://regex101.com/ 
int passCtrlChr(char *str, char ctrlChr, char beforeCtrl, char afterCtrl, int startIdx) {
    //printf("ctrl op %c %s %c %c\n", ctrlChr, str, beforeCtrl, afterCtrl);
    int trav = startIdx;
    if (ctrlChr == '.') { 
        if (beforeCtrl == NULL) { 
            if (afterCtrl == '\0') {
                return startIdx;
            } else if (afterCtrl == '.' || afterCtrl == '*' || afterCtrl == '?') { //regex auto works for any string when . is next to the other specified chars
                //printf("%s", "get here2");
                return trav;
            } else {
                if (str[trav+2] == afterCtrl) { //checks if characters starting from startIdx to startIdx +2 gets something like a.b
                    return trav+3;
                } else {
                    return -1;
                }
            }
        } else if( afterCtrl == "\0") { // ex: "b."
        //printf("%s", "got here3");
            if (str[trav] == beforeCtrl) {
                return trav+2;
            } else {
                return -1;
            }
        } else if (afterCtrl == '*' || afterCtrl == '?' ) { //.* and .? makes whole string valid
            //printf("%s", "got here4");
            return trav;
        } else if (beforeCtrl == '.' || afterCtrl == '.' ){ //.. also works according to regex website for any string
            return trav;
        } else { // for things like a.b
            //printf("%s", "got here5");
            if (str[trav] == beforeCtrl  && str[trav+2] == afterCtrl) {
                return trav+3;
            } else {
                return -1;
            }
        }
    } else if (ctrlChr == '?') { // the 0 or 1 instance of char before ?
        if (beforeCtrl == NULL || (isalnum(beforeCtrl) == 0 && beforeCtrl != '.')) { //cannot have nothing or symbols before ?
            fprintf("stderr", "invalid use of control character");
            exit(-1);
        } else if ((afterCtrl == '.' ||  afterCtrl == '?')) { // so long as some valid char before
            return trav;
        } else if (afterCtrl == '*'){ // ?* will break
            fprintf("stderr", "invalid use of control character");
            exit(-1);            
        } else {
            int beforeCtrls = 0;
            while(str[trav] != '\0' && str[trav] !=  afterCtrl) {
                if (str[trav] != beforeCtrl) {
                    printf("%s", "stopped here");
                    return -1;
                } else {
                    if (str[trav] == beforeCtrl) {
                        beforeCtrls++;
                    }
                }
                trav++;

            }
            if (beforeCtrls > 1) {
                    return -1;
            } else {
                return trav+1;
            }

        }
    } else if (ctrlChr == "*") {
        if (beforeCtrl == NULL) {
            fprintf("stderr", "invalid use of control character");
            exit(-1);
        } else if (afterCtrl == '.' ||  afterCtrl == '?') {
            return trav;
        } else if (afterCtrl == '*'){
            fprintf("stderr", "invalid use of control character");
            exit(-1);            
        } else {
            while(str[trav] != '\0') {
                if (str[trav] == afterCtrl) {
                    return trav+1;
                }
                trav++;
            }
        }
    }
    return -1;
}
int findRegExp(FILE *fptr, char *expression, char* path) {
    char str[1000];
    int foundExp = 0;
     while ((fgets(str, 1000, fptr)) != NULL)
    {
        size_t strIdx = 0;
        size_t expIdx = 0;
        while(str[strIdx] != '\0' && expression[expIdx] != '\0') {
            //printf("expression char rn %c\n", expression[expIdx]);
            int passedStep = -1;
            int incExp = 0;
            if (isalnum(expression[expIdx]) ==0) { //current index a 
                if (expIdx == 0) {// first value in exp a control character
                    //printf("got here0 %c\n", expression[expIdx+1]);
                    passedStep = passCtrlChr(str, expression[expIdx], NULL, expression[expIdx+1], 0);\
                    incExp += 2;
                } else {
                    //printf("got here1 %c", expression[expIdx]);
                    passedStep = passCtrlChr(str, expression[expIdx], expression[expIdx-1], expression[expIdx+1], strIdx);
                    incExp += 3;
                }        
            } else if (isalnum(expression[expIdx+1]) == 0 && expression[expIdx+1] != '\0'){
                //printf("got here3 %c %s %d %d\n", expression[expIdx+1], expression, expIdx+1, strIdx);
                passedStep = passCtrlChr(str, expression[expIdx+1], expression[expIdx], expression[expIdx+2], strIdx);
                //printf("passed? %d\n", passedStep);
                incExp += 3;
            } else { // regular char comparison
                //printf("%s %c %c %d", "\ntesting regular char comp",expression[expIdx], str[strIdx], strIdx);
                if(expression[expIdx] == str[strIdx]){
                    passedStep = strIdx + 1;
                    incExp += 1;
                }
            }
            // after testing the expressions
            if (passedStep == -1) {
                //printf("%s", "\ntest failed");
                expIdx = 0;
                strIdx ++;
            } else {
                strIdx = passedStep;
                expIdx += incExp;
            }
        }
        if (str[strIdx] == '\0' && expression[expIdx] == '\0') {
            foundExp++;
            printf("%s :found in %s\n",str, path);
        } else if (str[strIdx] != '\0' && expression[expIdx] == '\0') {
            foundExp++;
            printf("%s :found in %s\n",str, path);
        }
    }
    if (foundExp >= 1) {
        return 1;
    } else {
        return 0;
    }

}
int findExp(FILE *fptr, char *expression, char* path) {
    char str[1000];
    char *inStr;
    int foundStr = 0;
    while ((fgets(str, 1000, fptr)) != NULL)
    {
        // Find first occurrence of expression in str
        inStr = strstr(str, expression);
        if (inStr != NULL)
        {   
            foundStr++;
            printf("%s :found in %s\n",str, path);
        }
    }
    if (foundStr >= 1) {
        return 1;
    } else {
        return 0;
    }
}
int checkFile(char *path, char* f, char* exp, int hasCtrl) {
    int foundExp = 0; // checking if expression found in file
    if (f != NULL) {
        if (strstr(path, f) != NULL) {
            FILE *fptr = fopen(path, "r");
            if (fptr != NULL){
                if (hasCtrl == 1) {
                    foundExp = findRegExp(fptr, exp, path);
                } else {
                    foundExp = findExp(fptr, exp, path);
                }
                //printf("going here for %s\n", ptr); //test
                return foundExp;
            } else {
                fprintf(stderr, "cannot open %s", path);
                return -1;
            }
        }
    } else {
        FILE *fptr = fopen(path, "r");
        if (fptr != NULL){
            if (hasCtrl == 1) {
                foundExp = findRegExp(fptr, exp, path);
            } else {
                foundExp = findExp(fptr, exp, path);
            }
            return foundExp;
        } else {
            fprintf(stderr, "cannot open %s", path);
            return -1;
        }
    }
}
void findFiles(char *base, char* symLink, char* str, char* f, int l, int hasCtrl) {
    //printf("checking for %s in %s", str, base);
    char path[1000];
    struct dirent *dp;
    DIR *dir = opendir(base);
    int foundExp = 0; // checking if expression found in file
    // Unable to open the directory
    if (dir == NULL) {
        //printf("invalid direct %s\n", base);
        //fprintf(stderr, "invalid direct %s\n", base);
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
            if (statChk == -1) {
                return;
            }
            if (S_ISLNK(statBuf.st_mode) && l == 1) { // if sym link and -l flag used
            // https://pubs.opengroup.org/onlinepubs/009696799/functions/realpath.html
                char actualPath[1024]; //for getting the content of the symbolic link
                char * ptr; 
                ptr = realpath(path, actualPath);
                if (ptr != NULL) { 
                    //printf("found sym path %s\n", ptr);
                    char *tempSym = path;
                    foundExp = checkFile(ptr, f, str, hasCtrl);
                    if (foundExp == 1) {
                        printf("symbolic link file: %s\n", tempSym);
                    } else if (foundExp == -1) {
                        return;
                    }
                    findFiles(ptr, tempSym, str, f, l, hasCtrl);
                } else {
                    fprintf(stderr, "cannot open %s", ptr);
                    return;
                }
            } else if(S_ISREG(statBuf.st_mode)) { // if just a regular file
                foundExp = checkFile(path, f, str, hasCtrl);
                if (foundExp == 1 && symLink != NULL) {
                    printf("symbolic link file: %s\n", symLink);
                } else if (foundExp == -1) {
                    return;
                }
                findFiles(path, symLink, str, f, l, hasCtrl);
            } else {
                //might be a directory
                findFiles(path, symLink, str, f, l, hasCtrl);
            }
        }
    }

    closedir(dir);
}
int main(int argc, char **argv) {
    // based on the manual on getopt: https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html 
    char* pFlag =NULL;
    char* fFlag = NULL;
    int lFlag = 0;
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
            lFlag = 1;
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
    for (index = optind; index < argc; index++) {
        fprintf (stderr, "Non-option argument %s\n", argv[index]);
    }
    if (fFlag != NULL) {
        if (strcmp(fFlag,"S") == 0) {
            fFlag = ".s";
        } else if (strcmp(fFlag,"c") == 0) {
            fFlag =".c";
        } else if (strcmp(fFlag,"h") == 0) {
            fFlag = ".h";
        } else {
            fprintf(stderr, "Invalid option arg for -f: %s\n", fFlag);
            return -1;
        }
    } 
    char *p = sFlag;
    int hasCtrl = 0; // to check if control characters found
    while (*p != '\0') {
        int checkAlNum = isalnum(*p);
        if (checkAlNum == 0){
            if (*p == '?' || *p == '*' || *p == '(' || *p == ')' || *p == '.') {
                //printf("found control character found %c\n", *p);
                hasCtrl = 1;
            } else {
                fprintf(stderr, "Invalid expression string: %s\n", sFlag);
                return -1;
            }
        }
        p++;
    }
    findFiles(pFlag, NULL, sFlag, fFlag, lFlag, hasCtrl);
    return 0;


}
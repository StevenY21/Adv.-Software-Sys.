#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 2056
#define MAX_NAME_VALUE_PAIRS 2056

// Define a struct for name value pairs
typedef struct {
    char name[MAX_LINE_LENGTH];
    char value[MAX_LINE_LENGTH];
} NameValuePair;

// Initialize pair array and pairCount variable
NameValuePair pairs[MAX_NAME_VALUE_PAIRS];
int pairCount = 0;

// findPair, function to search for matching pair with name
int findPair(char* name) {
    for (int i = 0; i < pairCount; i++) {
        if (strcmp(pairs[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int main(int argc, char *argv[]) {
    // Initializing our variables, the file and the line
    FILE *file;
    char line[MAX_LINE_LENGTH];
    // If there's a filename, open it for reading, otherwise set to stdin
    if (argc > 1) {
        file = fopen(argv[1], "r");
        if (file == NULL) {
            printf("Could not open file %s\n", argv[1]);
            return 1;
        }
    } else {
        file = stdin;
    }
    // While there's input
    while (fgets(line, sizeof(line), file)) {
        // Use strtok to separate the name and value since we know the form of the data
        char* name = strtok(line, "=");
        char* value = strtok(NULL, "\n");
        // If we have both a name and value (correct formatting)
        if (name && value) {
            // Use findPair to see if name already exists in our pairs array
            int index = findPair(name);
            // If there's no match, add a new pair to array and write
            if (index == -1) {
                strcpy(pairs[pairCount].name, name);
                strcpy(pairs[pairCount].value, value);
                pairCount++;
                printf("%s=%s\n", pairs[pairCount-1].name, pairs[pairCount-1].value);
            } else {
                // If there is a match, check if the value differs
                if (strcmp(pairs[index].value, value) != 0) {
                    // If it differs, update and write
                    strcpy(pairs[index].value, value);
                    printf("%s=%s\n", pairs[index].name, pairs[index].value);
                }
            }
        }
    }
    // If we were using file for input, close it
    if (argc > 1) {
        fclose(file);
    }
    // Exit
    return 0;
}
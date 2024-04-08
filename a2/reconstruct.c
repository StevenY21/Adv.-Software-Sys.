#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LENGTH 50
#define MAX_VALUE_LENGTH 50
#define MAX_PAIRS 100

typedef struct {
    char name[MAX_NAME_LENGTH];
    char value[MAX_VALUE_LENGTH];
} NameVal;

typedef struct {
    char currSample[MAX_PAIRS][MAX_NAME_LENGTH];
    int nextEmpty; // for tracking next open array spot
} Sample;

int main() {
    NameVal nameValPairs[MAX_PAIRS];
    int numPairs = 0;
    int numUniques = 0;
    char headName[MAX_NAME_LENGTH];
    char endName[MAX_NAME_LENGTH];
    char prevChar[MAX_NAME_LENGTH];
    char *input[] = {"expt=1", "temperature=60 F", "time=10 s", "time=20 s", "temperature=61 F", "time=30 s", "expt=2", "temperature=64 F", "time=40 s", "time=50 s"};
    int numInputs = sizeof(input) / sizeof(input[0]);
    char uniqueNames[numInputs][MAX_NAME_LENGTH];

    // Getting all the name value pairs set up, as well as checking the number of unique names
    for (int i = 0; i < numInputs; i++) {
        char name[MAX_NAME_LENGTH];
        char value[MAX_VALUE_LENGTH];
        if (sscanf(input[i], "%[^=]=%[^\n]", name, value) == 2){
            strcpy(nameValPairs[numPairs].name, name);
            strcpy(nameValPairs[numPairs].value, value);
            numPairs++;
            if (numUniques == 0 || strcmp(name, prevChar) != 0) {
                if (numUniques == 0) {
                    strcpy(headName, name);
                    strcpy(uniqueNames[numUniques], name);
                    numUniques++;
                    strcpy(prevChar, name);
                } else if (strcmp(name, headName) == 0 && strcmp(endName, "") == 0) {
                    strcpy(endName, prevChar);
                } else {
                    int duped = 0;
                    for (int j = 0; j < numPairs-1; j++) {
                        if (strcmp(nameValPairs[j].name, name) == 0) {
                            duped = 1;
                            break;
                        }
                    }
                    if (duped == 0) {
                        strcpy(uniqueNames[numUniques], name);
                        numUniques++;
                        strcpy(prevChar, name);
                    }
                }
            }
        }
    }

    printf("%-20s", "Sample #");
    for (int i = 0; i < numUniques; i++) {
        printf(" %-20s", uniqueNames[i]);
    }
    printf("%s", "\n");

    char prevName[MAX_NAME_LENGTH];
    int currSampleNum = 1; // Start sample numbering from 1
    Sample samples[100];
    int i = 0;

    while(i < numPairs) {
        // If it's the head of the sample
        if (strcmp(nameValPairs[i].name, prevName) != 0) {
            char name[MAX_NAME_LENGTH];
            strcpy(name, nameValPairs[i].name);
            int nameIdx = 0;
            // Find the unique name index
            for (int j = 0; j < numUniques; j++) {
                if (strcmp(uniqueNames[j], name) == 0) {
                    nameIdx = j;
                    break;
                }
            }
            // Copy previous sample if exists
            if (strcmp(samples[currSampleNum].currSample[nameIdx], "") != 0){
                currSampleNum++;
                memcpy(samples[currSampleNum].currSample, samples[currSampleNum-1].currSample, sizeof(samples[currSampleNum-1].currSample));
                strcpy(samples[currSampleNum].currSample[nameIdx], nameValPairs[i].value);
                // Clear values of other names after the current name
                for (int j = nameIdx+1; j < numUniques; j++) {
                    if (strcmp(samples[currSampleNum].currSample[j], "") != 0) {
                        strcpy(samples[currSampleNum].currSample[j], "");
                    }
                }
                samples[currSampleNum].nextEmpty = nameIdx+1;
            } else {
                strcpy(samples[currSampleNum].currSample[samples[currSampleNum].nextEmpty], nameValPairs[i].value);
                samples[currSampleNum].nextEmpty++;
            }
        } else { // Same name found
            char name[MAX_NAME_LENGTH];
            strcpy(name, nameValPairs[i].name);
            int nameIdx = 0;
            // Find the unique name index
            for (int j = 0; j < numUniques; j++) {
                if (strcmp(uniqueNames[j], name) == 0) {
                    nameIdx = j;
                    break;
                }
            }
            // Copy previous sample if exists
            currSampleNum++;
            memcpy(samples[currSampleNum].currSample, samples[currSampleNum-1].currSample, sizeof(samples[currSampleNum-1].currSample));
            strcpy(samples[currSampleNum].currSample[nameIdx], nameValPairs[i].value);
            // Clear values of other names after the current name
            for (int j = nameIdx+1; j < numUniques; j++) {
                if (strcmp(samples[currSampleNum].currSample[j], "") != 0) {
                    strcpy(samples[currSampleNum].currSample[j], "");
                }
            }
        }
        strcpy(prevName, nameValPairs[i].name);
        i++;
    }

    for(int i = 1; i <= currSampleNum; i++) {
        printf("%-20d", i);
        for(int j = 0; j < numUniques; j++) {
            printf(" %-20s", samples[i].currSample[j]);
        }
        printf("%s", "\n");
    }

    return 0;
}

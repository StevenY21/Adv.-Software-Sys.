#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>

#define MAX_NAME_LENGTH 50
#define MAX_VALUE_LENGTH 50
#define MAX_PAIRS 100
#define MAX_SAMPLES 100
#define MAX_STRING_SIZE 2056

typedef struct {
    char name[MAX_NAME_LENGTH];
    char value[MAX_VALUE_LENGTH];
} NameVal;

typedef struct {
    char currSample[MAX_PAIRS][MAX_NAME_LENGTH];
    int nextEmpty; // for tracking next open array spot
} Sample;

// Structure for shared data
typedef struct {
    char **buffer;   // Shared buffer
    int in;        // Index for inserting into the buffer
    int out;       // Index for removing from the buffer
    int size;      // Size of the buffer
    pthread_mutex_t mutex;
    int input_done;
} shared_data_t;

void writeToWriteBuffer(shared_data_t *write_shared_data, Sample samples[], int numSamples, int numUniques, char uniqueNames[MAX_PAIRS][MAX_NAME_LENGTH]) {
    for (int i = 1; i <= numSamples; i++) {
        char sampleData[MAX_NAME_LENGTH * MAX_PAIRS] = "";
        for (int j = 0; j < numUniques; j++) {
            strcat(sampleData, uniqueNames[j]);
            strcat(sampleData, "=");
            strcat(sampleData, samples[i].currSample[j]);
            if (j < numUniques - 1) {
                strcat(sampleData, ", ");
            }
        }
        sprintf(*(write_shared_data->buffer + write_shared_data->in), "%s", sampleData);
        printf("Buffer: %s\n", *(write_shared_data->buffer + write_shared_data->in));
        write_shared_data->in = (write_shared_data->in + 1) % write_shared_data->size;
    }
}

int main(int argc, char *argv[]) {
    int read_shm_id = atoi(argv[1]);
    int write_shm_id = atoi(argv[2]);
    printf("Attaching to read shared memory segment ID: %d and write shared memory segment id: %d\n", read_shm_id, write_shm_id);
    shared_data_t *read_shared_data = (shared_data_t *)shmat(read_shm_id, NULL, 0);
    if ((void *)read_shared_data == (void *)-1) {
        perror("shmat");
        return 1;
    }
    shared_data_t *write_shared_data = (shared_data_t *)shmat(write_shm_id, NULL, 0);
    if ((void *)write_shared_data == (void *)-1) {
        perror("shmat");
        return 1;
    }

    // Allocate memory for the buffer within the shared memory segment
    read_shared_data->buffer = (char **)((char *)read_shared_data + sizeof(shared_data_t));
    write_shared_data->buffer = (char **)((char *)write_shared_data + sizeof(shared_data_t));

    // Initialize each element of the buffer
    for (int j = 0; j < read_shared_data->size; j++) {
        read_shared_data->buffer[j] = (char *)(read_shared_data->buffer + read_shared_data->size) + j * MAX_STRING_SIZE;
    }
    for (int j = 0; j < write_shared_data->size; j++) {
        write_shared_data->buffer[j] = (char *)(write_shared_data->buffer + write_shared_data->size) + j * MAX_STRING_SIZE;
    }

    NameVal nameValPairs[MAX_PAIRS];
    int numPairs = 0;
    int numUniques = 0;
    char headName[MAX_NAME_LENGTH];
    char endName[MAX_NAME_LENGTH];
    char prevChar[MAX_NAME_LENGTH];

    char uniqueNames[MAX_PAIRS][MAX_NAME_LENGTH];

    // Getting all the name value pairs set up, as well as checking the number of unique names
    while(1) {
        char name[MAX_NAME_LENGTH];
        char value[MAX_VALUE_LENGTH];
 
        // Read from the buffer
        if (sscanf(*(read_shared_data->buffer + read_shared_data->out), "%[^=]=%[^\n]", name, value) == 2) {
            printf("Read from buffer: Name=%s, Value=%s\n", name, value);
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
                    for (int j = 0; j < numPairs - 1; j++) {
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
            // Move to the next position in the buffer
            read_shared_data->out = (read_shared_data->out + 1) % read_shared_data->size;
        } else {
            break;
        }
    }

    printf("%-20s", "Sample #");
    for (int i = 0; i < numUniques; i++) {
        printf(" %-20s", uniqueNames[i]);
    }
    printf("%s", "\n");

    char prevName[MAX_NAME_LENGTH];
    int currSampleNum = 1;
    Sample samples[MAX_SAMPLES];
    int i = 0;

    while (i < numPairs) {
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
            if (strcmp(samples[currSampleNum].currSample[nameIdx], "") != 0) {
                currSampleNum++;
                memcpy(samples[currSampleNum].currSample, samples[currSampleNum - 1].currSample,
                       sizeof(samples[currSampleNum - 1].currSample));
                strcpy(samples[currSampleNum].currSample[nameIdx], nameValPairs[i].value);
                // Clear values of other names after the current name
                for (int j = nameIdx + 1; j < numUniques; j++) {
                    if (strcmp(samples[currSampleNum].currSample[j], "") != 0) {
                        strcpy(samples[currSampleNum].currSample[j], "");
                    }
                }
                samples[currSampleNum].nextEmpty = nameIdx + 1;
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
            memcpy(samples[currSampleNum].currSample, samples[currSampleNum - 1].currSample,
                   sizeof(samples[currSampleNum - 1].currSample));
            strcpy(samples[currSampleNum].currSample[nameIdx], nameValPairs[i].value);
            // Clear values of other names after the current name
            for (int j = nameIdx + 1; j < numUniques; j++) {
                if (strcmp(samples[currSampleNum].currSample[j], "") != 0) {
                    strcpy(samples[currSampleNum].currSample[j], "");
                }
            }
        }
        strcpy(prevName, nameValPairs[i].name);
        i++;
    }

    for (int i = 1; i <= currSampleNum; i++) {
        printf("%-20d", i);
        for (int j = 0; j < numUniques; j++) {
            printf(" %-20s", samples[i].currSample[j]);
        }
        printf("%s", "\n");
    }

    // Write samples to the write buffer
    writeToWriteBuffer(write_shared_data, samples, currSampleNum, numUniques, uniqueNames);

    // Change input done flag
    write_shared_data->input_done = 1;
    // Detach from shared memory
    if (input_args->buffer_type == "sync") {
        pthread_mutex_unlock (&shared_data->mutex);
    }
    if (shmdt(read_shared_data) == -1) {
        perror("shmdt");
        return 1;
    }
    if (shmdt(write_shared_data) == -1) {
        perror("shmdt");
        return 1;
    }

    return 0;
}
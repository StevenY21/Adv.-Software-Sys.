#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <getopt.h>
#include <assert.h>
#include <semaphore.h>

#define MAX_NAME_LENGTH 50
#define MAX_VALUE_LENGTH 50
#define MAX_PAIRS 2000
#define MAX_SAMPLES 650
#define MAX_STRING_SIZE 2056
#define MAX_BUFFER_SIZE 50

typedef struct {
    char name[MAX_NAME_LENGTH];
    char value[MAX_VALUE_LENGTH];
} NameVal;

typedef struct {
    char currSample[MAX_PAIRS][MAX_NAME_LENGTH];
    int nextEmpty;
} Sample;

// Structure for shared data
typedef struct {
    char buffer[MAX_BUFFER_SIZE][MAX_STRING_SIZE];
    int in;
    int out;
    int size;
    sem_t sem_empty;
    sem_t sem_full;
    sem_t sem_mutex;
    int input_done;
} shared_data_t;
typedef struct {
    char *shm_id_str_read;
    char *shm_id_str_write;
    char *input_file;
    char *buffer_type;
    char *argn;
} fn_args; // all of the arguments needed for observe, reconstruct, and tapplot

int main(int argc, char *argv[]) {
    int read_shm_id = atoi(argv[1]);
    int write_shm_id = atoi(argv[2]);
    
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

    NameVal* nameValPairs = (NameVal*) malloc(MAX_PAIRS * sizeof(NameVal));
    int numPairs = 0;
    int numUniques = 0;
    char headName[MAX_NAME_LENGTH];
    char endName[MAX_NAME_LENGTH];
    char prevChar[MAX_NAME_LENGTH];

    char (*uniqueNames)[MAX_NAME_LENGTH] = malloc(MAX_PAIRS * sizeof(*uniqueNames));

    // Getting all the name value pairs set up, as well as checking the number of unique names
    while(1) {
        char name[MAX_NAME_LENGTH];
        char value[MAX_VALUE_LENGTH];
        usleep(1000);
        int empty;
        sem_getvalue(&read_shared_data->sem_full, &empty);
        if (empty == 0 && read_shared_data->input_done) {
            break;
        }

        sem_wait(&read_shared_data->sem_full);

        sem_wait(&read_shared_data->sem_mutex);

        // Read from the buffer
        char *data = read_shared_data->buffer[read_shared_data->out];
        if (sscanf(data, "%[^=]=%[^\n]", name, value) == 2) {
            printf("Reconstruct read from buffer: Name=%s, Value=%s\n", name, value);
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
            sem_post(&read_shared_data->sem_mutex);
        }
        sem_post(&read_shared_data->sem_empty);
    }

    printf("%-20s", "Sample #");
    for (int i = 0; i < numUniques; i++) {
        printf(" %-20s", uniqueNames[i]);
    }
    printf("%s", "\n");

    // Initialize the last encountered values for each name
    char (*lastValues)[MAX_VALUE_LENGTH] = malloc(MAX_PAIRS * sizeof(*lastValues));
    int *seenInCurrentSample = malloc(MAX_PAIRS * sizeof(int));

    memset(lastValues, 0, MAX_PAIRS * sizeof(*lastValues));
    memset(seenInCurrentSample, 0, MAX_PAIRS * sizeof(int));

    int currSampleNum = 0;
    Sample* samples = (Sample*) malloc(MAX_SAMPLES * sizeof(Sample));
    memset(samples, 0, MAX_SAMPLES * sizeof(Sample));

    for (int i = 0; i < numPairs; i++) {
        char* name = nameValPairs[i].name;
        char* value = nameValPairs[i].value;

        // Find the unique name index
        int nameIdx = -1;
        for (int j = 0; j < numUniques; j++) {
            if (strcmp(uniqueNames[j], name) == 0) {
                nameIdx = j;
                break;
            }
        }
        // If we've seen this name in the current sample, start a new sample
        if (seenInCurrentSample[nameIdx]) {
            currSampleNum++;
            memset(seenInCurrentSample, 0, MAX_PAIRS * sizeof(int)); // Reset seenInCurrentSample for the new sample
        }

        // Update the value for the current name in the current sample
        strcpy(samples[currSampleNum].currSample[nameIdx], value);
        // Update the last encountered value for this name
        strcpy(lastValues[nameIdx], value);
        // Mark this name as seen in the current sample
        seenInCurrentSample[nameIdx] = 1;

        // For all names we haven't seen in the current sample, use the last encountered value
        for (int j = 0; j < numUniques; j++) {
            if (!seenInCurrentSample[j]) {
                strcpy(samples[currSampleNum].currSample[j], lastValues[j]);
            }
        }
    }

    for (int i = 1; i <= currSampleNum; i++) {
        printf("%-20d", i);
        for (int j = 0; j < numUniques; j++) {
            printf(" %-20s", samples[i].currSample[j]);
        }
        printf("%s", "\n");
    }

    for (int i = 1; i <= currSampleNum; i++) {
    char sampleData[MAX_NAME_LENGTH * MAX_PAIRS] = "";
    for (int j = 0; j < numUniques; j++) {
        strcat(sampleData, uniqueNames[j]);
        strcat(sampleData, "=");
        strcat(sampleData, samples[i].currSample[j]);
        if (j < numUniques - 1) {
            strcat(sampleData, ", ");
        }
    }

    sem_wait(&write_shared_data->sem_empty);

    sem_wait(&write_shared_data->sem_mutex);
    sprintf(write_shared_data->buffer[write_shared_data->in], "%s", sampleData);
    printf("Reconstruct write to buffer: %s\n", sampleData);
    write_shared_data->in = (write_shared_data->in + 1) % write_shared_data->size;
    sem_post(&write_shared_data->sem_mutex);
    sem_post(&write_shared_data->sem_full);
}
    // Change input done flag
    write_shared_data->input_done = 1;

    printf("Reconstruct: end of input reached. Detaching from shared memory...\n");
    free(nameValPairs);
    free(lastValues);
    free(seenInCurrentSample);
    free(uniqueNames);
    free(samples);
    // Detach from shared memory
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
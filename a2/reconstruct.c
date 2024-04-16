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
#define MAX_SAMPLES 1000
#define MAX_STRING_SIZE 2056
#define MAX_BUFFER_SIZE 50
#define EMPTY "EMPTY"
#define DELAY 1E4
typedef struct {
    char name[MAX_NAME_LENGTH];
    char value[MAX_VALUE_LENGTH];
} NameVal;

typedef struct {
    char currSample[MAX_PAIRS][MAX_NAME_LENGTH];
    int nextEmpty;
} Sample;

// 4-slot definitions
typedef char data_t[MAX_STRING_SIZE];
typedef enum {bit0=0, bit1=1} bit;

// Shared data struct
typedef struct {
    // Ring buffer variables, flag for ending, and semaphores
    char buffer[MAX_BUFFER_SIZE][MAX_STRING_SIZE];
    int in;
    int out;
    int size;
    sem_t sem_empty;
    sem_t sem_full;
    sem_t sem_mutex;
    int input_done;
    // 4-slot buffer variables
    bit latest;
    bit reading;
    data_t buffer_4slot[2][2];
    bit slot[2];
} shared_data_t;

// 4-slot buffer functions
void recBufwrite (shared_data_t *shared_data, char* item) {
    bit pair, index;
    pair = !shared_data->reading;
    index = !shared_data->slot[pair];
    strcpy(shared_data->buffer_4slot[pair][index], item);
    shared_data->slot[pair] = index;
    shared_data->latest = pair;
}

char* recBufread (shared_data_t *shared_data) {
    bit pair, index;
    pair = shared_data->latest;
    shared_data->reading = pair;
    index = shared_data->slot[pair];
    return (shared_data->buffer_4slot[pair][index]);
}
typedef struct {
    char *shm_id_str_read;
    char *shm_id_str_write;
    char *input_file;
    char *buffer_type;
    char *argn;
} fn_args;
void reconstruct(void* input) {
    fn_args *input_args = (fn_args*) input;
    int read_shm_id = atoi(input_args->shm_id_str_read);
    int write_shm_id = atoi(input_args->shm_id_str_write);
    char *buffer_type = input_args->buffer_type;
    
    shared_data_t *read_shared_data = (shared_data_t *)shmat(read_shm_id, NULL, 0);
    if ((void *)read_shared_data == (void *)-1) {
        perror("shmat");
        return;
    }

    shared_data_t *write_shared_data = (shared_data_t *)shmat(write_shm_id, NULL, 0);
    if ((void *)write_shared_data == (void *)-1) {
        perror("shmat");
        return;
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

        // Initialize name and value
        char name[MAX_NAME_LENGTH];
        char value[MAX_VALUE_LENGTH];

        // Ring buffer read process begins
        if(strcmp(buffer_type, "sync") == 0){
            // Sleep for a tiny bit before checking the flag, otherwise we could
            // get stuck if we wait for the semaphore after input_done is set
            usleep(1000);
            int empty;
            // Check if we're finished processing
            sem_getvalue(&read_shared_data->sem_full, &empty);
            if (empty == 0 && read_shared_data->input_done) {
                break;
            }
            // Wait for full semaphore and mutex
            sem_wait(&read_shared_data->sem_full);
            sem_wait(&read_shared_data->sem_mutex);

            // Read from the buffer
            char *data = read_shared_data->buffer[read_shared_data->out];
            // Parse the string with sscanf and assign name & value
            if (sscanf(data, "%[^=]=%[^\n]", name, value) == 2) {
                printf("Reconstruct read from ring buffer: Name=%s, Value=%s\n", name, value);
                strcpy(nameValPairs[numPairs].name, name);
                strcpy(nameValPairs[numPairs].value, value);
                numPairs++;
                // If this is the first name, or it's different from the last
                if (numUniques == 0 || strcmp(name, prevChar) != 0) {
                    // If it's the first, just add it
                    if (numUniques == 0) {
                        strcpy(headName, name);
                        strcpy(uniqueNames[numUniques], name);
                        numUniques++;
                        strcpy(prevChar, name);
                    // If we've reached the headname again and have no endname, set it
                    } else if (strcmp(name, headName) == 0 && strcmp(endName, "") == 0) {
                        strcpy(endName, prevChar);
                    // Otherwise,
                    } else {
                        // Set duplicate flag
                        int duplicate = 0;
                        // For every pair, check if there's a duplicate and set the flag
                        for (int j = 0; j < numPairs - 1; j++) {
                            if (strcmp(nameValPairs[j].name, name) == 0) {
                                duplicate = 1;
                                break;
                            }
                        }
                        // If there's no duplicate, add to uniques and keep track of most recent unique encountered
                        if (duplicate == 0) {
                            strcpy(uniqueNames[numUniques], name);
                            numUniques++;
                            strcpy(prevChar, name);
                        }
                    }
                }
                // Move to the next position in the buffer
                read_shared_data->out = (read_shared_data->out + 1) % read_shared_data->size;
                // Signal mutex
                sem_post(&read_shared_data->sem_mutex);
            }
            // Signal empty semaphore
            sem_post(&read_shared_data->sem_empty);
        // If it's not sync, then it's 4-slot
        } else{
            // Read from the 4-slot buffer
            char *data = recBufread(read_shared_data);
            for (int j = 0; j < DELAY; j++){
                    int success = sched_yield();
                    if (success != 0) {
                        perror("sched_yield");
                    }
            }
            // Check if all data has been processed by checking if we read "input_done"
            if (strcmp(data, "observe_input_done") == 0) {
                break;
            }
            // Parse the string with sscanf and assign name and value
            if (sscanf(data, "%[^=]=%[^\n]", name, value) == 2) {
                printf("Reconstruct read from 4-slot buffer: Name=%s, Value=%s\n", name, value);
                strcpy(nameValPairs[numPairs].name, name);
                strcpy(nameValPairs[numPairs].value, value);
                numPairs++;
                // If this is the first name, or it's different from the last
                if (numUniques == 0 || strcmp(name, prevChar) != 0) {
                    // If it's the first, just add it
                    if (numUniques == 0) {
                        strcpy(headName, name);
                        strcpy(uniqueNames[numUniques], name);
                        numUniques++;
                        strcpy(prevChar, name);
                    // If we've reached the headname again and have no endname, set it
                    } else if (strcmp(name, headName) == 0 && strcmp(endName, "") == 0) {
                        strcpy(endName, prevChar);
                    // Otherwise,
                    } else {
                        // Set duplicate flag
                        int duplicate = 0;
                        // For every pair, check if there's a duplicate and set the flag
                        for (int j = 0; j < numPairs - 1; j++) {
                            if (strcmp(nameValPairs[j].name, name) == 0) {
                                duplicate = 1;
                                break;
                            }
                        }
                        // If there's no duplicate, add to uniques and keep track of most recent unique encountered
                        if (duplicate == 0) {
                            strcpy(uniqueNames[numUniques], name);
                            numUniques++;
                            strcpy(prevChar, name);
                        }
                    }
                }
            }
        }
    }

    // Print the top row of the table, with Sample # and all unique names
    printf("%-20s", "Sample #");
    for (int i = 0; i < numUniques; i++) {
        printf(" %-20s", uniqueNames[i]);
    }
    printf("%s", "\n");

    // Initialize the last encountered values for each name + seen in current sample
    char (*lastValues)[MAX_VALUE_LENGTH] = malloc(MAX_PAIRS * sizeof(*lastValues));
    int *seenInCurrentSample = malloc(MAX_PAIRS * sizeof(int));

    memset(lastValues, 0, MAX_PAIRS * sizeof(*lastValues));
    memset(seenInCurrentSample, 0, MAX_PAIRS * sizeof(int));

    // Initialize current sample and samples
    int currSampleNum = 0;
    Sample* samples = (Sample*) malloc(MAX_SAMPLES * sizeof(Sample));
    memset(samples, 0, MAX_SAMPLES * sizeof(Sample));

    // For all pairs
    for (int i = 0; i < numPairs; i++) {

        // Initialize name and value
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
            memset(seenInCurrentSample, 0, MAX_PAIRS * sizeof(int));
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
    // Print out the rows for the table, with the current sample number and sample data
    for (int i = 1; i <= currSampleNum; i++) {
        printf("%-20d", i);
        for (int j = 0; j < numUniques; j++) {
            printf(" %-20s", samples[i].currSample[j]);
        }
        printf("%s", "\n");
    }

    // For every sample
    for (int i = 1; i <= currSampleNum; i++) {
        // Intialize sample data array
        char sampleData[MAX_NAME_LENGTH * MAX_PAIRS] = "";
        // Construct sampleData for writing in desired format
        for (int j = 0; j < numUniques; j++) {
            strcat(sampleData, uniqueNames[j]);
            strcat(sampleData, "=");
            strcat(sampleData, samples[i].currSample[j]);
            // If it's not last, add a comma and a space
            if (j < numUniques - 1) {
                strcat(sampleData, ", ");
            }
        }
        // If it's synchronous, start ring buffer write process
        if(strcmp(buffer_type, "sync") == 0){
            // Wait for empty and mutex semaphores
            sem_wait(&write_shared_data->sem_empty);
            sem_wait(&write_shared_data->sem_mutex);
            // Write to buffer and print out what we wrote
            sprintf(write_shared_data->buffer[write_shared_data->in], "%s", sampleData);
            printf("Reconstruct write to buffer: %s\n", sampleData);
            // Adjust in value
            write_shared_data->in = (write_shared_data->in + 1) % write_shared_data->size;
            // Signal mutex and full semaphores
            sem_post(&write_shared_data->sem_mutex);
            sem_post(&write_shared_data->sem_full);
        }
        // If it's asynchronous, start 4-slot buffer write process
        else{
            recBufwrite(write_shared_data, sampleData);
            for (int j = 0; j < DELAY; j++){
                    int success = sched_yield();
                    if (success != 0) {
                        perror("sched_yield");
                    }
            }
            printf("Reconstruct write to 4-slot buffer: %s\n", sampleData);
        }
    }

    // Signal input done, with either writing a special string for 4-slot or setting the flag for ring
    if(strcmp(buffer_type, "async") == 0){
        recBufwrite(write_shared_data, "reconstruct_input_done");
        for (int j = 0; j < DELAY; j++){
                int success = sched_yield();
                if (success != 0) {
                    perror("sched_yield");
                }
        }
    } else{
        write_shared_data->input_done = 1;
    }

    // Free everything we allocated on the heap (I adjusted all the large local values to do this because stack overflow was an issue)
    free(nameValPairs);
    free(lastValues);
    free(seenInCurrentSample);
    free(uniqueNames);
    free(samples);

    // Detach from shared memory
    if (shmdt(read_shared_data) == -1) {
        perror("shmdt");
        return;
    }
    if (shmdt(write_shared_data) == -1) {
        perror("shmdt");
        return;
    }
    // Exit
    return;
}
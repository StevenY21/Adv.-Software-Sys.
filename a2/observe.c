#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <getopt.h>
#include <semaphore.h>

#define MAX_LINE_LENGTH 2056
#define MAX_NAME_VALUE_PAIRS 1000
#define MAX_STRING_SIZE 2056
#define MAX_BUFFER_SIZE 50
#define EMPTY "EMPTY"

// Define a struct for name value pairs
typedef struct {
    char name[MAX_LINE_LENGTH];
    char value[MAX_LINE_LENGTH];
} NameValuePair;

// Initialize pair array and pairCount variable
NameValuePair pairs[MAX_NAME_VALUE_PAIRS];
int pairCount = 0;

// 4-slot definitions
typedef char data_t[MAX_STRING_SIZE];
typedef enum {bit0=0, bit1=1} bit;

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
typedef struct {
    char *shm_id_str_read;
    char *shm_id_str_write;
    char *input_file;
    char *buffer_type;
    char *argn;
} fn_args;
// findPair, function to search for matching pair with name
int findPair(char* name) {
    for (int i = 0; i < pairCount; i++) {
        if (strcmp(pairs[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// 4-slot buffer functions
void obsBufWrite (shared_data_t *shared_data, char* item) {
    bit pair, index;
    pair = !shared_data->reading;
    index = !shared_data->slot[pair];
    strcpy(shared_data->buffer_4slot[pair][index], item);
    shared_data->slot[pair] = index;
    shared_data->latest = pair;
}

char* obsBufread (shared_data_t *shared_data) {
    bit pair, index;
    pair = shared_data->latest;
    shared_data->reading = pair;
    index = shared_data->slot[pair];
    return (shared_data->buffer_4slot[pair][index]);
}

void observe(void* input) {
    // Initializing our variables, the file, the line, and shared memory id
    FILE *file;
    fn_args *input_args = (fn_args*) input;
    char *in_file = input_args->input_file;
    char line[MAX_LINE_LENGTH];
    int shm_id = atoi(input_args->shm_id_str_write);
    char *buffer_type = input_args->buffer_type;

    // Attach to shared memory
    shared_data_t *shared_data = (shared_data_t *)shmat(shm_id, NULL, 0);
    if ((void *)shared_data == (void *)-1) {
        perror("shmat");
        return;
    }
    // If there's a filename, open it for reading, otherwise set to stdin
    if (in_file != NULL) {
        file = fopen(in_file, "r");
        if (file == NULL) {
            printf("Could not open file %s\n", in_file);
            return;
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
                // If async, use 4-slot write
                if (strcmp(buffer_type, "async") == 0) {
                    char pair[MAX_STRING_SIZE];
                    snprintf(pair, MAX_STRING_SIZE, "%s=%s", name, value);
                    obsBufWrite(shared_data, pair);
                    printf("Observe write to 4-slot buffer: %s\n", pair);
                // If sync, use ring buffer write
                } else{
                    sem_wait(&shared_data->sem_empty);
                    sem_wait(&shared_data->sem_mutex);
                    snprintf(shared_data->buffer[shared_data->in], MAX_STRING_SIZE, "%s=%s", name, value);
                    printf("Observe write to ring buffer: %s\n", shared_data->buffer[shared_data->in]);
                    shared_data->in = (shared_data->in + 1) % shared_data->size;
                    sem_post(&shared_data->sem_mutex);
                    sem_post(&shared_data->sem_full);
                }
            } else {
                // If there is a match, check if the value differs
                // If it differs, update and write
                if (strcmp(pairs[index].value, value) != 0) {
                    // If async, use 4-slot write
                    if (strcmp(buffer_type, "async") == 0) {
                        char pair[MAX_STRING_SIZE];
                        snprintf(pair, MAX_STRING_SIZE, "%s=%s", name, value);
                        obsBufWrite(shared_data, pair);
                        printf("Observe write to 4-slot buffer: %s\n", pair);
                    // If sync, use ring buffer write
                    } else{
                        strcpy(pairs[index].value, value);
                        sem_wait(&shared_data->sem_empty);
                        sem_wait(&shared_data->sem_mutex);
                        snprintf(shared_data->buffer[shared_data->in], MAX_STRING_SIZE, "%s=%s", name, value);
                        printf("Observe write to ring buffer: %s\n", shared_data->buffer[shared_data->in]);
                        shared_data->in = (shared_data->in + 1) % shared_data->size;
                        sem_post(&shared_data->sem_mutex);
                        sem_post(&shared_data->sem_full);
                    }
                }
            }
        }
    }

    // Signal input done, with either writing a special string for 4-slot or setting the flag for ring
    if(strcmp(buffer_type, "async") == 0){
        obsBufWrite(shared_data, "observe_input_done");
    } else{
        shared_data->input_done = 1;
    }
    shared_data->input_done = 1;
    // If we were using file for input, close it
    if (in_file != NULL) {
        fclose(file);
    }
    printf("Observe: end of input reached. Detaching from shared memory...\n");
    // detach from shared memory
    if (shmdt(shared_data) == -1) {
        perror("shmdt");
        return;
    }
    // Exit
    return;
}

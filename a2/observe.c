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

#define MAX_LINE_LENGTH 2056
#define MAX_NAME_VALUE_PAIRS 2056
#define MAX_STRING_SIZE 2056
#define MAX_BUFFER_SIZE 50

// Define a struct for name value pairs
typedef struct {
    char name[MAX_LINE_LENGTH];
    char value[MAX_LINE_LENGTH];
} NameValuePair;

// Initialize pair array and pairCount variable
NameValuePair pairs[MAX_NAME_VALUE_PAIRS];
int pairCount = 0;

typedef struct {
    char buffer[MAX_BUFFER_SIZE][MAX_STRING_SIZE];   // Shared buffer
    int in;        // Index for inserting into the buffer
    int out;       // Index for removing from the buffer
    int size;      // Size of the buffer
    int input_done;
    pthread_mutex_t mutex;		/* Mutex for shared buffer.                        */
    pthread_cond_t empty_cond;         /* Condition when >= 1 full buffer slots.          */
    pthread_cond_t full_cond;            /* Condition when at least 1 empty mailbox slot.   */   
} shared_data_t;

typedef struct {
    char *shm_id_str_read;
    char *shm_id_str_write;
    char *input_file;
    char *buffer_type;
    char *argn;
} fn_args; // all of the arguments needed for observe, reconstruct, and tapplot

// findPair, function to search for matching pair with name
int findPair(char* name) {
    for (int i = 0; i < pairCount; i++) {
        if (strcmp(pairs[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}
// Function to check if the buffer is full from lecture
int isFull(shared_data_t *shared_data) {
    return (shared_data->in + 1) % shared_data->size == shared_data->out;
}

// Function to check if the buffer is empty from lecture
int isEmpty(shared_data_t *shared_data) {
    return shared_data->in == shared_data->out;
}
void observe(void *input) {
    printf("%s", "observing time\n");
    fn_args *input_args = (fn_args*) input;
    FILE *file;
    char line[MAX_LINE_LENGTH];
    printf("%s", "getting shm_id\n");
    int shm_id = atoi(input_args->shm_id_str_write);
    fprintf(stderr,"Attaching to shared memory segment ID: %d\n", shm_id);
    // Attach to shared memory
    shared_data_t *shared_data = (shared_data_t *)shmat(shm_id, NULL, 0);
    if ((void *)shared_data == (void *)-1) {
        perror("Observe shmat");
        return;
    }
    pthread_mutex_lock (&shared_data->mutex);
    // If there's a filename, open it for reading, otherwise set to stdin
    if (input_args->input_file != NULL) {
        file = fopen(input_args->input_file, "r");
        if (file == NULL) {
            printf("Could not open file %s\n", input_args->input_file);
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
                while(isFull(shared_data)){
                    // wait until the buffer is not full before writing
                    pthread_cond_wait (&shared_data->full_cond, &shared_data->mutex);
                }
                sprintf(*(shared_data->buffer + shared_data->in), "%s=%s", name, value);
                printf("Buffer: %s\n", *(shared_data->buffer + shared_data->in));
                shared_data->in = (shared_data->in + 1) % shared_data->size;
            } else {
                // If there is a match, check if the value differs
                if (strcmp(pairs[index].value, value) != 0) {
                    // If it differs, update and write
                    strcpy(pairs[index].value, value);
                    while(isFull(shared_data)){
                        printf("Observe: buffer is full.\n");
                    }
                    sprintf(*(shared_data->buffer + shared_data->in), "%s=%s", name, value);
                    printf("Buffer: %s\n", *(shared_data->buffer + shared_data->in));
                    shared_data->in = (shared_data->in + 1) % shared_data->size;
                }
            }
        }
    }
    // If we were using file for input, close it
    if (input_args->input_file != NULL) {
        fclose(file);
    }
    pthread_mutex_unlock (&shared_data->mutex);
    printf("Observe: end of input reached. Detaching from shared memory...\n");
    // detach from shared memory
    if (shmdt(shared_data) == -1) {
        perror("Observe shmdt");
        return;
    }
    fprintf(stderr,"%s\n", "exiting");
    // Exit
    return;
}

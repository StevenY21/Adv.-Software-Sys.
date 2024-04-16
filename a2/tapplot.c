#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <getopt.h>
#include <semaphore.h>
#include <assert.h>

#define MAX_STRING_SIZE 2056
#define MAX_BUFFER_SIZE 50
#define EMPTY "EMPTY"

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
typedef struct {
    char *shm_id_str_read;
    char *shm_id_str_write;
    char *input_file;
    char *buffer_type;
    char *argn;
} fn_args;
// 4-slot buffer functions
void bufwrite (shared_data_t *shared_data, char* item) {
    bit pair, index;
    pair = !shared_data->reading;
    index = !shared_data->slot[pair];
    strcpy(shared_data->buffer_4slot[pair][index], item);
    shared_data->slot[pair] = index;
    shared_data->latest = pair;
}

char* bufread (shared_data_t *shared_data) {
    bit pair, index;
    pair = shared_data->latest;
    shared_data->reading = pair;
    index = shared_data->slot[pair];
    return (shared_data->buffer_4slot[pair][index]);
}

void tapplot(void *input) {
    FILE *fp;
    fn_args *input_args = (fn_args*) input;
    int shm_id = atoi(input_args->shm_id_str_read);
    char *buffer_type = input_args->buffer_type;
    int argn = atoi(input_args->argn);

    // Attach to the shared memory segment
    shared_data_t *shared_data = (shared_data_t *)shmat(shm_id, NULL, 0);
    if ((void *)shared_data == (void *)-1) {
        perror("shmat");
        return;
    }

    // Open pipe to helper.pl
    fp = popen("./helper.pl", "w");
    if (fp == NULL) {
        perror("popen");
        return;
    }

    // Loop to read from shared buffer and write to pipe
    while (1) {
        // Move data definition to before the ifs, so we can access it after it's set
        char *data;
        // If buffer type is sync, initialize ring buffer read process
        if(strcmp(buffer_type, "sync") == 0){
            // Sleep for a tiny bit before checking the flag, otherwise we could
            // get stuck if we wait for the semaphore after input_done is set
            usleep(1000);

            // Check if we're finished processing
            int empty;
            sem_getvalue(&shared_data->sem_full, &empty);
            if (empty == 0 && shared_data->input_done) {
                break;
            }
            
            // Wait for the full semaphore
            sem_wait(&shared_data->sem_full);

            // Acquire the mutex to access the shared buffer
            sem_wait(&shared_data->sem_mutex);

            // Read data from the buffer
            data = shared_data->buffer[shared_data->out];
            printf("Tapplot read from ring buffer: %s\n", data);
            shared_data->out = (shared_data->out + 1) % shared_data->size;

            // Release the mutex and signal empty semaphore
            sem_post(&shared_data->sem_mutex);
            sem_post(&shared_data->sem_empty);

        // If buffer type is async, enter async read process
        } else{
            // Read from the 4-slot buffer
            data = bufread(shared_data);
            // Check if all data has been processed by checking if we read "input_done"
            if (strcmp(data, "reconstruct_input_done") == 0) {
                break;
            }
        // Tapplot reads way faster than reconstruct writes, so I'll omit this for now to avoid muddying up the output
        // Could be uncommented if needed for demo purposes
            if(strcmp(data, "EMPTY") != 0){
                //printf("Tapplot read from 4-slot buffer: %s\n", data);
            }
        }

        // Parse data and write to the pipe
        char *token = strtok(data, ",");
        int field_index = 0;
        char *plot_data = NULL;
        while (token != NULL) {
            if (field_index == argn - 1) {
                plot_data = token;
                break;
            }
            token = strtok(NULL, ",");
            field_index++;
        }

        // Write data to the pipe
        if (plot_data != NULL) {
            fprintf(fp, "%s\n", plot_data);
            fflush(fp);
        }
    }

    // Close the pipe
    pclose(fp);
    // Detach shared memory
    if (shmdt(shared_data) == -1) {
        perror("shmdt");
        return;
    }
    // Exit
    return;
}
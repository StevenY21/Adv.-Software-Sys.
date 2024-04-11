#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <getopt.h>
#include <semaphore.h>

// Structure for shared data
typedef struct {
    char **buffer;   // Shared buffer
    int in;        // Index for inserting into the buffer
    int out;       // Index for removing from the buffer
    int size;      // Size of the buffer
    sem_t sem_empty;
    sem_t sem_full;
    int input_done;
} shared_data_t;

// Function to check if the buffer is full from lecture
int isFull(shared_data_t *shared_data) {
    return (shared_data->in + 1) % shared_data->size == shared_data->out;
}

// Function to check if the buffer is empty from lecture
int isEmpty(shared_data_t *shared_data) {
    return shared_data->in == shared_data->out;
}

int main(int argc, char *argv[]) {
    int shm_id = atoi(argv[1]);
    int argn = argc > 3 ? atoi(argv[3]) : 1;

    shared_data_t *shared_data = (shared_data_t *)shmat(shm_id, NULL, 0);
    if ((void *)shared_data == (void *)-1) {
        perror("shmat");
        return 1;
    }

    // Loop to read from shared buffer and plot
    while (1) {
        sem_wait(&shared_data->sem_full);
        char *data = shared_data->buffer[shared_data->out];
        shared_data->out = (shared_data->out + 1) % shared_data->size;
        sem_post(&shared_data->sem_empty);

        // Parse data to extract the required field based on argn
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

        // Plot the data using helper.py script
        if (plot_data != NULL) {
            char arg_buffer[50];
            snprintf(arg_buffer, sizeof(arg_buffer), "%d", atoi(plot_data));
            execlp("python3", "python3", "helper.py", arg_buffer, NULL);
            perror("execlp");
            exit(EXIT_FAILURE);
        }
        int empty;
        sem_getvalue(&shared_data->sem_full, &empty);
        printf("Value of empty: %d, Value of input done: %d\n", empty, shared_data->input_done);
        if (empty == 0 && shared_data->input_done) {
            break;
        }
    }

    // Detach shared memory
    if (shmdt(shared_data) == -1) {
        perror("shmdt");
        return 1;
    }

    return 0;
}

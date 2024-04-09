#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>

#define MAX_STRING_SIZE 2056
#define MAX_PROCESSES 100
// Structure for shared data
typedef struct {
    char **buffer;   // Shared buffer
    int in;        // Index for inserting into the buffer
    int out;       // Index for removing from the buffer
    int size;      // Size of the buffer
} shared_data_t;

int main(int argc, char *argv[]) {
    int i;
    int c;
    char *buffer_size = "1\0";
    char *buffer_type = NULL;
    char *last_program = NULL;
    char *first_program = NULL;
    char *argn = "1\0";
    int num_threads = 0;
    char *input_file = NULL;
    char *program_names[MAX_PROCESSES];

    while ((c = getopt(argc, argv, "b:s:p:")) != -1) {
        switch (c) {
            case 'b':
                buffer_type = optarg;
                break;
            case 's':
                buffer_size = optarg;
                break;
            case 'p':
                if(optind < argc) {
                    program_names[num_threads] = argv[optind];
                    num_threads++;
                    if(num_threads == 1){
                        first_program = argv[optind];
                    }
                    last_program = argv[optind];
                    // if there's an input filename given after observe, get it
                    if (strcmp(optarg, "observe") == 0 && optind < argc && argv[optind][0] != '-') {
                        input_file = argv[optind];
                        optind++;
                    }
                    break;
                }
        }
    }
    if (last_program != NULL) {
        for (int i = optind; i < argc; i++) {
            if (strcmp(argv[i], last_program) == 0 && i + 1 < argc) {
                argn = argv[i + 1];
                break;
            }
        }
    }

    int shm_ids[num_threads];
    shared_data_t *shared_data[num_threads];
    
    for (i = 0; i < num_threads; i++) {
        shm_ids[i] = shmget(IPC_PRIVATE, sizeof(shared_data_t), IPC_CREAT | 0666);
        if (shm_ids[i] < 0) {
            perror("shmget");
            exit(1);
        }

        shared_data[i] = (shared_data_t *)shmat(shm_ids[i], NULL, 0);
        if ((void *)shared_data[i] == (void *)-1) {
            perror("shmat");
            exit(1);
        }
        printf("Shared memory segment %d created with ID %d\n", i, shm_ids[i]);

        shared_data[i]->in = 0;
        shared_data[i]->out = 0;
        shared_data[i]->size = atoi(buffer_size);
        // Allocate memory for the buffer
        shared_data[i]->buffer = (char **)malloc(shared_data[i]->size * sizeof(char *));
        if (shared_data[i]->buffer == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory for buffer\n");
            exit(EXIT_FAILURE);
        }

        // Initialize each element of the buffer
        for (int j = 0; j < shared_data[i]->size; j++) {
            shared_data[i]->buffer[j] = (char *)malloc(MAX_STRING_SIZE * sizeof(char));
            if (shared_data[i]->buffer[j] == NULL) {
                fprintf(stderr, "Error: Failed to allocate memory for buffer element %d\n", j);
                exit(EXIT_FAILURE);
            }
        }

        // Print debug information
        printf("Buffer allocated and initialized successfully:\n");
        printf("Buffer size: %d\n", shared_data[i]->size);
        for (int j = 0; j < shared_data[i]->size; j++) {
            printf("Buffer element %d: %p\n", j, (void *)shared_data[i]->buffer[j]);
        }
        char shm_id_str_read[10];
        char shm_id_str_write[10];
        if (fork() == 0) {
            if (strcmp(program_names[i], last_program) == 0) {
                // if last, read from previous
                sprintf(shm_id_str_read, "%d", shm_ids[i - 1]);
                fprintf(stderr, "Executing: %s\n", last_program);
                execlp(last_program, last_program, shm_id_str_read, buffer_type, argn, NULL);
            } else if(strcmp(program_names[i], first_program) == 0){
                // if first, write to current
                sprintf(shm_id_str_write, "%d", shm_ids[i]);
                fprintf(stderr, "Executing: %s\n", first_program);
                execlp(first_program, first_program, shm_id_str_write, input_file, buffer_type, NULL);
            } else {
                // if middle, then read from previous and write to current
                sprintf(shm_id_str_read, "%d", shm_ids[i - 1]);
                sprintf(shm_id_str_write, "%d", shm_ids[i]);
                fprintf(stderr, "Executing: %s\n", program_names[i]);
                execlp(program_names[i], program_names[i], shm_id_str_read, shm_id_str_write, buffer_type, NULL);
            }
            perror("execlp");
            exit(1);
        }
    }

    for (i = 0; i < num_threads; i++) {
        wait(NULL);
    }

    for (i = 0; i < num_threads; i++) {
        for (int j = 0; j < shared_data[i]->size; j++) {
            free(shared_data[i]->buffer[j]);
        }
        free(shared_data[i]->buffer);
        if (shmdt(shared_data[i]) == -1) {
            perror("shmdt");
            exit(1);
        }
        if (shmctl(shm_ids[i], IPC_RMID, NULL) == -1) {
            perror("shmctl");
            exit(1);
        }
    }
    return 0;
}


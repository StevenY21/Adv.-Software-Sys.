#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <getopt.h>
#include <string.h>
#include <semaphore.h>

#define MAX_STRING_SIZE 2056
#define MAX_PROCESSES 100
#define MAX_BUFFER_SIZE 50

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

int main(int argc, char *argv[]) {
    int i;
    int c;
    char *buffer_size = "1\0";
    char *buffer_type = NULL;
    char *last_program = NULL;
    char *first_program = NULL;
    char *argn = "1\0";
    int num_processes = 0;
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
                    program_names[num_processes] = argv[optind];
                    num_processes++;
                    if(num_processes == 1){
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

    int num_pairs = num_processes - 1;

    int shm_ids[num_pairs];
    shared_data_t *shared_data[num_pairs];

    for (i = 0; i < num_pairs; i++) {
        size_t shm_size = sizeof(shared_data_t);
        printf("Shared memory size: %ld\n", shm_size);
        shm_ids[i] = shmget(IPC_PRIVATE, shm_size, IPC_CREAT | 0666);
        if (shm_ids[i] < 0) {
            perror("shmget");
            exit(1);
        }

        shared_data[i] = (shared_data_t *)shmat(shm_ids[i], NULL, 0);
        if ((void *)shared_data[i] == (void *)-1) {
            perror("shmat");
            exit(1);
        }
        printf("Shared memory segment %d created with ID %d at location %p\n", i, shm_ids[i], shared_data[i]);
        shared_data[i]->in = 0;
        shared_data[i]->out = 0;
        shared_data[i]->size = atoi(buffer_size);
        shared_data[i]->input_done = 0;
        printf("Tapper assigned buffer address: %p\n", shared_data[i]->buffer);
        // Initialize the empty and full semaphores
        sem_init(&shared_data[i]->sem_empty, 1, shared_data[i]->size);
        sem_init(&shared_data[i]->sem_full, 1, 0);
        sem_init(&shared_data[i]->sem_mutex, 1, 1);
    }

    for (i = 0; i < num_processes; i++) {
        char shm_id_str_read[10];
        char shm_id_str_write[10];
        if (fork() == 0) {
            if (i == 0) {
                sprintf(shm_id_str_write, "%d", shm_ids[i]);
                fprintf(stderr, "Executing: %s\n", first_program);
                execlp(first_program, first_program, shm_id_str_write, input_file, buffer_type, NULL);
            } else if (i == num_processes - 1) {
                sprintf(shm_id_str_read, "%d", shm_ids[i - 1]);
                fprintf(stderr, "Executing: %s\n", last_program);
                execlp(last_program, last_program, shm_id_str_read, buffer_type, argn, NULL);
            } else {
                sprintf(shm_id_str_read, "%d", shm_ids[i - 1]);
                sprintf(shm_id_str_write, "%d", shm_ids[i]);
                fprintf(stderr, "Executing: %s\n", program_names[i]);
                execlp(program_names[i], program_names[i], shm_id_str_read, shm_id_str_write, buffer_type, NULL);
            }
            perror("execlp");
            exit(1);
        }
    }

    // Wait for all processes to finish
    for (i = 0; i < num_processes; i++) {
        wait(NULL);
    }

    // Detach and remove shared memory segments
    for (i = 0; i < num_pairs; i++) {
        if (shmdt(shared_data[i]) == -1) {
            perror("shmdt");
            exit(1);
        }
        if (shmctl(shm_ids[i], IPC_RMID, NULL) == -1) {
            perror("shmctl");
            exit(1);
        }
        sem_destroy(&shared_data[i]->sem_empty);
        sem_destroy(&shared_data[i]->sem_full);
        sem_destroy(&shared_data[i]->sem_mutex);
    }

    return 0;
}

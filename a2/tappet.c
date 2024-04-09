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
#define MAX_THREADS 100
#define BUFFER_SIZE   2
#define MAX_PRODUCERS 10
typedef struct {
    char **buffer;   // Shared buffer
    int in;        // Index for inserting into the buffer
    int out;       // Index for removing from the buffer
    int size;      // Size of the buffer
} shared_data_t;
typedef struct {
  char *data;			/* Slot data.                            */
  size_t size;			/* Size of data.                         */
  pthread_t id;			/* ID of destination thread.             */
} slot_t;
typedef struct {
    char *shm_id_str_read;
    char *shm_id_str_write;
    char *input_file;
    char *buffer_type;
    char *argn;
} fn_args; // all of the arguments needed for observe, reconstruct, and tapplot
int main(int argc, char *argv[]) {
    char *task_names[MAX_THREADS];
    int num_tasks;
    char *buffer_type = NULL;
    int buffer_size = NULL;
    char *argn = "1\0";
    char *input_file = NULL;
    char *last_program = NULL;
    int c;
    while ((c = getopt (argc, argv, "b:s:p:")) != -1) {// checking the options and sending errors if invalid options found
        switch (c)
        {
        case 'p':
            if(optind < argc) {
                task_names[MAX_THREADS] = argv[optind];
                num_tasks++;
                last_program = argv[optind];
                // if there's an input filename given after observe, get it
                if (strcmp(optarg, "observe") == 0 && optind < argc && argv[optind][0] != '-') {
                    input_file = argv[optind];
                    optind++;
                }
                break;
            }
        case 'b':
            buffer_type = optarg;
        case 's':
            buffer_size = atoi(optarg);
            break;
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
    int shm_ids[num_tasks];
    shared_data_t *shared_data[num_tasks];
    pthread_t thread_arr[num_tasks];
    for (int i = 0; i < num_tasks; i++) {
        size_t shm_size = sizeof(shared_data_t) + buffer_size * MAX_STRING_SIZE;
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
        printf("Shared memory segment %d created with ID %d\n", i, shm_ids[i]);

        shared_data[i]->in = 0;
        shared_data[i]->out = 0;
        shared_data[i]->size = buffer_size;

        // Allocate memory for the buffer within the shared memory segment
        shared_data[i]->buffer = (char **)((char *)shared_data[i] + sizeof(shared_data_t));

        // Initialize each element of the buffer
        for (int j = 0; j < shared_data[i]->size; j++) {
            shared_data[i]->buffer[j] = (char *)(shared_data[i]->buffer + shared_data[i]->size) + j * MAX_STRING_SIZE;
        }
        fn_args *arguments;
        char shm_id_str_read[10];
        char shm_id_str_write[10];
        arguments->buffer_type = buffer_type;
        arguments->argn = argn;
        void * (*fn) (void *);
        //if (strcmp(task_names[i], "observe") == 0) {
        //    fn = observe;
        //} else if (strcmp(task_names[i], "reconstruct") == 0) {
        //    fn = reconstruct;
        //} else {
        //    fn = tapplot;
        //}
        arguments->shm_id_str_read = NULL;
        arguments->shm_id_str_write = NULL;
        if (strcmp(task_names[i], last_program) == 0) {
            // if last, read from previous
            sprintf(shm_id_str_read, "%d", shm_ids[i - 1]);
            fprintf(stderr, "Executing: %s\n", last_program);
            arguments->shm_id_str_read = shm_id_str_read;
            //execlp(last_program, last_program, shm_id_str_read, buffer_type, argn, NULL);
            if (pthread_create(thread_arr[i], NULL, fn, (void *)arguments) != 0) {
                perror("thread creation failed");
                exit(1);
            }
        } else if(i == 0){ //im assuming i == 0 is the first one
            // if first, write to current
            sprintf(shm_id_str_write, "%d", shm_ids[i]);
            fprintf(stderr, "Executing: %s\n", task_names[i]);
            arguments->shm_id_str_write = shm_id_str_write;
            //execlp(task_names[i], task_names[i], shm_id_str_write, input_file, buffer_type, NULL);
            if (pthread_create(thread_arr[i], NULL, fn, (void *)arguments) != 0) {
                perror("thread creation failed");
                exit(1);
            }
        } else {
            // if middle, then read from previous and write to current
            sprintf(shm_id_str_read, "%d", shm_ids[i - 1]);
            sprintf(shm_id_str_write, "%d", shm_ids[i]);
            fprintf(stderr, "Executing: %s\n", task_names[i]);
            arguments->shm_id_str_write = shm_id_str_write;
            arguments->shm_id_str_read = shm_id_str_read;
            //execlp(task_names[i], task_names[i], shm_id_str_read, shm_id_str_write, buffer_type, NULL);
            if (pthread_create(thread_arr[i], NULL, fn, (void *)arguments) != 0) {
                perror("thread creation failed");
                exit(1);
            }
        }
    }
    // this wait might not be needed
    for (int i = 0; i < num_tasks; i++) {
        wait(NULL);
    }
    for (int i = 0; i < num_tasks; i++) {
        pthread_join (thread_arr[i], NULL);
    }
    for (int i = 0; i < num_tasks; i++) {
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

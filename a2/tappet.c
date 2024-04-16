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
#include <semaphore.h>
#define MAX_STRING_SIZE 2056
#define MAX_BUFFER_SIZE 50
#define MAX_THREADS 100
#define BUFFER_SIZE   2

typedef struct {
    // We have the buffer, the in and out indices, the size, the empty and full semaphores, and the input_done flag
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
    int num_tasks = 0;
    char *buffer_type = NULL;
    int buffer_size = 0;
    char *argn = "1\0";
    char *input_file = NULL;
    char *last_program = NULL;
    int c;
    while ((c = getopt (argc, argv, "b:s:t:")) != -1) {// checking the options and sending errors if invalid options found
        switch (c)
        {
        case 'b':
            buffer_type = optarg;
        case 's':
            buffer_size = atoi(optarg);
            break;
        case 't':
            if(optind < argc) {
                task_names[num_tasks] = argv[optind];
                printf("%s\n", task_names[num_tasks]);
                num_tasks++;
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
    if (last_program != NULL) { //for getting argn value
        for (int i = optind; i < argc; i++) {
            if (strcmp(argv[i], last_program) == 0 && i + 1 < argc) {
                argn = argv[i + 1];
                break;
            }
        }
    }
    printf("buffer type %s\n", buffer_type);
    int num_buffers = num_tasks;
    int shm_ids[num_buffers];
    shared_data_t *shared_data[num_buffers];
    pthread_t task_threads[num_buffers];
    for(int i = 0; i < num_buffers; i++) {
        size_t shm_size = sizeof(shared_data_t);
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
        shared_data[i]->size = buffer_size;
        shared_data[i]->input_done = 0;
        //shared_data[i]->count = 0;
        printf("Tapper assigned buffer address: %p\n", shared_data[i]->buffer);
        //pthread_mutex_init (&shared_data[i]->mutex, NULL);
        //pthread_cond_init (&shared_data[i]->empty_cond, NULL);
        //pthread_cond_init (&shared_data[i]->full_cond, NULL);
        sem_init(&shared_data[i]->sem_empty, 1, shared_data[i]->size);
        sem_init(&shared_data[i]->sem_full, 1, 0);
        sem_init(&shared_data[i]->sem_mutex, 1, 1);
    }
    for (int i = 0; i < num_tasks; i++) {
        fprintf(stderr, "%s", "getting thread args\n");
        fn_args *arguments = (fn_args *)malloc(sizeof(fn_args));
        char shm_id_str_read[10];
        char shm_id_str_write[10];
        arguments->buffer_type = buffer_type;

        arguments->argn = argn;
        //getting lib func
        // don't forget
        /*
        LD_LIBRARY_PATH=/full/path/to/library/directory:${LD_LIBRARY_PATH}
        export LD_LIBRARY_PATH
        
        */
        void (*fn)(void*);
        void *libtap =dlopen("libtap.so", RTLD_LAZY); 
        if (libtap == NULL) {
            fprintf(stderr, "%s\n", "cannot open library");
            exit(1);
        }
        *(void**)(&fn) = dlsym(libtap, task_names[i]);
        if (fn == NULL) {
            fprintf(stderr, "%s\n", "cannot get function");
            exit(1);
        }
        arguments->shm_id_str_read = "";
        arguments->shm_id_str_write = "";
        arguments->input_file = input_file;
        if (i == 0) { //first task, reads from stdin/file and writes to buffer
            sprintf(shm_id_str_write, "%d", shm_ids[i]);
            arguments->shm_id_str_write = shm_id_str_write;
            fprintf(stderr, "Executing first task: %s\n", task_names[i]);
        } else if (i == num_tasks - 1) { // last task, only reads from buffer and writes to stdout
            sprintf(shm_id_str_read, "%d", shm_ids[i - 1]);
            arguments->shm_id_str_read = shm_id_str_read;
            fprintf(stderr, "Executing last task: %s\n", last_program);
        } else { //read from previous buffer, write to next
            sprintf(shm_id_str_read, "%d", shm_ids[i - 1]);
            sprintf(shm_id_str_write, "%d", shm_ids[i]);
            fprintf(stderr, "Executing: %s\n", task_names[i]);
            arguments->shm_id_str_write = shm_id_str_write;
            arguments->shm_id_str_read = shm_id_str_read;
        }
        printf("creating thread %s\n", task_names[i]);
        if (pthread_create(&task_threads[i], NULL, (void *(*)(void *))fn, (void *)arguments) != 0) {
            perror("thread creation failed");
            exit(1);
        }
        sleep(1);
    }
    //not 100% sure if i need
    printf("joining threads %d numtasks\n", num_tasks);
    for(int i = 0; i < num_tasks; i++) {
        printf("joining thread %d\n", i);
        pthread_join(task_threads[i], NULL);
        printf("done joining thread %d\n", i);
    }
    printf("destroying mutexes and conds and segments\n");
    for (int i = 0; i < num_tasks; i++) {
        printf("destroying pthread mutexes and conds for %d\n", i);
        //pthread_mutex_destroy (&shared_data[i]->mutex);
        //pthread_cond_destroy (&shared_data[i]->empty_cond);
        //pthread_cond_destroy (&shared_data[i]->full_cond);
        printf("detaching segments for %d\n", i);
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
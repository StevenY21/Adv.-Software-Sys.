#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_SIZE 10   // Size of the shared memory buffer

// Structure for shared data
typedef struct {
    int buffer[SHM_SIZE];   // Shared buffer
    int in;                  // Index for inserting into the buffer
    int out;                 // Index for removing from the buffer
} shared_data_t;

int main() {
    int shm_id;                  // Shared memory ID
    shared_data_t *shared_data;  // Pointer to shared data

    // Create shared memory segment
    shm_id = shmget(IPC_PRIVATE, sizeof(shared_data_t), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }

    // How to attach shared memory segment?
    /* Attach shared memory segment to shared_data */

    if ((void *)shared_data == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    // Initialize shared data
    shared_data->in = 0;
    shared_data->out = 0;

    // Create producer process
    if (fork() == 0) {
        int value = 0;
        while (1) {
		
	    // Produce data. Add the data to the shared buffer.


            // Sleep for some time
            sleep(1);
        }
    }

    // Create consumer process
    if (fork() == 0) {
        int value;
        while (1) {

            // Consume data. Remove data from shared buffer.

            // Sleep for some time
            sleep(2);
        }
    }

    // Wait for child processes to terminate
    wait(NULL);
    wait(NULL);

    // Detach shared memory segment
    if (shmdt(shared_data) == -1) {
        perror("shmdt");
        exit(1);
    }

    // Delete shared memory segment
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }

    return 0;
}

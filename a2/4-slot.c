/* Copyright Rich West, Boston University, 2024

   This is an example producer-consumer thread program using Simpson's
   4-slot asynchronous buffering.

   It shows that by adjusting the execution times of the producer and
   consumer (effectively changing whether they are suitably
   rate-matched or not) leads to differing amounts of skipped (i.e.,
   lost) data. What is read, however, should remain the most recently
   observed values. 

   This is relevant to understanding how to correctly sample e.g.,
   sensor readings.

   Compile/link with: gcc -o 4-slot-threads 4-slot-threads.c -lpthread
   Test with: 4-slot-threads [integer-delay] > output_file
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_ITER 1000		/* Max iterations to produce/consume
				   data. */

FILE *fp;

/* Start of 4-slot buffering logic. */
typedef int data_t;
# define EMPTY 0xFF

typedef enum {bit0=0, bit1=1} bit;
bit latest = 0, reading = 0;  // Control variables

// Initialize four data slots for items.
data_t buffer[2][2] = {{EMPTY, EMPTY}, {EMPTY, EMPTY}}; 
bit slot[2] = {0, 0};

void bufwrite (data_t item) {
  bit pair, index;
  pair = !reading;
  index = !slot[pair]; // Avoids last written slot in this pair, which
		       // reader may be reading.
  buffer[pair][index] = item; // Copy item to data slot.
  slot[pair] = index; // Indicates latest data within selected pair.
  latest = pair; // Indicates pair containing latest data.
}

data_t bufread (data_t buffer[2][2]) {
  bit pair, index;
  pair = latest;
  reading = pair;
  index = slot[pair];
  return (buffer[pair][index]);
}
/* End of 4-slot buffering logic. */


// Try changing DELAY between 1E4 and 1E8 to see changes in loss.
#define DELAY 1E4
uint delay = DELAY;

/* Write data to 4-slot buffer. */
void put_data (void) {

  for (int i = 0; i < MAX_ITER; i++) {
    bufwrite (i);

    for (int j = 0; j < delay; j++); /* Add some delay. */
    sched_yield ();		     /* Allow another thread to run. */
  }
}

/* Read data from 4-slot buffer. */
void get_data (void) {

  static data_t old_value;
  data_t new_value;
  
  for (int i = 0; i < MAX_ITER; i++) {

    new_value = bufread (buffer);
    fprintf (fp, "Got data: %d\n", new_value); // Write to pipe.
    fflush (fp);
    
    printf ("Got data: %d ", new_value);       // And to stdout.
    fflush (stdout);
    
    if (new_value > old_value + 1)
      printf ("*********** Skipped data **********\n");
    else if (new_value == old_value)
      printf ("========== Unchanged data =========\n");
    else if (new_value < old_value)
      printf ("<<<<<<<<<<< Reverse data <<<<<<<<<<\n");
    else
      printf ("\n");

    old_value = new_value;

    for (int j = 0; j < delay; j++); /* Add some delay. */
    sched_yield ();		     /* Allow another thread to run. */
  }
}


int main (int argc, char *argv[]) {

  pthread_t producer, consumer;

  if (!(argc == 1 || argc == 2)) {
    fprintf (stderr, "Usage: %s [delay (1E4-1E8)]\n", argv[0]);
    exit (1);
  }
  else if (argc == 2) {
    delay = strtod (argv[1], NULL); // Works with command-line args
				    // with 'E' in them.
  }

  /* Create pipe to 4-slot Perl script for parsing data. This in turn
     pipes its output to gnuplot for live plotting. */
  fp = popen ("./4-slot.pl", "w");


  /* Now create a producer thread and a consumer thread that use a
     4-slot bufferto exchange data. */
  if (pthread_create (&producer, NULL,
		      (void *(*)(void *))put_data, NULL) != 0) {
    perror ("Unable to create thread");
    exit (1);
  }
  
  if (pthread_create (&consumer, NULL,
		      (void *(*)(void *))get_data, NULL) != 0) {
    perror ("Unable to create thread");
    exit (1);
  }

  /* Have the main thread wait for for the producer and consumer
     threads to complete their execution. */
  pthread_join (producer, NULL);
  pthread_join (consumer, NULL);

  /* Close the pipe created with popen. */
  pclose (fp);
  return 0;
  
}
/* Copyright Rich West, Boston Universty, 2024

   Simple Pthread example to exchange data between N producers and N
   consumers via a shared circular buffer guarded by mutexes and
   coordinated with condition variables. Guarantees integrity of
   buffered data shared between threads running at their own
   speeds.  */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>

#define BUFFER_SIZE   2
#define MAX_PRODUCERS 10

typedef struct {
  char *data;			/* Slot data.                            */
  size_t size;			/* Size of data.                         */
  pthread_t id;			/* ID of destination thread.             */
} slot_t;

typedef struct {
  slot_t slot[BUFFER_SIZE];
  int count;			/* Number of full slots in buffer.       */
  int in_marker;		/* Next slot to add data to.             */
  int out_marker;		/* Next slot to take data from.          */
  pthread_mutex_t mutex;		/* Mutex for shared buffer.                        */
  pthread_cond_t occupied_slot;         /* Condition when >= 1 full buffer slots.          */
  pthread_cond_t empty_slot;            /* Condition when at least 1 empty mailbox slot.   */    
} buffer_t;

typedef struct{
  pthread_t producer;		/* Producer thread identifier.           */
  pthread_t consumer;		/* Matching consumer thread identifier.  */
} thr_name_t;

buffer_t buffer;

int producers=1;		/* Default.                              */
int total_msgs=1;		/* Default.                              */
thr_name_t thread_table[MAX_PRODUCERS];

pthread_t lookup (pthread_t id);

/* Produce data referenced by `item' of `size' bytes and place in the
   buffer pointed to by `b'. The data is to be delivered to `target' 
   thread.                                                               */   
void produce (buffer_t *b, char *item, size_t size, pthread_t target) {
  
  pthread_mutex_lock (&b->mutex);

  while (b->count >= BUFFER_SIZE)
    pthread_cond_wait (&b->empty_slot, &b->mutex);

  assert (b->count < BUFFER_SIZE);

  b->slot[b->in_marker].data = (char *) malloc (size);
  b->slot[b->in_marker].size = size;
  b->slot[b->in_marker].id   = target;

  //printf("%s: item is:%s", __FUNCTION__, item);

  memcpy (b->slot[b->in_marker].data, item, size);
  b->in_marker++;
  b->in_marker %= BUFFER_SIZE;
  b->count++;

  pthread_cond_broadcast (&b->occupied_slot);
  
  pthread_mutex_unlock (&b->mutex);
  
}

char *consume (buffer_t *b) {

  char *item;
  pthread_t self=pthread_self();

  pthread_mutex_lock (&b->mutex);
  while (b->count <= 0)
    pthread_cond_wait (&b->occupied_slot, &b->mutex);
  
  assert (b->count > 0);

  /* Check id of target thread to see message is for this thread.        */    
  if (!pthread_equal(b->slot[b->out_marker].id, self)) {
    /* Data is not for this thread. */

    //printf ("%s: slot id: %lu, consumer id: %lu\n",
    //	    __FUNCTION__,
    //	    b->slot[b->out_marker].id,
    //	    self);
    pthread_mutex_unlock (&b->mutex);
    
    return NULL;
  }
  
  item = (char *) malloc (b->slot[b->out_marker].size);

  memcpy (item, b->slot[b->out_marker].data, b->slot[b->out_marker].size);
  free (b->slot[b->out_marker].data);
  b->out_marker++;
  b->out_marker %= BUFFER_SIZE;
  b->count--;

  pthread_cond_broadcast (&b->empty_slot);

  pthread_mutex_unlock (&b->mutex);

  return (item);

}


/* Returns the target/consumer thread id for the producer thread whose id 
   is `id'.                                                              */
pthread_t lookup (pthread_t id) {
  
  int i;

  for (i = 0; i < producers; i++)
    if (pthread_equal (thread_table[i].producer, id))
      return (thread_table[i].consumer);
  
  return (-1);
}


void get_data (void) {

  char *data;
  int i = 0;
  
  while (i < total_msgs) {
    while ((data = consume (&buffer)) == NULL) {
      //printf ("%s: Waiting. Buffer count: %d, in: %d, out %d, slot id: %lu, tid: %lu\n",
      //	      __FUNCTION__, buffer.count, buffer.in_marker,
      //      buffer.out_marker,
      //      buffer.slot[buffer.out_marker].id,
      //      pthread_self());
    //sleep (1);
    //sched_yield();
    }
    printf ("My id is %lu: Message reads: %s", pthread_self(), data);
    free (data);
    i++;
  }
}

void put_data (void) {
  
  int i = 0;
  char s[80];
  pthread_t self=pthread_self();
  pthread_t target;

  target = lookup (self);

  while (i < total_msgs) {
    sprintf (s, "Current count is %d, producer is %lu, target is %lu\n", 
	     i, self, target);
    produce (&buffer, s, strlen(s)+1, target);
    i++;
  }
}

void init_buffer (buffer_t *b) {
  
  int i;

  b->in_marker = b->out_marker = b->count = 0;

  for (i = 0; i < BUFFER_SIZE; i++)
    b->slot[i].data = NULL;

  pthread_mutex_init (&b->mutex, NULL);
  pthread_cond_init (&b->occupied_slot, NULL);
  pthread_cond_init (&b->empty_slot, NULL);
}


int main (int argc, char **argv) {
  
  int i;

  static pthread_attr_t tattr; // Thread attributes. Used for controlling scheduling here.
  static struct sched_param tp; 

  if (!(argc == 1 || argc == 3)) {
    fprintf (stderr, "Usage: %s [number_of_producers msgs_per_consumer]\n", argv[0]);
    exit (1);
  }
  else if (argc == 3) {
    producers = atoi (argv[1]);
    total_msgs = atoi (argv[2]);
  }

  init_buffer (&buffer);

  // Prepare thread attributes -- First, the scheduling policy
  if (pthread_attr_setschedpolicy(&tattr, SCHED_RR)) {
    fprintf (stderr, "Cannot set thread scheduling policy!\n");
    exit (1);
  }

  // Now set the priority between 0 and 99
  tp.sched_priority = 50;
  if (pthread_attr_setschedparam(&tattr,  &tp)) {
    fprintf (stderr, "Cannot set thread scheduling priority!\n");
    exit (1);
  }

  // Now set the thread scope to be system-wide
  if (pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM)) {
    fprintf (stderr, "Cannot set thread execution scope!\n");
    exit (1);
  }

  /* Create a thread for every producer, and a corresponding thread
     for every consumer. There should be one consumer thread for every
     producer thread. NB: We create all the consumers before producers
     to ensure they exist before starting any producers that might try
     to pair with non-existent consumers. */

  for (i = 0; i < producers; i++) {
    if (pthread_create (&thread_table[i].consumer, &tattr,
			(void *(*)(void *))get_data, NULL) != 0) {
      perror ("Unable to create thread");
      exit (1);
    }
  }

  for (i = 0; i < producers; i++) {
    if (pthread_create (&thread_table[i].producer, &tattr,
			(void *(*)(void *))put_data, NULL) != 0) {
      perror ("Unable to create thread");
      exit (1);
    }
  }
#if 0
  for (i = 0; i < producers; i++) {
    if (pthread_create (&thread_table[i].producer, &tattr,
			(void *(*)(void *))put_data, NULL) != 0) {
      perror ("Unable to create thread");
      exit (1);
    }
    if (pthread_create (&thread_table[i].consumer, &tattr,
			(void *(*)(void *))get_data, NULL) != 0) {
      perror ("Unable to create thread");
      exit (1);
    }
  }
#endif
 
  for (i = 0; i < producers; i++) {
    pthread_join (thread_table[i].producer, NULL);
    pthread_join (thread_table[i].consumer, NULL);
  }

  // We don't need our mutex or condition variables any more
  pthread_mutex_destroy (&buffer.mutex);
  pthread_cond_destroy (&buffer.occupied_slot);
  pthread_cond_destroy (&buffer.empty_slot);
  
  return 0;
}
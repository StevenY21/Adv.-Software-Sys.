#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>


void *print_message_function( void *ptr );

main()
{
     pthread_t thread1, thread2, thread3, thread4, thread5, thread6, thread7;
     char *message1 = "Thread 1";
     char *message2 = "Thread 2";
     char *message3 = "Thread 3";
     char *message4 = "Thread 4";
     int  iret1, iret2, iret3, iret4;

     iret1 = pthread_create( &thread1, NULL, print_message_function, (void*) message1);
     iret2 = pthread_create( &thread2, NULL, print_message_function, (void*) message2);
     iret3 = pthread_create( &thread3, NULL, print_message_function, (void*) message3);
     iret4 = pthread_create( &thread4, NULL, print_message_function, (void*) message4);

     exit(0);
}

void *print_message_function( void *ptr )
{
     char *message;
     if(strcmp(ptr, "Thread 1") != 0){
	     sleep(3);
     }
     message = (char *) ptr;
     printf("%s \n", message);
}
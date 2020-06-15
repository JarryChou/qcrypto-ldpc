/**
 * notif_handler.cpp
 * 
 * Descript: 
 * This code will asynchronously read epochs from the input pipe and concat it to the command to execute.
 * This is different from the shell script which does them synchronously (incurring significant time delays with transferd)
 * This allows any epochs from transferd to be buffered with this program while executing other scripts (e.g. splicer or pfind/costream)
 * 
 * WARNING(s):
 * 1. Note that this script takes in a shell command as a user param and executes it. Extreme caution must be used with this program.
 * In fact, I will go so far as to say that in the future once the command is finalized, it should be hardcoded into this program, or
 * this program could be used as part of an attack.
 * 
 * Usage: notif_handler {input pipe from transferd} {command to execute}
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string>
#include <queue>
#include <unordered_set>

#define DEBUG 1
#undef DEBUG

#define BUFFER_LENGTH = 10

// Functions
void *read_notification_from_pipe( void *ptr );
void *process_notification( void *ptr );

// Variables
std::queue<std::string> pendingNotifsQueue;
std::unordered_set<std::string> receivedNotifsSet;
int itemsToProcess = 0;
char processingThreadBlocked = 0;

// Mutexes
// https://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html#SYNCHRONIZATION
// Also see: semaphores
pthread_mutex_t mutexQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexProcess = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char *argv[]) {
     // Param management
     if (argc != 3) {
          printf("Wrong number of parameters.\n Usage: program {input pipe from transferd} {shell command to execute}");
     }
     char *inputPipe = argv[1];
     char *shellCmd = argv[2];

     // Thread management
     pthread_t thread1, thread2;
     int iret1, iret2;

     /* Create independent threads each of which will execute function */
     // src: https://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html#SYNCHRONIZATION

     iret1 = pthread_create( &thread1, NULL, read_notification_from_pipe, (void*) inputPipe);
     iret2 = pthread_create( &thread2, NULL, process_notification, (void*) shellCmd);

     /* Wait till threads are complete before main continues. Unless we  */
     /* wait we run the risk of executing an exit which will terminate   */
     /* the process and all threads before the threads have completed.   */

     pthread_join( thread1, NULL);
     pthread_join( thread2, NULL); 

     exit(0);
}

/**
 * Thread 1: read notifications from pipe with mutex to handle data structures that need to be thread-safe.
 */
void *read_notification_from_pipe( void *ptr ) {
    char *inputPipeName = (char *) ptr;
    FILE *notifHandle = fopen(inputPipeName, "r+");
    // Open notification pipe
    if (notifHandle == NULL) { 
          fprintf(stderr, "Notification pipe couldn't be opened.\n");
    } else {
          // One epoch is 8 characters long
          char buffer[BUFFER_LENGTH];
          // Continually receive and store into queue
          while (1) {
               if (!fgets(buffer, BUFFER_LENGTH, notifHandle)) break;
               fprintf(stderr, "Received %s\n", buffer);
               std::string notification;
               notification += buffer;
               // If it's a new notif we've not received before
               if (receivedNotifsSet.find(notification) == receivedNotifsSet.end()) {
                    pthread_mutex_lock(&mutexQueue);
                    receivedNotifsSet.insert(notification);
                    pendingNotifsQueue.push(notification);
                    itemsToProcess++;
                    pthread_mutex_unlock(&mutexQueue);
                    if (processingThreadBlocked) {
                         pthread_mutex_unlock(&mutexProcess);
                    }
                    fprintf(stderr, "Unlocked\n");
               }
          }
          fprintf(stderr, "Something went wrong with the notifHandle.\n");
          fclose(notifHandle);
    }
}

/**
 * Thread 2: block if there is no new notifications. Otherwise, process the notifications.
 */
void *process_notification( void *ptr ) {
     // Initialize base command
     std::string baseCommand;
     baseCommand += (char *) ptr;
     baseCommand += " ";
     while (1) {
          // Block until we have something to process
          if (itemsToProcess <= 0) {
               pthread_mutex_lock(&mutexProcess);
               fprintf(stderr, "In blocking mode\n");
               processingThreadBlocked = 1;
               pthread_mutex_lock(&mutexProcess);
               fprintf(stderr, "Out of blocking mode\n");
               // We will now wait for the other pthread to unlock this mutex
               // Once that is done we will also unlock this mutex for repeated use
               pthread_mutex_unlock(&mutexProcess);
               processingThreadBlocked = 0;
          }
          fprintf(stderr, "Now beginning to process stuff\n");
          while (itemsToProcess > 0) {
               // Read epoch
               pthread_mutex_lock(&mutexQueue);
               std::string currentCommand = baseCommand + pendingNotifsQueue.front();
               pendingNotifsQueue.pop();
               itemsToProcess--;
               pthread_mutex_unlock(&mutexQueue);
               // Execute command on epoch
               system(currentCommand.c_str());
          }
     }
}
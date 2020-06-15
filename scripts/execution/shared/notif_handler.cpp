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
 * 2. Note that this script is very specific on _what_ it is receiving. It only receives items of length 8 (i.e. epochs). Anything else is truncated.
 * 
 * Usage: notif_handler {input pipe from transferd} {command to execute}
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string>
#include <queue>
#include <unordered_set>

#undef DEBUG
#define DEBUG 1

#define BUFFER_LENGTH 10
#define EPOCH_LENGTH 8

// Functions
void *read_notification_from_pipe( void *ptr );
void *process_notification( void *ptr );

// Variables
// Using string to avoid the headache, but you could optimize if this is necessary
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
          exit(1);
     }
     char *inputPipe = argv[1];
     char *shellCmd = argv[2];

     // Thread management
     /* Create independent threads each of which will execute function */
     // src: https://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html#SYNCHRONIZATION
     pthread_t thread1;
     // Read notifications on pthread
     int iret1 = pthread_create(&thread1, NULL, read_notification_from_pipe, (void*) inputPipe)
     // Process notifications on main 
     process_notification(shellCmd);
     // This section never gets called in the current code because there's no termination
     pthread_join(thread1, NULL);
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
               // Possible optimization: Increase buffer size, string tokenize
               if (!fgets(buffer, BUFFER_LENGTH, notifHandle)) break;
               #ifdef DEBUG
               fprintf(stderr, "Received %s\n", buffer);
               #endif
               std::string notification;
               notification.reserve(BUFFER_LENGTH);
               notification += buffer;
               // If it's a new notif we've not received before
               if (receivedNotifsSet.find(notification) == receivedNotifsSet.end()) {
                    receivedNotifsSet.insert(notification);
                    pthread_mutex_lock(&mutexQueue);
                    pendingNotifsQueue.push(notification);
                    itemsToProcess++;
                    pthread_mutex_unlock(&mutexQueue);
                    // Unblock the other thread if needed
                    if (processingThreadBlocked) {
                         pthread_mutex_unlock(&mutexProcess);
                    }
                    #ifdef DEBUG
                    fprintf(stderr, "Unlocked\n");
                    #endif
               }
          }
          fprintf(stderr, "Something went wrong with the notifHandle.\n");
          fclose(notifHandle);
    }
}

/**
 * Thread 2: block if there is no new notifications. Otherwise, process the notifications.
 */
void process_notification( char *ptr ) {
     // Initialize base command
     // Reuse the same char buffer for optimal performance
     int baseCmdLength = strlen(ptr);
     int cmdBufferLength = baseCmdLength + EPOCH_LENGTH + 2;
     char cmdBuffer[baseCmdLength + EPOCH_LENGTH + 2]; // +1 for terminating character, +1 for space in between
     strcpy(cmdBuffer, ptr);
     cmdBuffer[baseCmdLength] = ' ';
     while (1) {
          // Block until we have something to process
          if (itemsToProcess <= 0) {
               pthread_mutex_lock(&mutexProcess);
               #ifdef DEBUG
               fprintf(stderr, "In blocking mode\n");
               #endif
               processingThreadBlocked = 1;
               pthread_mutex_lock(&mutexProcess);
               #ifdef DEBUG
               fprintf(stderr, "Out of blocking mode\n");
               #endif
               // We will now wait for the other pthread to unlock this mutex
               // Once that is done we will also unlock this mutex for repeated use
               pthread_mutex_unlock(&mutexProcess);
               processingThreadBlocked = 0;
          }
          #ifdef DEBUG
          fprintf(stderr, "Now beginning to process stuff\n");
          #endif
          while (itemsToProcess > 0) {
               // Read epoch
               pthread_mutex_lock(&mutexQueue);
               // Put terminating character after the base command
               cmdBuffer[baseCmdLength + 1] = '\0';
               // Since we know the length of the epoch we can do this
               strncpy(cmdBuffer, pendingNotifsQueue.front().c_str(), EPOCH_LENGTH);
               // Add terminating character at the very end (this doesn't truncate epoch)
               cmdBuffer[cmdBufferLength - 1] = '\0';
               pendingNotifsQueue.pop();
               itemsToProcess--;
               pthread_mutex_unlock(&mutexQueue);
               // Execute command on epoch
               system(cmdBuffer);
          }
     }
}
/**
 * notif_handler.cpp
 * 
 * Descript: 
 * This code will asynchronously read epochs from the input pipe and concat it to the command to execute.
 * This is different from the shell script which does them synchronously (incurring significant time delays with transferd)
 * This allows any epochs from transferd to be buffered with this program while executing other scripts (e.g. splicer or pfind/costream)
 * 
 * KIV: Add-on to allow it to handle more pipes
 * KIV: printf the system command so that it can output more formats
 * 
 * WARNING(s):
 * 1. Note that this script takes in a shell command as a user param and executes it. Extreme caution must be used with this program.
 * In fact, I will go so far as to say that in the future once the command is finalized, it should be hardcoded into this program, or
 * this program could be used as part of an attack.
 * 
 * 2. Note that this script is very specific on _what_ it is receiving. 
 * It only receives items of length 8 (i.e. epochs). Anything else is truncated.
 * 
 * Usage: notif_handler inputPipe command [maxConsecutive]
 *   KIV: [inputPipe command ...]
 *   
 * inputPipe:       Input pipe, usually from transferd
 * command:         Command to execute (e.g. "sudo bash xxx.sh"). The epoch will be concatenated to the end of this command.
 * maxConsecutive:  If set, represents the maximum number of epochs to buffer before sending.
 *                  The command sent will also be different. The format will then be "command epoch maxConsecutive".
 *                  Buffered epochs refer to epochs that are consecutively numbered e.g. abcd1111, abcd1112, abcd1113, then if
 *                  abcd1115 arrives, "cmd abcd1111 3" will be called.
 *                  By default, this is disabled and the command is just sent as soon as possible as "command epoch".
 *                  The current maximum value is 999 (configure MAX_BUFFERED_STR_LEN to support longer values).
 * 
 * KIV: The notif handler can buffer from multiple input pipes. The current implementation uses:
 * 1 pThread for reading from multiple inputPipes
 * 1 pThread PER command handler (commands are run synchronously on the thread itself, although this can be easily modified by adding an ampersand to the command)
 * 
 * Example(s): notif_handler "./pipes/pipe" "echo"
 *             notif_handler "./pipes/pipe" "echo" "./pipes/pipe2" "sudo bash cmd.sh" 
 * 
 * Considerations:
 * 1.  What happens if the program encounters multiple duplicate epochs?
 *     It will not send repeated epochs. However, it will only check the previous epoch. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string>
#include <cstring>
#include <queue>

#undef DEBUG
#define DEBUG 1

#define BUFFER_LENGTH 10
#define EPOCH_LENGTH 8
#define MAX_BUFFERED_STR_LEN 3 // Just additional buffer space for the # of epochs buffered

// Helper functions
/**
 * Calculates the difference between 2 epochs from LSB.
 * @param epoch1 1st epoch in hex format
 * @param epoch2 2nd epoch in hex format
 * @return difference
*/
int calculateDiffBetweenEpochs(char* epoch1, char* epoch2) {
     int diff = 0;
     int correctedEpoch1Char = 0;
     // Start from LSB
     for (size_t i = EPOCH_LENGTH - 1; i >= 0; i--) {
          // Cannot early terminate to account for huge epoch jumps
          if (epoch1[i] != epoch2[i]) {
               // Correct epoch1[i] to contiguous 0,1,2...d,e,f
               if (epoch1[i] < 60) { // is ASCII 0-9, connect it to a,b...e,f
                    correctedEpoch1Char = epoch1[i] + 37;
               } else if (epoch1[i] < 71) { // is ASCII A-F (capital), make it lower case
                    correctedEpoch1Char = epoch1[i] + 32;
               } else { // is ASCII a-f, use as is
                    correctedEpoch1Char = epoch1[i];
               }
               // Since each hex is 4 bits, bitshift the diff 4 bits to the left accordingly
               diff = (epoch2[i] - correctedEpoch1Char) << (4 * (EPOCH_LENGTH - 1 - i));
          }
     }
     return diff;
}

/**
 * Helper function to swap the epochs in the prevNotif with the buffer epoch using XOR
 */
void swapPrevWithBuffer(char* prevNotif, char* buffer) {
     for (size_t i = 0; i < EPOCH_LENGTH; i++) {
          prevNotif[i] ^= buffer[i];
          buffer[i] ^= prevNotif[i];
          prevNotif[i] ^= buffer[i];
     }
     
}

// Thread functions
void *read_notification_from_pipe( void *ptr );
void process_notification( char *ptr );

// Variables
// Using string to avoid the headache, but you could optimize if this is necessary
std::queue<std::string> pendingNotifsQueue;
char previousNotif[EPOCH_LENGTH + 1]; // Refers to the previous notification sent.
int itemsToProcess = 0;
int maxConsecutive = 0;
int currentConsecutive = 1;
char processingThreadBlocked = 0;

// Mutexes
// https://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html#SYNCHRONIZATION
// Also see: semaphores
pthread_mutex_t mutexQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexProcess = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char *argv[]) {
     // KIV: Use proper argument handling? Not really necessary though
     // https://stackoverflow.com/questions/9642732/parsing-command-line-arguments-in-c
     if (argc < 3 || argc > 4) {
          printf("Wrong number of parameters.\n Usage: notif_handler inputPipe command [bufferMaxForEc]");
          exit(1);
     } 
     char *inputPipe = argv[1];
     char *shellCmd = argv[2];
     if (argc == 4) { maxConsecutive = strtol(argv[3], NULL, 10); } // Read as number

     // Thread management
     /* Create independent threads each of which will execute function */
     // src: https://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html#SYNCHRONIZATION
     pthread_t thread1;
     // Read notifications on pthread
     int iret1 = pthread_create(&thread1, NULL, read_notification_from_pipe, (void*) inputPipe);
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
    int epochDiff = 0;
    // Open notification pipe
    if (notifHandle == NULL) { 
          fprintf(stderr, "Notification pipe couldn't be opened.\n");
    } else {
          // One notification/epoch is 8 characters long, unless we want to also add the number of consecutive epochs
          char buffer[BUFFER_LENGTH + 1 + (MAX_BUFFERED_STR_LEN + 1) * (maxConsecutive ? 1 : 0)];
          // Continually receive and store into queue
          while (1) {
               // Possible optimization: Increase buffer size, string tokenize
               // Read into buffer
               if (!fgets(buffer, BUFFER_LENGTH, notifHandle)) break;
               #ifdef DEBUG
               fprintf(stderr, "Received %s\n", buffer);
               #endif
               // Check if it's a new notif we've not received before
               epochDiff = calculateDiffBetweenEpochs(buffer, previousNotif);
               // Mode: normal execution
               if (maxConsecutive == 0) {
                    // Continue case: repeated epoch
                    if (epochDiff == 0) { continue; }
               } else {
                    // Mode: Buffering and sending first epoch + consecutive epochs
                    // Since previousNotif stores the previousNotif we sent, we need to calc the actual epochDiff
                    epochDiff -= currentConsecutive - 1;
                    // Continue case: repeated epoch
                    if (epochDiff == 0) { continue; }
                    // Continue case: consecutive epoch and haven't hit max
                    else if (epochDiff == 1 && currentConsecutive < maxConsecutive) {
                         currentConsecutive++;
                         continue;
                    } else if (currentConsecutive >= maxConsecutive) {
                         // Push to queue case: chain broken
                         // Swap previous epoch with buffer epoch
                         swapPrevWithBuffer(previousNotif, buffer);
                         // Prepare notif with buffer
                         buffer[BUFFER_LENGTH] = ' ';
                         sprintf(&buffer[BUFFER_LENGTH + 1],"%d", currentConsecutive);
                         buffer[BUFFER_LENGTH + MAX_BUFFERED_STR_LEN + 2] =  '\0';
                         currentConsecutive = 1;
                    }
               }
               // Push to queue
               std::string notification;
               if (maxConsecutive == 0) {
                    notification.reserve(BUFFER_LENGTH + 1);
                    // Replace previous epoch with buffer
                    strncpy(previousNotif, buffer, EPOCH_LENGTH);
                    previousNotif[EPOCH_LENGTH + 1] = '\0';
               } else {
                    notification.reserve(BUFFER_LENGTH + MAX_BUFFERED_STR_LEN + 2);
               }
               notification += buffer;
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
     int cmdBufferLength = baseCmdLength + EPOCH_LENGTH + MAX_BUFFERED_STR_LEN + 2;
     char cmdBuffer[cmdBufferLength]; // +1 for terminating character, +1 for space in between
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
          while (itemsToProcess > 0) {
               // Read epoch
               pthread_mutex_lock(&mutexQueue);
               // Put terminating character after the base command
               cmdBuffer[baseCmdLength + 1] = '\0';
               // Since we know the length of the epoch we can do this
               strncat(cmdBuffer, pendingNotifsQueue.front().c_str(), EPOCH_LENGTH);
               // Add terminating character at the very end in case it's not already in (this doesn't truncate epoch)
               cmdBuffer[cmdBufferLength - 1] = '\0';
               pendingNotifsQueue.pop();
               itemsToProcess--;
               pthread_mutex_unlock(&mutexQueue);
               // Execute command on epoch
               system(cmdBuffer);
               #ifdef DEBUG
               fprintf(stderr, "exec %s\n", cmdBuffer);
               #endif
          }
     }
}
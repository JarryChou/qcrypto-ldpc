#include "debug.h"

// DEBUGGER HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */
/**
 * @brief malloc wrapper for debugging purposes
 * 
 * @param s 
 * @return char* 
 */
char *malloc2(unsigned int s) {
  char *p;
  #ifdef mallocdebug
  printf("process %d malloc call no. %d for %d bytes...", getpid(), mcall, s);
  #endif
  // mcall++; // unusued global int variable for debugging 
  p = malloc(s);
  #ifdef mallocdebug
  printf("returned: %p\n", p);
  #endif
  return p;
}

/**
 * @brief free wrapper for debugging purposes
 * 
 * @param p 
 */
void free2(void *p) {
  #ifdef mallocdebug
  printf("process %d free call no. %d for %p\n", getpid(), fcall, p);
  #endif
  // fcall++; // unusued global int variable for debugging 
  free(p);
  return;
}

int mdmpidx = 0; /**< Debug value for printing */
/**
 * @brief Helper to dump message into a file
 * 
 * @param kb processblock pointer
 * @param msg message to dump to file
 */
void dumpmsg(ProcessBlock *kb, char *msg) {
  char dumpname[200];
  int dha; /* handle */
  int tosend = ((unsigned int *)msg)[1];
  int sent = 0, retval;

  #ifdef DEBUG
  #else
  return; /* if debug is off */
  #endif

  sprintf(dumpname, "msgdump_%1d_%03d", kb->role, mdmpidx);
  mdmpidx++;
  dha = open(dumpname, O_WRONLY | O_CREAT, 0644);
  do {
    retval = write(dha, msg, tosend - sent);
    if (retval == -1) {
      fprintf(stderr, "cannot save msg\n");
      exit(-1);
    }
    usleep(100000);
    sent += retval;
  } while (tosend - sent > 0);
  close(dha);
  return;
}

int dumpindex = 0; /**< Debug value for printing */
/**
 * @brief Helper function to dump the state of the system to a disk file . 
 * 
 * Dumps the processblock structure, if present the buffer files, the parity files and the
   diffidx buffers as plain binaries
 * 
 * @param kb processblock pointer
 */
void dumpstate(ProcessBlock *kb) {
  char dumpname[200];
  int dha; /* handle */

  #ifndef DEBUG
  return; /* if debugging is off */
  #endif

  sprintf(dumpname, "kbdump_%1d_%03d", kb->role, dumpindex);
  dumpindex++;
  dha = open(dumpname, O_WRONLY | O_CREAT, 0644);
  if (write(dha, kb, sizeof(ProcessBlock)) == -1) {
    fprintf(stderr, "dump fail (1)\n");
  }
  if (kb->mainBufPtr)
    if (-1 == write(dha, kb->mainBufPtr, sizeof(unsigned int) *
        (2 * kb->initialBits + 3 * ((kb->initialBits + 31) / 32)))) {
      fprintf(stderr, "dump fail (2)\n");
    }

  if (kb->lp0)
    if (-1 == write(dha, kb->lp0, sizeof(unsigned int) * 6 * ((kb->workbits + 31) / 32))) {
      fprintf(stderr, "dump fail (4)\n");
    }

  if (kb->diffidx)
    if (-1 == write(dha, kb->diffidx, sizeof(unsigned int) * 2 * kb->diffBlockCountMax)) {
      fprintf(stderr, "dump fail (5)\n");
    }

  close(dha);
  return;
}

/**
 * @brief for debug: output permutation
 * 
 * @param kb processblock pointer
 */
void output_permutation(ProcessBlock *kb) {
  char name[200] = "permutlist_";
  FILE *fp;
  int i;
  sprintf(name, "permutelist_%d", kb->role);
  fp = fopen(name, "w");
  for (i = 0; i < kb->workbits; i++)
    fprintf(fp, "%d %d\n", kb->permuteIndex[i], kb->reverseIndex[i]);
  fclose(fp);
}
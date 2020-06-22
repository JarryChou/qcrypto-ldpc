#include "debug.h"

// DEBUGGER HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */
/* debugging */
int mcall = 0, fcall = 0;
char *malloc2(unsigned int s) {
  char *p;
  #ifdef mallocdebug
  printf("process %d malloc call no. %d for %d bytes...", getpid(), mcall, s);
  #endif
  mcall++;
  p = malloc(s);
  #ifdef mallocdebug
  printf("returned: %p\n", p);
  #endif
  return p;
}
void free2(void *p) {
  #ifdef mallocdebug
  printf("process %d free call no. %d for %p\n", getpid(), fcall, p);
  #endif
  fcall++;
  free(p);
  return;
}

/* helper to dump message into a file */
int mdmpidx = 0;
void dumpmsg(struct keyblock *kb, char *msg) {
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

/* helper function to dump the state of the system to a disk file . Dumps the
   keyblock structure, if present the buffer files, the parity files and the
   diffidx buffers as plain binaries */
int dumpindex = 0;
void dumpstate(struct keyblock *kb) {
  char dumpname[200];
  int dha; /* handle */

  return; /* if debugging is off */

  sprintf(dumpname, "kbdump_%1d_%03d", kb->role, dumpindex);
  dumpindex++;
  dha = open(dumpname, O_WRONLY | O_CREAT, 0644);
  write(dha, kb, sizeof(struct keyblock));
  if (kb->mainbuf)
    write(dha, kb->mainbuf,
          sizeof(unsigned int) *
              (2 * kb->initialbits + 3 * ((kb->initialbits + 31) / 32)));

  if (kb->lp0)
    write(dha, kb->lp0, sizeof(unsigned int) * 6 * ((kb->workbits + 31) / 32));

  if (kb->diffidx)
    write(dha, kb->diffidx, sizeof(unsigned int) * 2 * kb->diffnumber_max);

  close(dha);
  return;
}

/* for debug: output permutation */
void output_permutation(struct keyblock *kb) {
  char name[200] = "permutlist_";
  FILE *fp;
  int i;
  sprintf(name, "permutelist_%d", kb->role);
  fp = fopen(name, "w");
  for (i = 0; i < kb->workbits; i++)
    fprintf(fp, "%d %d\n", kb->permuteindex[i], kb->reverseindex[i]);
  fclose(fp);
}

//  ERROR MANAGEMENT
int emsg(int code) {
  fprintf(stderr, "%s\n", errormessage[code]);
  return code;
}
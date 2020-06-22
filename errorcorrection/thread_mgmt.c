#include "thread_mgmt.h"

// THREAD MANAGEMENT
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS
/* code to check if a requested bunch of epochs already exists in the thread
   list. Uses the start epoch and an epoch number as arguments; returns 0 if
   the requested epochs are not used yet, otherwise 1. */
int check_epochoverlap(unsigned int epoch, int num) {
  struct blockpointer *bp = blocklist;
  unsigned int se;
  int en;
  while (bp) { /* as long as there are more blocks to test */
    se = bp->content->startepoch;
    en = bp->content->numberofepochs;
    if (MAX(se, epoch) <= (MIN(se + en, epoch + num) - 1)) {
      return 1; /* overlap!! */
    }
    bp = bp->next;
  }
  /* did not find any overlapping epoch */
  return 0;
}

// MAIN FUNCTIONS
/* code to prepare a new thread from a series of raw key files. Takes epoch,
   number of epochs and an initially estimated error as parameters. Returns
   0 on success, and 1 if an error occurred (maybe later: errorcode) */
int create_thread(unsigned int epoch, int num, float inierr, float BellValue) {
  static unsigned int temparray[TEMPARRAYSIZE];
  static struct header_3 h3;           /* header for raw key file */
  unsigned int residue, residue2, tmp; /* leftover bits at end */
  int resbitnumber;                    /* number of bits in the residue */
  int newindex;                        /* points to the next free word */
  unsigned int epi;                    /* epoch index */
  unsigned int enu;
  int retval, i, bitcount;
  char ffnam[FNAMELENGTH + 10]; /* to store filename */
  struct blockpointer *bp;      /* to hold new thread */
  int getbytes;                 /* how much memory to ask for */
  unsigned int *rawmem;         /* to store raw key */

  /* read in file by file in temporary array */
  newindex = 0;
  resbitnumber = 0;
  residue = 0;
  bitcount = 0;
  for (enu = 0; enu < num; enu++) {
    epi = epoch + enu; /* current epoch index */
    strncpy(ffnam, fname[handleId_rawKeyDir], FNAMELENGTH);
    atohex(&ffnam[strlen(ffnam)], epi);
    handle[handleId_rawKeyDir] = open(ffnam, FILEINMODE); /* in blocking mode */
    if (-1 == handle[handleId_rawKeyDir]) {
      fprintf(stderr, "cannot open file >%s< errno: %d\n", ffnam, errno);
      return 67; /* error opening file */
    }
    /* read in file 3 header */
    if (sizeof(h3) != (i = read(handle[handleId_rawKeyDir], &h3, sizeof(h3)))) {
      fprintf(stderr, "error in read: return val:%d errno: %d\n", i, errno);
      return 68;
    }
    if (h3.epoc != epi) {
      fprintf(stderr, "incorrect epoch; want: %08x have: %08x\n", epi, h3.epoc);
      return 69; /* not correct epoch */
    }

    if (h3.bitsperentry != 1) return 70; /* not a BB84 raw key */
    if (bitcount + h3.length >= MAXBITSPERTHREAD)
      return 71; /* not enough space */

    i = (h3.length / 32) +
        ((h3.length & 0x1f) ? 1 : 0); /* number of words to read */
    retval = read(handle[handleId_rawKeyDir], &temparray[newindex], i * sizeof(unsigned int));
    if (retval != i * sizeof(unsigned int)) return 72; /* not enough read */

    /* close and possibly remove file */
    close(handle[handleId_rawKeyDir]);
    if (killmode) {
      retval = unlink(ffnam);
      if (retval) return 66;
    }

    /* residue update */
    tmp = temparray[newindex + i - 1] & ((~1) << (31 - (h3.length & 0x1f)));
    residue |= (tmp >> resbitnumber);
    residue2 = tmp << (32 - resbitnumber);
    resbitnumber += (h3.length & 0x1f);
    if (h3.length & 0x1f) { /* we have some residual bits */
      newindex += (i - 1);
    } else { /* no fresh residue, old one stays as is */
      newindex += i;
    }
    if (resbitnumber > 31) {         /* write one in buffer */
      temparray[newindex] = residue; /* store residue */
      newindex += 1;
      residue = residue2; /* new residue */
      resbitnumber -= 32;
    }
    bitcount += h3.length;
  }

  /* finish up residue */
  if (resbitnumber > 0) {
    temparray[newindex] = residue; /* msb aligned */
    newindex++;
  } /* now newindex contains the number of words for this key */

  /* create thread structure */
  bp = (struct blockpointer *)malloc2(sizeof(struct blockpointer));
  if (!bp) return 34; /* malloc failed */
  bp->content = (struct keyblock *)malloc2(sizeof(struct keyblock));
  if (!(bp->content)) return 34; /* malloc failed */
  /* zero all otherwise unset keyblock entries */
  bzero(bp->content, sizeof(struct keyblock));
  /* how much memory is needed ?
     raw key, permuted key, test selection, two permutation indices */
  getbytes = newindex * 3 * sizeof(unsigned int) +
             bitcount * 2 * sizeof(unsigned short int);
  rawmem = (unsigned int *)malloc2(getbytes);
  if (!rawmem) return 34;       /* malloc failed */
  bp->content->rawmem = rawmem; /* for later free statement */
  bp->content->startepoch = epoch;
  bp->content->numberofepochs = num;
  bp->content->mainbuf = rawmem; /* main key; keep this in mind for free */
  bp->content->permutebuf = &bp->content->mainbuf[newindex];
  bp->content->testmarker = &bp->content->permutebuf[newindex];
  bp->content->permuteindex =
      (unsigned short int *)&bp->content->testmarker[newindex];
  bp->content->reverseindex =
      (unsigned short int *)&bp->content->permuteindex[bitcount];
  /* copy raw key into thread and clear testbits, permutebits */
  for (i = 0; i < newindex; i++) {
    bp->content->mainbuf[i] = temparray[i];
    bp->content->permutebuf[i] = 0;
    bp->content->testmarker[i] = 0;
  }
  bp->content->initialbits = bitcount; /* number of bits in stream */
  bp->content->leakagebits = 0;        /* start with no initially lost bits */
  bp->content->processingstate = PRS_JUSTLOADED; /* just read in */
  bp->content->initialerror = (int)(inierr * (1 << 16));
  bp->content->BellValue = BellValue;
  /* insert thread in thread list */
  bp->epoch = epoch;
  bp->previous = NULL;
  bp->next = blocklist;
  if (blocklist) blocklist->previous = bp; /* update existing first entry */
  blocklist = bp;                          /* update blocklist */
  return 0;
}

/* function to obtain the pointer to the thread for a given epoch index.
   Argument is epoch, return value is pointer to a struct keyblock or NULL
   if none found. */
struct keyblock *get_thread(unsigned int epoch) {
  struct blockpointer *bp = blocklist;
  while (bp) {
    if (bp->epoch == epoch) return bp->content;
    bp = bp->next;
  }
  return NULL;
}

/* function to remove a thread out of the list. parameter is the epoch index,
   return value is 0 for success and 1 on error. This function is called if
   there is no hope for error recovery or for a finished thread. */
int remove_thread(unsigned int epoch) {
  struct blockpointer *bp = blocklist;
  while (bp) {
    if (bp->epoch == epoch) break;
    bp = bp->next;
  }
  if (!bp) return 49; /* no block there */
  /* remove all internal structures */
  free2(bp->content->rawmem); /* bit buffers, changed to rawmem 11.6.06chk */
  if (bp->content->lp0) free(bp->content->lp0); /* parity storage */
  if (bp->content->diffidx) free2(bp->content->diffidx);
  free2(bp->content); /* main thread frame */

  /* unlink thread out of list */
  if (bp->previous) {
    bp->previous->next = bp->next;
  } else {
    blocklist = bp->next;
  }
  if (bp->next) bp->next->previous = bp->previous;

  printf("removed thread %08x, new blocklist: %p \n", epoch, blocklist);
  fflush(stdout);
  /* remove thread list entry */
  free2(bp);
  return 0;
}
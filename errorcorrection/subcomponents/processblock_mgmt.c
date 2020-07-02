#include "processblock_mgmt.h"

// processblock MANAGEMENT HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */

/**
 * @brief code to check if a requested bunch of epochs already exists in the processblock list.
 * 
 * @param epoch start epoch
 * @param num number of consecutive epochs
 * @return int 0 if requested epochs are not used yet, otherwise 1
 */
int check_epochoverlap(unsigned int epoch, int num) {
  ProcessBlockDequeNode *bp = processBlockDeque;
  unsigned int se;
  int en;
  while (bp) { /* as long as there are more blocks to test */
    se = bp->content->startEpoch;
    en = bp->content->numberOfEpochs;
    if (MAX(se, epoch) <= (MIN(se + en, epoch + num) - 1)) {
      return 1; /* overlap!! */
    }
    bp = bp->next;
  }
  /* did not find any overlapping epoch */
  return 0;
}

/**
 * @brief Helper function that marks the end of the QBER process
 * 
 * @param processBlock 
 */
void pBlkMgmt_finishQberEst(ProcessBlock *processBlock) {
  /* determine process variables */
  processBlock->processingState = PSTATE_ERR_KNOWN;
  processBlock->estimatedSampleSize = processBlock->leakageBits; /* is this needed? */
}

// processblock MANAGEMENT MAIN FUNCTIONS
/* ------------------------------------------------------------------------- */
/**
 * @brief code to prepare a new processblock from a series of raw key files. 
 * 
 * @param epoch start epoch
 * @param num number of consecutive epochs
 * @param inierr initially estimated error
 * @param bellValue 
 * @return int 0 on success, otherwise error code
 */
int pBlkMgmt_createProcessBlock(unsigned int epoch, int num, float inierr, float bellValue) {
  static unsigned int temparray[TEMP_ARRAY_SIZE];
  static struct header_3 h3;           /* header for raw key file */
  unsigned int residue, residue2, tmp; /* leftover bits at end */
  int resbitnumber;                    /* number of bits in the residue */
  int newindex;                        /* points to the next free word */
  unsigned int epi;                    /* epoch index */
  unsigned int enu;
  int retval, i, bitcount;
  char ffnam[FNAME_LENGTH + 10]; /* to store filename */
  ProcessBlockDequeNode *bp;      /* to hold new processblock */
  int getbytes;                 /* how much memory to ask for */
  unsigned int *rawMemPtr;         /* to store raw key */

  /* read in file by file in temporary array */
  newindex = 0;
  resbitnumber = 0;
  residue = 0;
  bitcount = 0;
  for (enu = 0; enu < num; enu++) {
    epi = epoch + enu; /* current epoch index */
    strncpy(ffnam, arguments.fname[handleId_rawKeyDir], FNAME_LENGTH);
    atohex(&ffnam[strlen(ffnam)], epi);
    arguments.handle[handleId_rawKeyDir] = open(ffnam, FILE_INMODE); /* in blocking mode */
    if (-1 == arguments.handle[handleId_rawKeyDir]) {
      fprintf(stderr, "cannot open file >%s< errno: %d\n", ffnam, errno);
      return 67; /* error opening file */
    }
    /* read in file 3 header */
    if (sizeof(h3) != (i = read(arguments.handle[handleId_rawKeyDir], &h3, sizeof(h3)))) {
      fprintf(stderr, "error in read: return val:%d errno: %d\n", i, errno);
      return 68;
    }
    if (h3.epoc != epi) {
      fprintf(stderr, "incorrect epoch; want: %08x have: %08x\n", epi, h3.epoc);
      return 69; /* not correct epoch */
    }

    if (h3.bitsperentry != 1) return 70; /* not a BB84 raw key */
    if (bitcount + h3.length >= MAX_BITS_PER_PROCESSBLOCK)
      return 71; /* not enough space */

    i = wordIndex(h3.length) + 
        (modulo32(h3.length) ? 1 : 0); /* number of words to read */
    retval = read(arguments.handle[handleId_rawKeyDir], &temparray[newindex], i * WORD_SIZE);
    if (retval != i * WORD_SIZE) return 72; /* not enough read */

    /* close and possibly remove file */
    close(arguments.handle[handleId_rawKeyDir]);
    if (arguments.removeRawKeysAfterUse) {
      retval = unlink(ffnam);
      if (retval) return 66;
    }

    /* residue update */
    tmp = temparray[newindex + i - 1] & ((~1) << (31 - modulo32(h3.length)));
    residue |= (tmp >> resbitnumber);
    residue2 = tmp << (32 - resbitnumber);
    resbitnumber += modulo32(h3.length);
    if (modulo32(h3.length) != 0) { /* we have some residual bits */
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

  /* create processblock structure */
  bp = (ProcessBlockDequeNode *)malloc2(sizeof(ProcessBlockDequeNode));
  if (!bp) return 34; /* malloc failed */
  bp->content = (ProcessBlock *)malloc2(sizeof(ProcessBlock));
  if (!(bp->content)) return 34; /* malloc failed */
  /* zero all otherwise unset processblock entries */
  bzero(bp->content, sizeof(ProcessBlock));
  /* how much memory is needed ?
     raw key, permuted key, test selection, two permutation indices */
  getbytes = newindex * 3 * WORD_SIZE +
             bitcount * 2 * sizeof(unsigned short int);
  rawMemPtr = (unsigned int *)malloc2(getbytes);
  if (!rawMemPtr) return 34;       /* malloc failed */
  bp->content->rawMemPtr = rawMemPtr; /* for later free statement */
  bp->content->startEpoch = epoch;
  bp->content->numberOfEpochs = num;
  bp->content->mainBufPtr = rawMemPtr; /* main key; keep this in mind for free */
  bp->content->permuteBufPtr = &bp->content->mainBufPtr[newindex];
  bp->content->testedBitsMarker = &bp->content->permuteBufPtr[newindex];
  bp->content->permuteIndex =
      (unsigned short int *)&bp->content->testedBitsMarker[newindex];
  bp->content->reverseIndex =
      (unsigned short int *)&bp->content->permuteIndex[bitcount];
  /* copy raw key into processblock and clear testbits, permutebits */
  for (i = 0; i < newindex; i++) {
    bp->content->mainBufPtr[i] = temparray[i];
    bp->content->permuteBufPtr[i] = 0;
    bp->content->testedBitsMarker[i] = 0;
  }
  bp->content->initialBits = bitcount; /* number of bits in stream */
  bp->content->leakageBits = 0;        /* start with no initially lost bits */
  bp->content->processingState = PSTATE_JUST_LOADED; /* just read in */
  bp->content->initialErrRate = (int)(inierr * (1 << 16));
  bp->content->bellValue = bellValue;
  /* insert processblock in processblock list */
  bp->epoch = epoch;
  bp->previous = NULL;
  bp->next = processBlockDeque;
  if (processBlockDeque) processBlockDeque->previous = bp; /* update existing first entry */
  processBlockDeque = bp;                          /* update processBlockDeque */
  return 0;
}

/**
 * @brief Function to obtain the pointer to the processblock for a given epoch index.
 * 
 * @param epoch epoch 
 * @return ProcessBlock* pointer to a processblock, or NULL if none found
 */
ProcessBlock *pBlkMgmt_getProcessBlock(unsigned int epoch) {
  ProcessBlockDequeNode *bp = processBlockDeque;
  while (bp) {
    if (bp->epoch == epoch) return bp->content;
    bp = bp->next;
  }
  return NULL;
}

/**
 * @brief Function to remove a processblock out of the list.
 * 
 *  This function is called if
   there is no hope for error recovery or for a finished processblock.
 * 
 * @param epoch epoch index
 * @return int 0 if success, 1 for error
 */
int pBlkMgmt_removeProcessBlock(unsigned int epoch) {
  ProcessBlockDequeNode *bp = processBlockDeque;
  while (bp) {
    if (bp->epoch == epoch) break;
    bp = bp->next;
  }
  if (!bp) return 49; /* no block there */
  /* remove all internal structures */
  free2(bp->content->rawMemPtr); /* bit buffers, changed to rawMemPtr 11.6.06chk */
  if (bp->content->lp0) free(bp->content->lp0); /* parity storage */
  if (bp->content->diffidx) free2(bp->content->diffidx);
  free2(bp->content); /* main processblock frame */

  /* unlink processblock out of list */
  if (bp->previous) {
    bp->previous->next = bp->next;
  } else {
    processBlockDeque = bp->next;
  }
  if (bp->next) bp->next->previous = bp->previous;

  printf("removed processblock %08x, new processBlockDeque: %p \n", epoch, processBlockDeque);
  fflush(stdout);
  /* remove processblock list entry */
  free2(bp);
  return 0;
}

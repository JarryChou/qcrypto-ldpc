#include "cascade_biconf.h"

// PERMUTATIONS HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */
/** @brief Helper to fix the permuted/unpermuted bit changes.
  
  Decides via a parameter in pb->binarySearchDepth MSB what polarity to take.

  @param pb ptr to processblock to fix */
void fix_permutedbits(ProcessBlock *pb) {
  int i, k;
  unsigned int *src, *dst;
  unsigned short *idx; /* pointers to data loc and permute idx */
  if (pb->binarySearchDepth & RUNLEVEL_LEVELMASK) { /* we are in pass 1 */
    src = pb->permuteBufPtr;
    dst = pb->mainBufPtr;
    idx = pb->reverseIndex;
  } else { /* we are in pass 0 */
    src = pb->mainBufPtr;
    dst = pb->permuteBufPtr;
    idx = pb->permuteIndex;
  }
  bzero(dst, wordCount(pb->workbits) * WORD_SIZE); /* clear dest */
  for (i = 0; i < pb->workbits; i++) {
    k = idx[i]; /* permuted bit index */
    if (uint32AllZeroExceptAtN(i) & src[wordIndex(i)]) dst[wordIndex(k)] |= uint32AllZeroExceptAtN(k);
  }
  return;
}

// CASCADE BICONF HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */
/** @brief Helper function to generate a pseudorandom bit pattern into the test bit
   buffer. 

   Unused
   
  @param pb processblock pointer (kb contains other params for final parity test)
  @param seed seed for the RNG
 */
/*
void generate_selectbitstring(ProcessBlock *pb, unsigned int seed) {
  int i;                                    // number of bits to be set 
  pb->rngState = seed;                     // set new seed 
  for (i = 0; i < wordIndex(pb->workbits); i++) // take care of the full bits 
    pb->testedBitsMarker[i] = rnd_getPrngValue2_32(&pb->rngState);
  pb->testedBitsMarker[wordIndex(pb->workbits)] = // prepare last few bits 
      rnd_getPrngValue2_32(&pb->rngState) & uint32AllOnesExceptLastN(modulo32(pb->workbits - 1));
  return;
}
*/

/** @brief Helper function to generate a pseudorandom bit pattern into the test bit
   buffer AND transfer the permuted key buffer into it.
     
   For a more compact parity generation in the last round.

   @param pb processblock pointer (kb contains other params for final parity test)
 */
void cascade_generateBiconfString(ProcessBlock *pb) {
  int i; /* number of bits to be set */
  /* take care of the full bits */
  for (i = 0; i < wordIndex(pb->workbits); i++) { 
    /* get permuted bit */
    pb->testedBitsMarker[i] = 
        rnd_getPrngValue2_32(&pb->rngState) & 
        pb->permuteBufPtr[i]; 
  }
  /* prepare last few bits */
  pb->testedBitsMarker[wordIndex(pb->workbits)] = 
      rnd_getPrngValue2_32(&pb->rngState) & 
      uint32AllOnesExceptLastN(modulo32(pb->workbits - 1)) &
      pb->permuteBufPtr[wordIndex(pb->workbits)];
  return;
}

/** @brief Helper function to prepare a parity list of a given pass in a block, compare
   it with the received list & return number of differing bits
   
  @param pb processblock pointer
  @param pass pass 
  @return Number of differing bits  
 */
int cascade_prepParityListAndDiffs(ProcessBlock *pb, int pass) {
  int numberofbits = 0;
  int i, partitions;          /* counting index, num of blocks */
  unsigned int *lp, *rp, *pd; /* local/received & diff parity pointer */
  unsigned int *d;            /* for paritylist */
  int k;
  switch (pass) {
    case 0:
      k = pb->k0;
      d = pb->mainBufPtr;
      lp = pb->lp0;
      rp = pb->rp0;
      pd = pb->pd0;
      partitions = pb->partitions0;
      break;
    case 1:
      k = pb->k1;
      d = pb->permuteBufPtr;
      lp = pb->lp1;
      rp = pb->rp1;
      pd = pb->pd1;
      partitions = pb->partitions1;
      break;
    default: /* wrong index */
      return -1;
  }
  helper_prepParityList(d, lp, k, pb->workbits); /* prepare bitlist */

  /* evaluate parity mismatch  */
  for (i = 0; i < wordCount(partitions); i++) {
    pd[i] = lp[i] ^ rp[i];
    numberofbits += countSetBits(pd[i]);
  }
  return numberofbits;
}

/** @brief Helper program to half parity difference intervals. 
 * 
 * no weird stuff should happen.
 * 
 * @param pb processblock pointer
 * @param inh_idx inh_index 
  * @return Number of initially dead intervals 
  */
void cascade_fixParityIntervals(ProcessBlock *pb, unsigned int *inh_idx) {
  int i, firstBitIndex, lastBitIndex;                       /* running index */
  for (i = 0; i < pb->diffBlockCount; i++) { /* go through all different blocks */
    firstBitIndex = pb->diffidx[i];
    lastBitIndex = pb->diffidxe[i]; /* old bitindices */
    if (firstBitIndex >  lastBitIndex) {
      /* was already old */
      continue;
    }
    if (inh_idx[wordIndex(i)] & uint32AllZeroExceptAtN(i)) { /* error is in upper (par match) */
      pb->diffidx[i] = firstBitIndex + (lastBitIndex - firstBitIndex + 1) / 2; /* take upper half */
    } else {
      pb->diffidxe[i] = firstBitIndex + (lastBitIndex - firstBitIndex + 1) / 2 - 1; /* take lower half */
    }
  }
}

/**
 * @brief Function to initiate a BICONF procedure on Bob side. 
 * 
 * cascade_generateBiconfString into pb, then
 * sends out a package calling for a BICONF reply.
 * 
 * @param pb processblock pointer
 * @return error code, 0 if success
 */
int cascade_initiateBiconf(ProcessBlock *pb) {
  EcPktHdr_CascadeBiconfInitReq *h6;           /* header for that message */
  int errorCode = 0;
  unsigned int seed;                          /* seed for permutation */
  errorCode = rnd_generateRngSeed(&seed);
  if (errorCode) return errorCode;

  /* update state variables */
  pb->biconfLength = pb->workbits; /* old was /2 - do we still need this? */
  pb->rngState = seed;

  /* generate local bit string for test mask */
  cascade_generateBiconfString(pb);

  /* fill message */
  errorCode = comms_createEcHeader((char **)&h6, SUBTYPE_CASCADE_BICONF_INIT_REQ, 0, pb);
  if (errorCode) return 60; // Hardcoded in original impl.
  h6->seed = seed;
  h6->number_of_bits = pb->biconfLength;
  pb->binarySearchDepth = 0; /* keep it to main buffer TODO: is this relevant? */

  /* submit message */
  comms_insertSendPacket((char *)h6, h6->base.totalLengthInBytes);
  return 0;
}

/**
 * @brief Start the parity generation process on Alice side.
 * 
 * Should create a BICONF response message
 * 
 * @param receivebuf input message
 * @return error code, 0 if success
 */
int cascade_generateBiconfReply(ProcessBlock *pb, char *receivebuf) {
  EcPktHdr_CascadeBiconfInitReq *in_head = (EcPktHdr_CascadeBiconfInitReq *)receivebuf; /* holds received message header */
  EcPktHdr_CascadeBiconfParityResp *h7;      /* holds response message header */
  int errorCode = 0;

  /* update processblock status */
  switch (pb->processingState) {
    case PSTATE_PERFORMED_PARITY:                /* just finished BICONF */
      pb->processingState = PSTATE_DOING_BICONF; /* update state */
      pb->biconfRound = 0;                   /* first round */
      break;
    case PSTATE_DOING_BICONF: /* already did a biconf */
      pb->biconfRound++;  /* increment processing round; more checks? */
      break;
  }
  /* extract number of bits and seed */
  pb->biconfLength = in_head->number_of_bits; /* do more checks? */
  pb->rngState = in_head->seed;    /* check for 0?*/

  /* prepare permutation list */
  /* old: helper_prepPermutationCore(pb); */

  /* generate local (alice) version of test bit section */
  cascade_generateBiconfString(pb);

  /* fill the response header */
  errorCode = comms_createEcHeader((char **)&h7, SUBTYPE_CASCADE_BICONF_PARITY_RESP, 0, pb);
  if (errorCode) {
    return 61; // This was hardcoded in the original implementation
  }

  /* evaluate the parity (updated to use testbit buffer */
  h7->parity = helper_singleLineParity(pb->testedBitsMarker, 0, in_head->number_of_bits - 1);

  /* update bitloss */
  pb->leakageBits++; /* one is lost */

  /* send out response header */
  comms_insertSendPacket((char *)h7, h7->base.totalLengthInBytes);

  return 0; /* return nicely */
}

/**
 * @brief Function to generate a single binary search request for a biconf cycle.
 * 
 * Takes currently the subset of the biconf subset and its complement, which
   is not very efficient: The second error could have been found using the
   unpermuted short sample with nuch less bits.
 * On success, a binarysearch packet gets emitted with 2 list entries.
 * 
 * @param pb processblock ptr
 * @param biconfLength length of biconf
 * @return error code, 0 if success. 
 */
int cascade_makeBiconfBinSearchReq(ProcessBlock *pb, int biconfLength) {
  unsigned int msg5BodySize;          /* size of message */
  EcPktHdr_CascadeBinSearchMsg *h5;       /* pointer to first message */
  int errorCode = 0;
  unsigned int *h5Data, *h5Index; /* data pointers */

  pb->diffBlockCount = 1;
  pb->diffidx[0] = 0;
  pb->diffidxe[0] = biconfLength - 1;
  /* obsolete: pb->diffidx[1]=biconfLength;pb->diffidxe[1]=pb->workbits-1; */

  pb->binarySearchDepth = RUNLEVEL_SECONDPASS; /* only pass 1 */

  /* prepare message buffer for first binsearch message  */
  msg5BodySize = 3 * WORD_SIZE; /* parity data need, and indexing need for selection and compl */
  errorCode = comms_createEcHeader((char **)&h5, SUBTYPE_CASCADE_BIN_SEARCH_MSG, msg5BodySize, pb);
  if (errorCode) return errorCode;
  h5Data = (unsigned int *)&h5[1]; /* start of data */
  h5->number_entries = pb->diffBlockCount;
  h5->indexPresent = 4; /* NEW this round we have a start/stop table */

  /* keep local status and indicate the BICONF round to Alice */
  h5->runlevel = pb->binarySearchDepth | RUNLEVEL_BICONF;

  /* prepare block index list of simple type 1, uncompressed uint32 */
  h5Index = &h5Data[1];

  /* for index mode 4: */
  h5Index[0] = 0; /* selected first bits */

  /* this information is IMPLICIT in the round 4 information and needs no transmission */
  /* h5Index[2]=biconfLength; h5Index[3] = pb->workbits-biconfLength-1;  */

  /* set the very first bit as parity */
  h5Data[0] = (helper_singleLineParity(pb->testedBitsMarker, 0, biconfLength / 2 - 1) << 31);

  /* increment lost bits */
  pb->leakageBits += 1;

  /* send out message */
  comms_insertSendPacket((char *)h5, h5->base.totalLengthInBytes);

  return 0;
}

/**
 * @brief Function to prepare the first message head (header_5) for a binary search.
 * 
 *  This assumes
   that all the parity buffers have been malloced and the remote parities
   reside in the proper arrays. This function will be called several times for
   different passes; it expexts local parities to be evaluated already.
 * 
 * @param processBlock processblock ptr
 * @param pass pass number
 * @return int 0 on success, error code otherwise
 */
int cascade_prepFirstBinSearchMsg(ProcessBlock *processBlock, int pass) {
  int i, j;                             /* index variables */
  int k;                                /* length of processblocks */
  unsigned int *pd;                     /* parity difference bitfield pointer */
  unsigned int msg5BodySize;            /* size of message excluding header */
  EcPktHdr_CascadeBinSearchMsg *h5;             /* pointer to first message */
  unsigned int *h5Data, *h5Index;       /* data pointers */
  unsigned int *d;                      /* temporary pointer on parity data */
  unsigned int resbuf;                  /* parity determination variables */
  int kdiff, firstBitIndex, lastBitIndex;                 /* working variables for parity eval */
  int partitions; /* local partitions o go through for diff idx */

  switch (pass) { /* sort out specifics */
    case 0:       /* unpermutated pass */
      pd = processBlock->pd0;                 k = processBlock->k0;
      partitions = processBlock->partitions0; d = processBlock->mainBufPtr; /* unpermuted key */
      break;
    case 1: /* permutated pass */
      pd = processBlock->pd1;                 k = processBlock->k1;
      partitions = processBlock->partitions1; d = processBlock->permuteBufPtr; /* permuted key */
      break;
    default: 
      return 59; /* illegal pass arg */
  }

  /* fill difference index memory */
  j = 0; /* index for mismatching blocks */
  for (i = 0; i < partitions; i++) {
    if (uint32AllZeroExceptAtN(i) & pd[wordIndex(i)]) {       /* this block is mismatched */
      processBlock->diffidx[j] = i * k;                               /* store bit index, not block index */
      processBlock->diffidxe[j] = processBlock->diffidx[j] + (k - 1); /* last block */
      j++;
    }
  }
  /* mark pass/round correctly in processBlock */
  processBlock->binarySearchDepth = (pass == 0) ? RUNLEVEL_FIRSTPASS : RUNLEVEL_SECONDPASS; /* first round */

  // Note: duplicate-ish code here with comms_makeBinSrchMsgHead
  /* prepare message buffer for first binsearch message  */
  // Parity need + indexing need
  msg5BodySize = 
      (wordCount(processBlock->diffBlockCount) + processBlock->diffBlockCount) * WORD_SIZE; /* parity data need */
  i = comms_createEcHeader((char **)&h5, SUBTYPE_CASCADE_BIN_SEARCH_MSG, msg5BodySize, processBlock);
  if (i) return 55;
  h5->number_entries = processBlock->diffBlockCount;
  h5->runlevel = processBlock->binarySearchDepth; /* keep local status */
  h5->indexPresent = 1;              /* this round we have an index table */

  h5Data = (unsigned int *)&h5[1]; /* start of data */

  /* prepare block index list of simple type 1, uncompressed uint32 */
  h5Index = &h5Data[wordCount(processBlock->diffBlockCount)];
  for (i = 0; i < processBlock->diffBlockCount; i++) 
    h5Index[i] = processBlock->diffidx[i];

  /* prepare parity results */
  // this is very similar to helper_prepParityList, but difference is kdiff is variable in this case
  resbuf = 0;
  for (i = 0; i < processBlock->diffBlockCount; i++) { /* go through all processblocks */
    kdiff = processBlock->diffidxe[i] - processBlock->diffidx[i] + 1; /* left length */
    firstBitIndex = processBlock->diffidx[i];
    lastBitIndex = firstBitIndex + kdiff / 2 - 1; /* first and last bitidx */
    resbuf = (resbuf << 1) + helper_singleLineParity(d, firstBitIndex, lastBitIndex);
    if ((modulo32(i)) == 31) {
      h5Data[wordIndex(i)] = resbuf;
    }
  }
  if (modulo32(i))
    h5Data[wordIndex(i)] = resbuf << (32 - modulo32(i)); /* last parity bits */

  /* increment lost bits */
  processBlock->leakageBits += processBlock->diffBlockCount;

  /* send out message */
  return comms_insertSendPacket((char *)h5, h5->base.totalLengthInBytes);
}

/**
 * @brief Function to proceed with the parity evaluation message.
 * 
 * This function should start the Binary search machinery.
 * Should spit out the first binary search message
 * 
 * @param receivebuf
 * @return error code, 0 if success 
 */
int cascade_startBinSearch(ProcessBlock *pb, char *receivebuf) {
  EcPktHdr_CascadeParityList *in_head; /* holds received message header */
  int l0, l1;                    /* helpers;  number of words for bitarrays */

  /* get pointers for header...*/
  in_head = (EcPktHdr_CascadeParityList *)receivebuf;

  /* prepare local parity info */
  pb->rngState = in_head->seed; /* new rng seed */
  helper_prepPermutationWrapper(pb);       /* also updates workbits */

  /* update partition numbers and leakageBits */
  pb->partitions0 = (pb->workbits + pb->k0 - 1) / pb->k0;
  pb->partitions1 = (pb->workbits + pb->k1 - 1) / pb->k1;

  /* freshen up internal info on bit numbers etc */
  pb->leakageBits += pb->partitions0 + pb->partitions1;

  /* prepare parity list and difference buffers  */
  l0 = wordCount(pb->partitions0);
  l1 = wordCount(pb->partitions1); /* size in words */
  pb->lp0 = (unsigned int *)malloc2((l0 + l1) * WORD_SIZE * 3);
  if (!pb->lp0) return 53; /* can't malloc */
  pb->lp1 = &pb->lp0[l0];  /* ptr to permuted parities */
  pb->rp0 = &pb->lp1[l1];  /* prt to rmt parities 0 */
  pb->rp1 = &pb->rp0[l0];  /* prt to rmt parities 1 */
  pb->pd0 = &pb->rp1[l1];  /* prt to rmt parities 0 */
  pb->pd1 = &pb->pd0[l0];  /* prt to rmt parities 1 */

  /* store received parity lists as a direct copy into the rp structure */
  memcpy(pb->rp0, &in_head[1], /* this is the start of the data section */
         (l0 + l1) * WORD_SIZE);

  /* fill local parity list, get the number of differences */
  pb->diffBlockCount = cascade_prepParityListAndDiffs(pb, 0);
  if (pb->diffBlockCount == -1) 
    return 74;
  pb->diffBlockCountMax = pb->diffBlockCount;

  /* reserve difference index memory for pass 0 */
  pb->diffidx = (unsigned int *)malloc2(pb->diffBlockCount * WORD_SIZE * 2);
  if (!pb->diffidx) /* can't malloc */
    return 54;
  pb->diffidxe = &pb->diffidx[pb->diffBlockCount]; /* end of interval */

  /* now hand over to the procedure preoaring the first binsearch msg
     for the first pass 0 */

  return cascade_prepFirstBinSearchMsg(pb, 0);
}

// CASCADE BICONF MAIN FUNCTIONS
/* ------------------------------------------------------------------------- */
/**
 * @brief Function to process a binarysearch request on alice identity. 
 * 
 * Installs the difference index list in the first run, and performs the parity checks in
 *  subsequent runs. should work with both passes now.
 * 
 * - work in progress, need do fix bitloss in last round
 * 
 * Note that this marks a packet to be sent by comms_insertSendPacket.
 * 
 * @param pb processblock ptr
 * @param in_head header for incoming request
 * @return error code, 0 if success
 */
int cascade_initiatorAlice_processBinSearch(ProcessBlock *pb, EcPktHdr_CascadeBinSearchMsg *in_head) {
  unsigned int *inh_data, *inh_idx;
  int i;
  EcPktHdr_CascadeBinSearchMsg *outgoingMsgHead; /* for reply message */
  unsigned int *outParity;       /* pointer to outgoing parity result info */
  unsigned int *outMatch;        /* pointer to outgoing matching info */
  unsigned int *d;                /* points to internal key data */
  int k;                          /* keeps blocklength */
  unsigned int matchResult = 0, parityResult = 0; /* for builduing outmsg */
  int firstBitIndex, lastBitIndex, midBitIndex;                  /* for parity evaluation */
  int lostBitsCount; /* number of key bits revealed in this round */
  int tmpSingleLineParity = 0;

  inh_data = (unsigned int *)&in_head[1]; /* parity pattern */

  /* find out if difference index should be installed */
  while (in_head->indexPresent) {
    if (pb->diffidx) { /* there is already a diffindex */
      if (pb->diffBlockCountMax >= in_head->number_entries) {
        /* just re-assign */
        pb->diffBlockCount = in_head->number_entries;
        break;
      }
      /* otherwise: not enough space; remove the old one... */
      free2(pb->diffidx);
      /* ....and continue re-assigning... */
    }
    /* allocate difference idx memory */
    pb->diffBlockCount = in_head->number_entries; /* from far cons check? */
    pb->diffBlockCountMax = pb->diffBlockCount;
    pb->diffidx = (unsigned int *)malloc2(pb->diffBlockCount * WORD_SIZE * 2);
    if (!pb->diffidx) return 54;                 /* can't malloc */
    pb->diffidxe = &pb->diffidx[pb->diffBlockCount]; /* end of interval */
    break;
  }

  inh_idx = &inh_data[wordCount(pb->diffBlockCount)]; /* index or matching part */

  /* sort out pass-dependent variables */
  if (in_head->runlevel & RUNLEVEL_LEVELMASK) { /* this is pass 1 */
    d = pb->permuteBufPtr;
    k = pb->k1;
  } else { /* this is pass 0 */
    d = pb->mainBufPtr;
    k = pb->k0;
  }

  /* special case to take care of if this is a BICONF localizing round:
     the variables d and k contain worng values at this point.
     this is taken care now */
  if (in_head->runlevel & RUNLEVEL_BICONF) {
    d = pb->testedBitsMarker;
    k = pb->biconfLength;
  }

  /* fix index list according to parity info or initial run */
  switch (in_head->indexPresent) { /* different encodings */
    case 0: /* repair index according to previous basis match */
      cascade_fixParityIntervals(pb, inh_idx);
      break;
    case 1: /* simple unsigned int encoding */
      for (i = 0; i < pb->diffBlockCount; i++) {
        pb->diffidx[i] = inh_idx[i];            /* store start bit index */
        pb->diffidxe[i] = inh_idx[i] + (k - 1); /* last bit */
      }
      break;
    case 4: /* only one entry; from biconf run. should end be biconflen? */
      pb->diffidx[0] = inh_idx[0];
      pb->diffidxe[0] = pb->workbits - 1;
      break;
      /* should have a case 3 here for direct bit encoding */
    default: /* do not know encoding */
      return 57;
  }

  /* other stuff in local keyblk to update */
  pb->leakageBits += pb->diffBlockCount; /* for incoming parity bits */
  /* check if this masking is correct? let biconf status survive  */
  pb->binarySearchDepth =
      ((in_head->runlevel + 1) & RUNLEVEL_ROUNDMASK) +
      (in_head->runlevel & (RUNLEVEL_LEVELMASK | RUNLEVEL_BICONF));

  /* prepare outgoing message header */
  outgoingMsgHead = comms_makeBinSrchMsgHead(pb, 0);
  if (!outgoingMsgHead) return 58;
  outParity = (unsigned int *)&outgoingMsgHead[1];
  outMatch = &outParity[wordCount(pb->diffBlockCount)];

  lostBitsCount = pb->diffBlockCount; /* to keep track of lost bits */

  /* go through all entries */
  for (i = 0; i < pb->diffBlockCount; i++) {
    parityResult <<= 1;
    matchResult <<= 1; /* make more room */
    /* first, determine parity on local inverval */
    firstBitIndex = pb->diffidx[i];
    lastBitIndex = pb->diffidxe[i];            /* old bitindices */
    if (firstBitIndex >  lastBitIndex) {       /* this is an empty message */
      lostBitsCount -= 2;
      // goto skpar2;
    } else if (firstBitIndex == lastBitIndex) {
      lostBitsCount -= 2;           /* one less lost on receive, 1 not sent */
      pb->diffidx[i] = firstBitIndex + 1; /* mark as emty */
      // goto skpar2;              /* no parity eval needed */
    } else {
      // Update bits
      midBitIndex = firstBitIndex + (lastBitIndex - firstBitIndex + 1) / 2 - 1; /* new lower mid bitidx */
      tmpSingleLineParity = helper_singleLineParity(d, firstBitIndex, midBitIndex);
      if (((inh_data[wordIndex(i)] & uint32AllZeroExceptAtN(i)) ? 1 : 0) == tmpSingleLineParity) {
        /* same parity, take upper half */
        firstBitIndex = midBitIndex + 1;
        pb->diffidx[i] = firstBitIndex; /* update first bit idx */
        matchResult |= 1;     /* match with incoming parity (take upper) */
      } else {
        lastBitIndex = midBitIndex;
        pb->diffidxe[i] = lastBitIndex; /* update last bit idx */
      }

      /* test overlap.... */
      if (firstBitIndex == lastBitIndex) {
        lostBitsCount--; /* one less lost */
        // goto skpar2; /* no parity eval needed */
      } else {
        /* now, prepare new parity bit */
        midBitIndex = firstBitIndex + (lastBitIndex - firstBitIndex + 1) / 2 - 1; /* new lower mid bitidx */
        tmpSingleLineParity = helper_singleLineParity(d, firstBitIndex, midBitIndex);
        /* if not the end of interval, save parity */
        // this is because if end of interval, parity = 0, and parityResult |= 0 does nothing
        if (lastBitIndex >= midBitIndex) {
          parityResult |= tmpSingleLineParity; /* save parity */
        }
      }
    }
  // skpar2:
    if ((modulo32(i)) == 31) { /* save stuff in outbuffers */
      outMatch[wordIndex(i)] = matchResult;
      outParity[wordIndex(i)] = parityResult;
    }
  }
  /* cleanup residual bit buffers */
  if (modulo32(i) != 0) {
    outMatch[wordIndex(i)] = matchResult << (32 - modulo32(i));
    outParity[wordIndex(i)] = parityResult << (32 - modulo32(i));
  }

  /* update outgoing info leakageBits */
  pb->leakageBits += lostBitsCount;

  /* mark message for sending */
  return comms_insertSendPacket((char *)outgoingMsgHead, outgoingMsgHead->base.totalLengthInBytes);
}

/** @brief Bob's function to process a binarysearch request. 
 * 
 * Checks parity
   lists and does corrections if necessary.
   initiates the next step (BICONF on pass 1) for the next round if ready.

   Note: uses globalvar biconfRounds

 * @param pb processblock ptr
 * @param in_head header of incoming type-5 ec packet
 * @return error code, 0 if success 
 */
int cascade_followerBob_processBinSearch(ProcessBlock *pb, EcPktHdr_CascadeBinSearchMsg *in_head) {
  unsigned int *inh_data, *inh_idx;
  int i;
  EcPktHdr_CascadeBinSearchMsg *outgoingMsgHead; /* for reply message */
  unsigned int *outParity;       /* pointer to outgoing parity result info */
  unsigned int *outMatch;        /* pointer to outgoing matching info */
  unsigned int *d = NULL;         /* points to internal key data */
  unsigned int *d2 = NULL; /* points to secondary to-be-corrected buffer */
  unsigned int matchResult = 0, parityResult = 0; /* for builduing outmsg */
  int firstBitIndex, lastBitIndex, midBitIndex;                  /* for parity evaluation */
  int lostBitsCount;  /* number of key bits revealed in this round */
  int thispass;   /* indincates the current pass */
  int biconfmark; /* indicates if this is a biconf round */
  int tmpSingleLineParity = 0;

  inh_data = (unsigned int *)&in_head[1];          /* parity pattern */
  inh_idx = &inh_data[wordCount(pb->diffBlockCount)]; /* index or matching part */

  /* repair index according to previous basis match */
  cascade_fixParityIntervals(pb, inh_idx);

  /* other stuff in local keyblk to update */
  pb->leakageBits += pb->diffBlockCount;           /* for incoming parity bits */
  pb->binarySearchDepth = in_head->runlevel + 1; /* better some checks? */

  /* prepare outgoing message header */
  outgoingMsgHead = comms_makeBinSrchMsgHead(pb, 0);
  if (!outgoingMsgHead) return 58;
  outParity = (unsigned int *)&outgoingMsgHead[1];
  outMatch = &outParity[(wordCount(pb->diffBlockCount))];

  /* initially we will loose those for outgoing parity bits */
  lostBitsCount = pb->diffBlockCount;

  /* make pass-dependent settings */
  thispass = (pb->binarySearchDepth & RUNLEVEL_LEVELMASK) ? 1 : 0;

  if (thispass == 0) {
    d = pb->mainBufPtr;     // level 0
  } else if (thispass == 1) {
    d = pb->permuteBufPtr;  // level 1
  }

  biconfmark = 0; /* default is no biconf */

  /* select test buffer in case this is a BICONF test round */
  if (pb->binarySearchDepth & RUNLEVEL_BICONF) {
    biconfmark = 1;
    d = pb->testedBitsMarker;
    d2 = pb->permuteBufPtr; /* for repairing also the permuted buffer */
  }

  /* go through all entries */
  for (i = 0; i < pb->diffBlockCount; i++) {
    matchResult <<= 1;
    parityResult <<= 1; /* make room for next bits */
    /* first, determine parity on local inverval */
    firstBitIndex = pb->diffidx[i];
    lastBitIndex = pb->diffidxe[i]; /* old bitindices */

    /* If this is an empty message , don't count or correct */
    if (firstBitIndex >  lastBitIndex) {  
      lostBitsCount -= 2;  /* No initial parity, no outgoing */
      goto skipparity; /* no more parity evaluation, skip rest */
    }
    /* If we have found the bit error */
    if (firstBitIndex == lastBitIndex) {
      if (biconfmark) flipBit(d2, firstBitIndex);
      flipBit(d, firstBitIndex);
      pb->correctedErrors++;
      lostBitsCount -= 2;           /* No initial parity, no outgoing */
      pb->diffidx[i] = firstBitIndex + 1; /* mark as emty */
      goto skipparity;          /* no more parity evaluation, skip rest */
    }

    midBitIndex = firstBitIndex + (lastBitIndex - firstBitIndex + 1) / 2 - 1; /* new lower mid bitidx */
    tmpSingleLineParity = helper_singleLineParity(d, firstBitIndex, midBitIndex);
    if (((inh_data[wordIndex(i)] & uint32AllZeroExceptAtN(i)) ? 1 : 0) == tmpSingleLineParity) {
      /* same parity, take upper half */
      firstBitIndex = midBitIndex + 1;
      pb->diffidx[i] = firstBitIndex; /* update first bit idx */
      matchResult |= 1;     /* indicate match with incoming parity */
    } else {
      lastBitIndex = midBitIndex;
      pb->diffidxe[i] = lastBitIndex; /* update last bit idx */
    }
    /* If this is end of interval, correct for error */
    if (firstBitIndex == lastBitIndex) {
      if (biconfmark) flipBit(d2, firstBitIndex);
      flipBit(d, firstBitIndex);
      pb->correctedErrors++;
      lostBitsCount--; /* we don't reveal anything on this one anymore */
      goto skipparity;
    }
    /* Else, prepare new parity bit */
    midBitIndex = firstBitIndex + (lastBitIndex - firstBitIndex + 1) / 2 - 1; /* new lower mid bitidx */
    parityResult |= helper_singleLineParity(d, firstBitIndex, midBitIndex); /* save parity */

  skipparity:
    if ((modulo32(i)) == 31) {
      /* save stuff in outbuffers */
      outMatch[wordIndex(i)] = matchResult;
      outParity[wordIndex(i)] = parityResult;
    }
  }
  /* Cleanup residual bit buffers */
  if (modulo32(i) != 0) {
    outMatch[wordIndex(i)] = matchResult << (32 - modulo32(i));
    outParity[wordIndex(i)] = parityResult << (32 - modulo32(i));
  }

  /* a blocklength k decides on a max number of rounds */
  if ((pb->binarySearchDepth & RUNLEVEL_ROUNDMASK) < log2Ceil((pb->processingState == PSTATE_DOING_BICONF)
      ? (pb->biconfLength) : (thispass ? pb->k1 : pb->k0))) {
    /* need to continue with this search; make packet 5 ready to send */
    pb->leakageBits += lostBitsCount;
    comms_insertSendPacket((char *)outgoingMsgHead, outgoingMsgHead->base.totalLengthInBytes);
    return 0;
  }

  pb->leakageBits += lostBitsCount; /* correction for unreceived parity bits and
                                   nonsent parities */

  /* cleanup changed bits in the other permuted field */
  fix_permutedbits(pb);

  /* prepare for alternate round; start with re-evaluation of parity. */
  while (True) { /* just a break construction.... */
    pb->binarySearchDepth = thispass ? RUNLEVEL_FIRSTPASS : RUNLEVEL_SECONDPASS;
    pb->diffBlockCount = cascade_prepParityListAndDiffs(pb, 1 - thispass); /* new differences */
    /* error if wrong pass */
    if (pb->diffBlockCount == -1) 
      return 74;
    
    // break if no more errors in block
    if ((pb->diffBlockCount == 0) && (thispass == 1)) 
      break;

    // if more space needed
    if (pb->diffBlockCount > pb->diffBlockCountMax) { 
      // Reassign the diff buffer
      free2(pb->diffidx);
      pb->diffBlockCountMax = pb->diffBlockCount;
      pb->diffidx = (unsigned int *)malloc2(pb->diffBlockCount * WORD_SIZE * 2);
      // Throw error if cannot malloc
      if (!pb->diffidx) 
        return 54; 
      // end of interval
      pb->diffidxe = &pb->diffidx[pb->diffBlockCount];
    }

    /* do basically a cascade_startBinSearch for next round */
    return cascade_prepFirstBinSearchMsg(pb, 1 - thispass);
  }

  /* now we have finished a consecutive the second round; there are no more errors in both passes.  */

  /* check for biconf reply */
  if (pb->processingState == PSTATE_DOING_BICONF) { 
    /* we are finally finished with the  BICONF corrections */
    /* update biconf status */
    pb->biconfRound++;

    /* generate new biconf request */
    if (pb->biconfRound < arguments.biconfRounds) {
      return cascade_initiateBiconf(pb); /* request another one */
    }
    /* initiate the privacy amplificaton */
    return privAmp_sendPrivAmpMsgAndPrivAmp(pb);
  }

  /* we have no more errors in both passes, and we were not yet
     in BICONF mode */

  /* initiate the BICONF state */
  pb->processingState = PSTATE_DOING_BICONF;
  pb->biconfRound = 0; /* first BICONF round */
  return cascade_initiateBiconf(pb);
}

/** @brief Start the parity generation process on bob's side. 
 * 
   Should either initiate a binary search, re-issue a BICONF request or
   continue to the parity evaluation. 

  * @param receivebuf parity reply from Alice
  * @return error message, 0 if success
  */
int cascade_receiveBiconfReply(ProcessBlock *pb, char *receivebuf) {
  EcPktHdr_CascadeBiconfParityResp *in_head  = (EcPktHdr_CascadeBiconfParityResp *)receivebuf; /* holds received message header */
  int localparity;

  pb->binarySearchDepth = RUNLEVEL_SECONDPASS; /* use permuted buf */

  /* update incoming bit leakage */
  pb->leakageBits++;

  /* evaluate local parity */
  localparity = helper_singleLineParity(pb->testedBitsMarker, 0, pb->biconfLength - 1);

  /* eventually start binary search */
  if (localparity != in_head->parity) {
    return cascade_makeBiconfBinSearchReq(pb, pb->biconfLength);
  }
  /* this location gets ONLY visited if there is no error in BICONF search */

  /* update biconf status */
  pb->biconfRound++;

  /* eventully generate new biconf request */
  if (pb->biconfRound < arguments.biconfRounds) {
    return cascade_initiateBiconf(pb); /* request another one */
  }
  /* initiate the privacy amplificaton */
  return privAmp_sendPrivAmpMsgAndPrivAmp(pb);
}

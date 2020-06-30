#include "cascade_biconf.h"

// PERMUTATIONS HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */
/** @brief Helper to fix the permuted/unpermuted bit changes.
  
  Decides via a parameter in kb->binarySearchDepth MSB what polarity to take.

  @param kb ptr to processblock to fix */
void fix_permutedbits(ProcessBlock *kb) {
  int i, k;
  unsigned int *src, *dst;
  unsigned short *idx; /* pointers to data loc and permute idx */
  if (kb->binarySearchDepth & RUNLEVEL_LEVELMASK) { /* we are in pass 1 */
    src = kb->permuteBufPtr;
    dst = kb->mainBufPtr;
    idx = kb->reverseIndex;
  } else { /* we are in pass 0 */
    src = kb->mainBufPtr;
    dst = kb->permuteBufPtr;
    idx = kb->permuteIndex;
  }
  bzero(dst, ((kb->workbits + 31) / 32) * 4); /* clear dest */
  for (i = 0; i < kb->workbits; i++) {
    k = idx[i]; /* permuted bit index */
    if (bt_mask(i) & src[i / 32]) dst[k / 32] |= bt_mask(k);
  }
  return;
}

// CASCADE BICONF HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */
/** @brief Helper function to generate a pseudorandom bit pattern into the test bit
   buffer. 
   
  @param kb processblock pointer (kb contains other params for final parity test)
  @param seed seed for the RNG
 */
void generate_selectbitstring(ProcessBlock *kb, unsigned int seed) {
  int i;                                    /* number of bits to be set */
  kb->rngState = seed;                     /* set new seed */
  for (i = 0; i < (kb->workbits) / 32; i++) /* take care of the full bits */
    kb->testedBitsMarker[i] = PRNG_value2_32(&kb->rngState);
  kb->testedBitsMarker[kb->workbits / 32] = /* prepare last few bits */
      PRNG_value2_32(&kb->rngState) & lastmask((kb->workbits - 1) & 31);
  return;
}

/** @brief Helper function to generate a pseudorandom bit pattern into the test bit
   buffer AND transfer the permuted key buffer into it.
     
   For a more compact parity generation in the last round.

   @param kb processblock pointer (kb contains other params for final parity test)
 */
void generate_BICONF_bitstring(ProcessBlock *kb) {
  int i; /* number of bits to be set */
  /* take care of the full bits */
  for (i = 0; i < (kb->workbits) / 32; i++) { 
    /* get permuted bit */
    kb->testedBitsMarker[i] = 
        PRNG_value2_32(&kb->rngState) & 
        kb->permuteBufPtr[i]; 
  }
  /* prepare last few bits */
  kb->testedBitsMarker[kb->workbits / 32] = 
      PRNG_value2_32(&kb->rngState) & 
      lastmask((kb->workbits - 1) & 31) &
      kb->permuteBufPtr[kb->workbits / 32];
  return;
}

/** @brief Helper function to prepare a parity list of a given pass in a block, compare
   it with the received list & return number of differing bits
   
  @param kb processblock pointer
  @param pass pass 
  @return Number of differing bits  
 */
int do_paritylist_and_diffs(ProcessBlock *kb, int pass) {
  int numberofbits = 0;
  int i, partitions;          /* counting index, num of blocks */
  unsigned int *lp, *rp, *pd; /* local/received & diff parity pointer */
  unsigned int *d;            /* for paritylist */
  int k;
  switch (pass) {
    case 0:
      k = kb->k0;
      d = kb->mainBufPtr;
      lp = kb->lp0;
      rp = kb->rp0;
      pd = kb->pd0;
      partitions = kb->partitions0;
      break;
    case 1:
      k = kb->k1;
      d = kb->permuteBufPtr;
      lp = kb->lp1;
      rp = kb->rp1;
      pd = kb->pd1;
      partitions = kb->partitions1;
      break;
    default: /* wrong index */
      return -1;
  }
  prepare_paritylist_basic(d, lp, k, kb->workbits); /* prepare bitlist */

  /* evaluate parity mismatch  */
  for (i = 0; i < ((partitions + 31) / 32); i++) {
    pd[i] = lp[i] ^ rp[i];
    numberofbits += countSetBits(pd[i]);
  }
  return numberofbits;
}

/** @brief Helper program to half parity difference intervals. 
 * 
 * no weired stuff should happen.
 * 
 * @param kb processblock pointer
 * @param inh_idx inh_index 
  * @return Number of initially dead intervals 
  */
void fix_parity_intervals(ProcessBlock *kb, unsigned int *inh_idx) {
  int i, fbi, lbi;                       /* running index */
  for (i = 0; i < kb->diffBlockCount; i++) { /* go through all different blocks */
    fbi = kb->diffidx[i];
    lbi = kb->diffidxe[i]; /* old bitindices */
    if (fbi > lbi) {
      /* was already old */
      continue;
    }
    if (inh_idx[i / 32] & bt_mask(i)) { /* error is in upper (par match) */
      kb->diffidx[i] = fbi + (lbi - fbi + 1) / 2; /* take upper half */
    } else {
      kb->diffidxe[i] = fbi + (lbi - fbi + 1) / 2 - 1; /* take lower half */
    }
  }
}

/** @brief Helper for correcting one bit in pass 0 or 1 in their field */
void correct_bit(unsigned int *d, int bitindex) {
  d[bitindex / 32] ^= bt_mask(bitindex); /* flip bit */
  return;
}

/** @brief Helper funtion to get a simple one-line parity from a large string, but
   this time with a mask buffer to be AND-ed on the string.

 * @param d pointer to the start of the string buffer
 * @param m pointer to the start of the mask buffer
 * @param start start index
 * @param end end index
 * @return parity (0 or 1)
 */
int singleLineParityMasked(unsigned int *d, unsigned int *m, int start,
                              int end) {
  unsigned int tmp_par, lm, fm;
  int li, fi, ri;
  fi = start / 32;
  li = end / 32;
  lm = lastmask(end & 31);
  fm = firstmask(start & 31);
  if (li == fi) {
    tmp_par = d[fi] & lm & fm & m[fi];
  } else {
    tmp_par = (d[fi] & fm & m[fi]) ^ (d[li] & lm & m[li]);
    for (ri = fi + 1; ri < li; ri++) tmp_par ^= (d[ri] & m[ri]);
  } /* tmp_par holds now a combination of bits to be tested */
  return parity(tmp_par);
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
 * @param kb processblock ptr
 * @param in_head header for incoming request
 * @return error code, 0 if success
 */
int process_binsearch_alice(ProcessBlock *kb, EcPktHdr_CascadeBinSearchMsg *in_head) {
  unsigned int *inh_data, *inh_idx;
  int i;
  EcPktHdr_CascadeBinSearchMsg *out_head; /* for reply message */
  unsigned int *out_parity;       /* pointer to outgoing parity result info */
  unsigned int *out_match;        /* pointer to outgoing matching info */
  unsigned int *d;                /* points to internal key data */
  int k;                          /* keeps blocklength */
  unsigned int matchresult = 0, parityresult = 0; /* for builduing outmsg */
  int fbi, lbi, mbi;                  /* for parity evaluation */
  int lost_bits; /* number of key bits revealed in this round */
  int tmpSingleLineParity = 0;

  inh_data = (unsigned int *)&in_head[1]; /* parity pattern */

  /* find out if difference index should be installed */
  while (in_head->indexPresent) {
    if (kb->diffidx) { /* there is already a diffindex */
      if (kb->diffBlockCountMax >= in_head->number_entries) {
        /* just re-assign */
        kb->diffBlockCount = in_head->number_entries;
        break;
      }
      /* otherwise: not enough space; remove the old one... */
      free2(kb->diffidx);
      /* ....and continue re-assigning... */
    }
    /* allocate difference idx memory */
    kb->diffBlockCount = in_head->number_entries; /* from far cons check? */
    kb->diffBlockCountMax = kb->diffBlockCount;
    kb->diffidx = (unsigned int *)malloc2(kb->diffBlockCount * sizeof(unsigned int) * 2);
    if (!kb->diffidx) return 54;                 /* can't malloc */
    kb->diffidxe = &kb->diffidx[kb->diffBlockCount]; /* end of interval */
    break;
  }

  inh_idx = &inh_data[(kb->diffBlockCount + 31) / 32]; /* index or matching part */

  /* sort out pass-dependent variables */
  if (in_head->runlevel & RUNLEVEL_LEVELMASK) { /* this is pass 1 */
    d = kb->permuteBufPtr;
    k = kb->k1;
  } else { /* this is pass 0 */
    d = kb->mainBufPtr;
    k = kb->k0;
  }

  /* special case to take care of if this is a BICONF localizing round:
     the variables d and k contain worng values at this point.
     this is taken care now */
  if (in_head->runlevel & RUNLEVEL_BICONF) {
    d = kb->testedBitsMarker;
    k = kb->biconflength;
  }

  /* fix index list according to parity info or initial run */
  switch (in_head->indexPresent) { /* different encodings */
    case 0: /* repair index according to previous basis match */
      fix_parity_intervals(kb, inh_idx);
      break;
    case 1: /* simple unsigned int encoding */
      for (i = 0; i < kb->diffBlockCount; i++) {
        kb->diffidx[i] = inh_idx[i];            /* store start bit index */
        kb->diffidxe[i] = inh_idx[i] + (k - 1); /* last bit */
      }
      break;
    case 4: /* only one entry; from biconf run. should end be biconflen? */
      kb->diffidx[0] = inh_idx[0];
      kb->diffidxe[0] = kb->workbits - 1;
      break;
      /* should have a case 3 here for direct bit encoding */
    default: /* do not know encoding */
      return 57;
  }

  /* other stuff in local keyblk to update */
  kb->leakageBits += kb->diffBlockCount; /* for incoming parity bits */
  /* check if this masking is correct? let biconf status survive  */
  kb->binarySearchDepth =
      ((in_head->runlevel + 1) & RUNLEVEL_ROUNDMASK) +
      (in_head->runlevel & (RUNLEVEL_LEVELMASK | RUNLEVEL_BICONF));

  /* prepare outgoing message header */
  out_head = makeMessageHead5(kb, 0);
  if (!out_head) return 58;
  out_parity = (unsigned int *)&out_head[1];
  out_match = &out_parity[(kb->diffBlockCount + 31) / 32];

  lost_bits = kb->diffBlockCount; /* to keep track of lost bits */

  /* go through all entries */
  for (i = 0; i < kb->diffBlockCount; i++) {
    parityresult <<= 1;
    matchresult <<= 1; /* make more room */
    /* first, determine parity on local inverval */
    fbi = kb->diffidx[i];
    lbi = kb->diffidxe[i]; /* old bitindices */
    if (fbi > lbi) {       /* this is an empty message */
      lost_bits -= 2;
      goto skpar2;
    }
    if (fbi == lbi) {
      lost_bits -= 2;           /* one less lost on receive, 1 not sent */
      kb->diffidx[i] = fbi + 1; /* mark as emty */
      goto skpar2;              /* no parity eval needed */
    }
    mbi = fbi + (lbi - fbi + 1) / 2 - 1; /* new lower mid bitidx */
    tmpSingleLineParity = singleLineParity(d, fbi, mbi);
    if (((inh_data[i / 32] & bt_mask(i)) ? 1 : 0) == tmpSingleLineParity) {
      /* same parity, take upper half */
      fbi = mbi + 1;
      kb->diffidx[i] = fbi; /* update first bit idx */
      matchresult |= 1;     /* match with incoming parity (take upper) */
    } else {
      lbi = mbi;
      kb->diffidxe[i] = lbi; /* update last bit idx */
    }

    /* test overlap.... */
    if (fbi == lbi) {
      lost_bits--; /* one less lost */
      goto skpar2; /* no parity eval needed */
    }

    /* now, prepare new parity bit */
    mbi = fbi + (lbi - fbi + 1) / 2 - 1; /* new lower mid bitidx */
    tmpSingleLineParity = singleLineParity(d, fbi, mbi);
    /* if not the end of interval, save parity */
    // this is because if end of interval, parity = 0, and parityresult |= 0 does nothing
    if (lbi >= mbi) {
      parityresult |= tmpSingleLineParity; /* save parity */
    }
  skpar2:
    if ((i & 31) == 31) { /* save stuff in outbuffers */
      out_match[i / 32] = matchresult;
      out_parity[i / 32] = parityresult;
    }
  }
  /* cleanup residual bit buffers */
  if (i & 31) {
    out_match[i / 32] = matchresult << (32 - (i & 31));
    out_parity[i / 32] = parityresult << (32 - (i & 31));
  }

  /* update outgoing info leakageBits */
  kb->leakageBits += lost_bits;

  /* mark message for sending */
  comms_insertSendPacket((char *)out_head, out_head->base.totalLengthInBytes);

  return 0;
}

/**
 * @brief Function to initiate a BICONF procedure on Bob side. 
 * 
 * generate_BICONF_bitstring into kb, then
 * sends out a package calling for a BICONF reply.
 * 
 * @param kb processblock pointer
 * @return error code, 0 if success
 */
int initiate_biconf(ProcessBlock *kb) {
  EcPktHdr_CascadeBiconfInitReq *h6; /* header for that message */
  unsigned int seed = generateRngSeed();  /* seed for permutation */
  int errorCode = 0;

  /* update state variables */
  kb->biconflength = kb->workbits; /* old was /2 - do we still need this? */
  kb->rngState = seed;

  /* generate local bit string for test mask */
  generate_BICONF_bitstring(kb);

  /* fill message */
  errorCode = comms_createHeader((char **)&h6, SUBTYPE_CASCADE_BICONF_INIT_REQ, 0, kb);
  if (errorCode) return 60; // Hardcoded in original impl.
  h6->seed = seed;
  h6->number_of_bits = kb->biconflength;
  kb->binarySearchDepth = 0; /* keep it to main buffer TODO: is this relevant? */

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
int generateBiconfReply(ProcessBlock *kb, char *receivebuf) {
  EcPktHdr_CascadeBiconfInitReq *in_head = (EcPktHdr_CascadeBiconfInitReq *)receivebuf; /* holds received message header */
  EcPktHdr_CascadeBiconfParityResp *h7;      /* holds response message header */
  int errorCode = 0;

  /* update processblock status */
  switch (kb->processingState) {
    case PRS_PERFORMEDPARITY1:                /* just finished BICONF */
      kb->processingState = PRS_DOING_BICONF; /* update state */
      kb->biconfRound = 0;                   /* first round */
      break;
    case PRS_DOING_BICONF: /* already did a biconf */
      kb->biconfRound++;  /* increment processing round; more checks? */
      break;
  }
  /* extract number of bits and seed */
  kb->biconflength = in_head->number_of_bits; /* do more checks? */
  kb->rngState = in_head->seed;    /* check for 0?*/

  /* prepare permutation list */
  /* old: prepare_permut_core(kb); */

  /* generate local (alice) version of test bit section */
  generate_BICONF_bitstring(kb);

  /* fill the response header */
  errorCode = comms_createHeader((char **)&h7, SUBTYPE_CASCADE_BICONF_PARITY_RESP, 0, kb);
  if (errorCode) {
    return 61; // This was hardcoded in the original implementation
  }

  /* evaluate the parity (updated to use testbit buffer */
  h7->parity = singleLineParity(kb->testedBitsMarker, 0, in_head->number_of_bits - 1);

  /* update bitloss */
  kb->leakageBits++; /* one is lost */

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
 * @param kb processblock ptr
 * @param biconflength length of biconf
 * @return error code, 0 if success. 
 */
int initiate_biconf_binarysearch(ProcessBlock *kb, int biconflength) {
  unsigned int msg5BodySize;          /* size of message */
  EcPktHdr_CascadeBinSearchMsg *h5;       /* pointer to first message */
  int errorCode = 0;
  unsigned int *h5_data, *h5_idx; /* data pointers */

  kb->diffBlockCount = 1;
  kb->diffidx[0] = 0;
  kb->diffidxe[0] = biconflength - 1;
  /* obsolete: kb->diffidx[1]=biconflength;kb->diffidxe[1]=kb->workbits-1; */

  kb->binarySearchDepth = RUNLEVEL_SECONDPASS; /* only pass 1 */

  /* prepare message buffer for first binsearch message  */
  msg5BodySize = 3 * sizeof(unsigned int); /* parity data need, and indexing need for selection and compl */
  errorCode = comms_createHeader((char **)&h5, SUBTYPE_CASCADE_BIN_SEARCH_MSG, msg5BodySize, kb);
  if (errorCode) return errorCode;
  h5_data = (unsigned int *)&h5[1]; /* start of data */
  h5->number_entries = kb->diffBlockCount;
  h5->indexPresent = 4; /* NEW this round we have a start/stop table */

  /* keep local status and indicate the BICONF round to Alice */
  h5->runlevel = kb->binarySearchDepth | RUNLEVEL_BICONF;

  /* prepare block index list of simple type 1, uncompressed uint32 */
  h5_idx = &h5_data[1];

  /* for index mode 4: */
  h5_idx[0] = 0; /* selected first bits */

  /* this information is IMPLICIT in the round 4 information and needs no transmission */
  /* h5_idx[2]=biconflength; h5_idx[3] = kb->workbits-biconflength-1;  */

  /* set parity */
  h5_data[0] = (singleLineParity(kb->testedBitsMarker, 0, biconflength / 2 - 1) << 31);

  /* increment lost bits */
  kb->leakageBits += 1;

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
int prepare_first_binsearch_msg(ProcessBlock *processBlock, int pass) {
  int i, j;                             /* index variables */
  int k;                                /* length of processblocks */
  unsigned int *pd;                     /* parity difference bitfield pointer */
  unsigned int msg5BodySize;            /* size of message excluding header */
  EcPktHdr_CascadeBinSearchMsg *h5;             /* pointer to first message */
  unsigned int *h5_data, *h5_idx;       /* data pointers */
  unsigned int *d;                      /* temporary pointer on parity data */
  unsigned int resbuf;                  /* parity determination variables */
  int kdiff, fbi, lbi;                 /* working variables for parity eval */
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
    if (bt_mask(i) & pd[i / 32]) {       /* this block is mismatched */
      processBlock->diffidx[j] = i * k;                               /* store bit index, not block index */
      processBlock->diffidxe[j] = processBlock->diffidx[j] + (k - 1); /* last block */
      j++;
    }
  }
  /* mark pass/round correctly in processBlock */
  processBlock->binarySearchDepth = (pass == 0) ? RUNLEVEL_FIRSTPASS : RUNLEVEL_SECONDPASS; /* first round */

  // Note: duplicate-ish code here with makeMessageHead5
  /* prepare message buffer for first binsearch message  */
  // Parity need + indexing need
  msg5BodySize = 
      (((processBlock->diffBlockCount + 31) / 32) + processBlock->diffBlockCount) * sizeof(unsigned int); /* parity data need */
  i = comms_createHeader((char **)&h5, SUBTYPE_CASCADE_BIN_SEARCH_MSG, msg5BodySize, processBlock);
  if (i) return 55;
  h5->number_entries = processBlock->diffBlockCount;
  h5->runlevel = processBlock->binarySearchDepth; /* keep local status */
  h5->indexPresent = 1;              /* this round we have an index table */

  h5_data = (unsigned int *)&h5[1]; /* start of data */

  /* prepare block index list of simple type 1, uncompressed uint32 */
  h5_idx = &h5_data[((processBlock->diffBlockCount + 31) / 32)];
  for (i = 0; i < processBlock->diffBlockCount; i++) 
    h5_idx[i] = processBlock->diffidx[i];

  /* prepare parity results */
  // this is very similar to prepare_paritylist_basic, but difference is kdiff is variable in this case
  resbuf = 0;
  for (i = 0; i < processBlock->diffBlockCount; i++) { /* go through all processblocks */
    kdiff = processBlock->diffidxe[i] - processBlock->diffidx[i] + 1; /* left length */
    fbi = processBlock->diffidx[i];
    lbi = fbi + kdiff / 2 - 1; /* first and last bitidx */
    resbuf = (resbuf << 1) + singleLineParity(d, fbi, lbi);
    if ((i & 31) == 31) {
      h5_data[i / 32] = resbuf;
    }
  }
  if (i & 31)
    h5_data[i / 32] = resbuf << (32 - (i & 31)); /* last parity bits */

  /* increment lost bits */
  processBlock->leakageBits += processBlock->diffBlockCount;

  /* send out message */
  comms_insertSendPacket((char *)h5, h5->base.totalLengthInBytes);

  return 0;
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
int start_binarysearch(ProcessBlock *kb, char *receivebuf) {
  EcPktHdr_CascadeParityList *in_head; /* holds received message header */
  int l0, l1;                    /* helpers;  number of words for bitarrays */

  /* get pointers for header...*/
  in_head = (EcPktHdr_CascadeParityList *)receivebuf;

  /* prepare local parity info */
  kb->rngState = in_head->seed; /* new rng seed */
  prepare_permutation(kb);       /* also updates workbits */

  /* update partition numbers and leakageBits */
  kb->partitions0 = (kb->workbits + kb->k0 - 1) / kb->k0;
  kb->partitions1 = (kb->workbits + kb->k1 - 1) / kb->k1;

  /* freshen up internal info on bit numbers etc */
  kb->leakageBits += kb->partitions0 + kb->partitions1;

  /* prepare parity list and difference buffers  */
  l0 = (kb->partitions0 + 31) / 32;
  l1 = (kb->partitions1 + 31) / 32; /* size in words */
  kb->lp0 = (unsigned int *)malloc2((l0 + l1) * 4 * 3);
  if (!kb->lp0) return 53; /* can't malloc */
  kb->lp1 = &kb->lp0[l0];  /* ptr to permuted parities */
  kb->rp0 = &kb->lp1[l1];  /* prt to rmt parities 0 */
  kb->rp1 = &kb->rp0[l0];  /* prt to rmt parities 1 */
  kb->pd0 = &kb->rp1[l1];  /* prt to rmt parities 0 */
  kb->pd1 = &kb->pd0[l0];  /* prt to rmt parities 1 */

  /* store received parity lists as a direct copy into the rp structure */
  memcpy(kb->rp0, &in_head[1], /* this is the start of the data section */
         (l0 + l1) * 4);

  /* fill local parity list, get the number of differences */
  kb->diffBlockCount = do_paritylist_and_diffs(kb, 0);
  if (kb->diffBlockCount == -1) 
    return 74;
  kb->diffBlockCountMax = kb->diffBlockCount;

  /* reserve difference index memory for pass 0 */
  kb->diffidx = (unsigned int *)malloc2(kb->diffBlockCount * sizeof(unsigned int) * 2);
  if (!kb->diffidx) /* can't malloc */
    return 54;
  kb->diffidxe = &kb->diffidx[kb->diffBlockCount]; /* end of interval */

  /* now hand over to the procedure preoaring the first binsearch msg
     for the first pass 0 */

  return prepare_first_binsearch_msg(kb, 0);
}

/** @brief Alice's wrapper function to process a binarysearch request.
 * 
 * Distinguishes between the two
   symmetries in the evaluation. This is onyl a wrapper.
   on alice side, it does a passive check; on bob side, it possibly corrects
   for errors. 
 * @param receivebuf 
 * @return error code, 0 if success 
 */
int process_binarysearch(ProcessBlock *kb, char *receivebuf) {
  EcPktHdr_CascadeBinSearchMsg *in_head = (EcPktHdr_CascadeBinSearchMsg *)receivebuf; /* holds received message header */

  if (kb->processorRole == INITIATOR) {
    return process_binsearch_alice(kb, in_head);
  } else if (kb->processorRole == FOLLOWER) {
    return process_binsearch_bob(kb, in_head);
  } else {
    return 56; // Illegal role
  }
}

/** @brief Bob's function to process a binarysearch request. 
 * 
 * Checks parity
   lists and does corrections if necessary.
   initiates the next step (BICONF on pass 1) for the next round if ready.

   Note: uses globalvar biconfRounds

 * @param kb processblock ptr
 * @param in_head header of incoming type-5 ec packet
 * @return error code, 0 if success 
 */
int process_binsearch_bob(ProcessBlock *kb, EcPktHdr_CascadeBinSearchMsg *in_head) {
  unsigned int *inh_data, *inh_idx;
  int i;
  EcPktHdr_CascadeBinSearchMsg *out_head; /* for reply message */
  unsigned int *out_parity;       /* pointer to outgoing parity result info */
  unsigned int *out_match;        /* pointer to outgoing matching info */
  unsigned int *d = NULL;         /* points to internal key data */
  unsigned int *d2 = NULL; /* points to secondary to-be-corrected buffer */
  unsigned int matchresult = 0, parityresult = 0; /* for builduing outmsg */
  int fbi, lbi, mbi;                  /* for parity evaluation */
  int lost_bits;  /* number of key bits revealed in this round */
  int thispass;   /* indincates the current pass */
  int biconfmark; /* indicates if this is a biconf round */
  int tmpSingleLineParity = 0;

  inh_data = (unsigned int *)&in_head[1];          /* parity pattern */
  inh_idx = &inh_data[(kb->diffBlockCount + 31) / 32]; /* index or matching part */

  /* repair index according to previous basis match */
  fix_parity_intervals(kb, inh_idx);

  /* other stuff in local keyblk to update */
  kb->leakageBits += kb->diffBlockCount;           /* for incoming parity bits */
  kb->binarySearchDepth = in_head->runlevel + 1; /* better some checks? */

  /* prepare outgoing message header */
  out_head = makeMessageHead5(kb, 0);
  if (!out_head) return 58;
  out_parity = (unsigned int *)&out_head[1];
  out_match = &out_parity[((kb->diffBlockCount + 31) / 32)];

  /* initially we will loose those for outgoing parity bits */
  lost_bits = kb->diffBlockCount;

  /* make pass-dependent settings */
  thispass = (kb->binarySearchDepth & RUNLEVEL_LEVELMASK) ? 1 : 0;

  if (thispass == 0) {
    d = kb->mainBufPtr;     // level 0
  } else if (thispass == 1) {
    d = kb->permuteBufPtr;  // level 1
  }

  biconfmark = 0; /* default is no biconf */

  /* select test buffer in case this is a BICONF test round */
  if (kb->binarySearchDepth & RUNLEVEL_BICONF) {
    biconfmark = 1;
    d = kb->testedBitsMarker;
    d2 = kb->permuteBufPtr; /* for repairing also the permuted buffer */
  }

  /* go through all entries */
  for (i = 0; i < kb->diffBlockCount; i++) {
    matchresult <<= 1;
    parityresult <<= 1; /* make room for next bits */
    /* first, determine parity on local inverval */
    fbi = kb->diffidx[i];
    lbi = kb->diffidxe[i]; /* old bitindices */

    /* If this is an empty message , don't count or correct */
    if (fbi > lbi) {  
      lost_bits -= 2;  /* No initial parity, no outgoing */
      goto skipparity; /* no more parity evaluation, skip rest */
    }
    /* If we have found the bit error */
    if (fbi == lbi) {
      if (biconfmark) correct_bit(d2, fbi);
      correct_bit(d, fbi);
      kb->correctedErrors++;
      lost_bits -= 2;           /* No initial parity, no outgoing */
      kb->diffidx[i] = fbi + 1; /* mark as emty */
      goto skipparity;          /* no more parity evaluation, skip rest */
    }

    mbi = fbi + (lbi - fbi + 1) / 2 - 1; /* new lower mid bitidx */
    tmpSingleLineParity = singleLineParity(d, fbi, mbi);
    if (((inh_data[i / 32] & bt_mask(i)) ? 1 : 0) == tmpSingleLineParity) {
      /* same parity, take upper half */
      fbi = mbi + 1;
      kb->diffidx[i] = fbi; /* update first bit idx */
      matchresult |= 1;     /* indicate match with incoming parity */
    } else {
      lbi = mbi;
      kb->diffidxe[i] = lbi; /* update last bit idx */
    }
    /* If this is end of interval, correct for error */
    if (fbi == lbi) {
      if (biconfmark) correct_bit(d2, fbi);
      correct_bit(d, fbi);
      kb->correctedErrors++;
      lost_bits--; /* we don't reveal anything on this one anymore */
      goto skipparity;
    }
    /* Else, prepare new parity bit */
    mbi = fbi + (lbi - fbi + 1) / 2 - 1; /* new lower mid bitidx */
    parityresult |= singleLineParity(d, fbi, mbi); /* save parity */

  skipparity:
    if ((i & 31) == 31) {
      /* save stuff in outbuffers */
      out_match[i / 32] = matchresult;
      out_parity[i / 32] = parityresult;
    }
  }
  /* Cleanup residual bit buffers */
  if (i & 31) {
    out_match[i / 32] = matchresult << (32 - (i & 31));
    out_parity[i / 32] = parityresult << (32 - (i & 31));
  }

  /* a blocklength k decides on a max number of rounds */
  if ((kb->binarySearchDepth & RUNLEVEL_ROUNDMASK) < log2Ceil((kb->processingState == PRS_DOING_BICONF)
      ? (kb->biconflength) : (thispass ? kb->k1 : kb->k0))) {
    /* need to continue with this search; make packet 5 ready to send */
    kb->leakageBits += lost_bits;
    comms_insertSendPacket((char *)out_head, out_head->base.totalLengthInBytes);
    return 0;
  }

  kb->leakageBits += lost_bits; /* correction for unreceived parity bits and
                                   nonsent parities */

  /* cleanup changed bits in the other permuted field */
  fix_permutedbits(kb);

  /* prepare for alternate round; start with re-evaluation of parity. */
  while (True) { /* just a break construction.... */
    kb->binarySearchDepth = thispass ? RUNLEVEL_FIRSTPASS : RUNLEVEL_SECONDPASS;
    kb->diffBlockCount =
        do_paritylist_and_diffs(kb, 1 - thispass);       /* new differences */
    if (kb->diffBlockCount == -1) return 74;                 /* wrong pass */
    if ((kb->diffBlockCount == 0) && (thispass == 1)) break; /* no more errors */
    if (kb->diffBlockCount > kb->diffBlockCountMax) {           /* need more space */
      free2(kb->diffidx); /* re-assign diff buf */
      kb->diffBlockCountMax = kb->diffBlockCount;
      kb->diffidx = (unsigned int *)malloc2(kb->diffBlockCount * sizeof(unsigned int) * 2);
      if (!kb->diffidx) return 54;                 /* can't malloc */
      kb->diffidxe = &kb->diffidx[kb->diffBlockCount]; /* end of interval */
    }

    /* do basically a start_binarysearch for next round */
    return prepare_first_binsearch_msg(kb, 1 - thispass);
  }

  /* now we have finished a consecutive the second round; there are no more
     errors in both passes.  */

  /* check for biconf reply  */
  if (kb->processingState == PRS_DOING_BICONF) { 
    /* we are finally finished with the  BICONF corrections */
    /* update biconf status */
    kb->biconfRound++;

    /* eventully generate new biconf request */
    if (kb->biconfRound < arguments.biconfRounds) {
      return initiate_biconf(kb); /* request another one */
    }
    /* initiate the privacy amplificaton */
    return initiate_privacyamplification(kb);
  }

  /* we have no more errors in both passes, and we were not yet
     in BICONF mode */

  /* initiate the BICONF state */
  kb->processingState = PRS_DOING_BICONF;
  kb->biconfRound = 0; /* first BICONF round */
  return initiate_biconf(kb);
}

/** @brief Start the parity generation process on bob's side. 
 * 
   Should either initiate a binary search, re-issue a BICONF request or
   continue to the parity evaluation. 

  * @param receivebuf parity reply from Alice
  * @return error message, 0 if success
  */
int receive_biconfreply(ProcessBlock *kb, char *receivebuf) {
  EcPktHdr_CascadeBiconfParityResp *in_head  = (EcPktHdr_CascadeBiconfParityResp *)receivebuf; /* holds received message header */
  int localparity;

  kb->binarySearchDepth = RUNLEVEL_SECONDPASS; /* use permuted buf */

  /* update incoming bit leakage */
  kb->leakageBits++;

  /* evaluate local parity */
  localparity = singleLineParity(kb->testedBitsMarker, 0, kb->biconflength - 1);

  /* eventually start binary search */
  if (localparity != in_head->parity) {
    return initiate_biconf_binarysearch(kb, kb->biconflength);
  }
  /* this location gets ONLY visited if there is no error in BICONF search */

  /* update biconf status */
  kb->biconfRound++;

  /* eventully generate new biconf request */
  if (kb->biconfRound < arguments.biconfRounds) {
    return initiate_biconf(kb); /* request another one */
  }
  /* initiate the privacy amplificaton */
  return initiate_privacyamplification(kb);
}

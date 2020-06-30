#include "helpers.h"

// GENERAL HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */
char hexdigits[] = "0123456789abcdef";
/**
 * @brief helper for name. adds a slash, hex file name and a terminal 0
 * 
 * @param target 
 * @param v 
 */
void atohex(char *target, unsigned int v) {
  int i;
  target[0] = '/';
  for (i = 1; i < 9; i++) target[i] = hexdigits[(v >> (32 - i * WORD_SIZE)) & 15];
  target[9] = 0;
}

// HELPER PERMUTATION FUNCTIONS BETWEEN CASCADE & QBER ESTIMATION
/* ------------------------------------------------------------------------- */

/**
 * @brief Helper function to compress key down in a sinlge sequence to eliminate the
   revealeld bits. 

   updates workbits accordingly, and reduces number of
   revealed bits in the leakage_bits_counter
 * 
 * @param pb 
 */
void cleanup_revealed_bits(ProcessBlock *pb) {
  int lastbit = pb->initialBits - 1;
  unsigned int *d = pb->mainBufPtr;    /* data buffer */
  unsigned int *m = pb->testedBitsMarker; /* index for spent bits */
  unsigned int bm;                  /* temp storage of bitmask */
  int i;

  /* find first nonused lastbit */
  while ((lastbit > 0) && (m[wordIndex(lastbit)] & bt_mask(lastbit))) 
    lastbit--;

  /* replace spent bits in beginning by untouched bits at end */
  for (i = 0; i <= lastbit; i++) {
    bm = bt_mask(i);
    if (m[wordIndex(i)] & bm) { /* this bit is revealed */
      d[wordIndex(i)] =
          (d[wordIndex(i)] & ~bm) |
          ((d[wordIndex(lastbit)] & bt_mask(lastbit)) ? bm : 0); /* transfer bit */
      /* get new lastbit */
      lastbit--;
      while ((lastbit > 0) && (m[wordIndex(lastbit)] & bt_mask(lastbit))) 
        lastbit--;
    }
  }
  /* i should now contain the number of good bits */
  pb->workbits = i;

  /* fill rest of buffer with zeros for not loosing any bits */
  d[wordIndex(i)] &= ((modulo32(i)) ? (0xffffffff << (32 - modulo32(i))) : 0);
  for (i = wordIndex(pb->workbits) + 1; i < wordCount(pb->initialBits); i++) {
    d[i] = 0;
    /* printf("   i= %d\n",i); */
  }

  /* update number of lost bits */
  pb->leakageBits = 0;

  return;
}

/**
 * @brief elper function to do generate the permutation array in the kb structure.
 * 
 * oes also re-ordering (in future), and truncates the discussed key to a
   length of multiples of k1so there are noleftover bits in the two passes.
 * 
 * @param pb pointer to processblock
 */
void prepare_permutation(ProcessBlock *pb) {
  /* do bit compression */
  cleanup_revealed_bits(pb);
  int workbits = pb->workbits;

  /* a quick-and-dirty cut for kb1 match, will change to reordering later.
     also: take more care about the leakage_bits here */

  /* assume last k1 block is filled with good bits and zeros */
  workbits = ((workbits / pb->k1) + 1) * pb->k1;
  /* forget the last bits if it is larger than the buffer */
  if (workbits > pb->initialBits) workbits -= pb->k1;

  pb->workbits = workbits;

  /* do first permutation - this is only the initial permutation */
  prepare_permut_core(pb);
  /* now the permutated buffer is renamed and the final permutation is performed */
  // Swap the buffer (addresses)
  unsigned int *tmpbuf = pb->mainBufPtr;
  pb->mainBufPtr = pb->permuteBufPtr;
  pb->permuteBufPtr = tmpbuf;
  
  /* do final permutation */
  prepare_permut_core(pb);
  return;
}

/**
 * @brief permutation core function; is used both for biconf and initial permutation
 * 
 * @param pb ptr to processblock
 */
void prepare_permut_core(ProcessBlock *pb) {
  int i, j, k;
  int workbits = pb->workbits;
  unsigned int rn_order = log2Ceil(workbits);

  #ifdef SYSTPERMUTATION

  /* this prepares a systematic permutation  - seems not to be better, but
     blocknumber must be coprime with 127 - larger primes? */
  for (i = 0; i < workbits; i++) {
    k = (127 * i * pb->k0 + i * pb->k0 / workbits) % workbits;
    pb->permuteIndex[k] = i;
    pb->reverseIndex[i] = k;
  }
  #else
  /* this is prepares a pseudorandom distribution */
  for (i = 0; i < workbits; i++) pb->permuteIndex[i] = 0xffff; /* mark unused */
  /* this routine causes trouble */
  for (i = 0; i < workbits; i++) { /* do permutation  */
    do {                           /* find a permutation index */
      j = PRNG_value2(rn_order, &pb->rngState);
    } while ((j >= workbits) || (pb->permuteIndex[j] != 0xffff)); /* out of range */
    k = j;
    pb->permuteIndex[k] = i;
    pb->reverseIndex[i] = k;
  }

  #endif

  bzero(pb->permuteBufPtr, wordCount(workbits) * WORD_SIZE); /* clear permuted buffer */
  for (i = 0; i < workbits; i++) {                   /*  do bit permutation  */
    k = pb->permuteIndex[i];
    if (bt_mask(i) & pb->mainBufPtr[wordIndex(i)]) pb->permuteBufPtr[wordIndex(k)] |= bt_mask(k);
  }

  /* for debug: output that stuff */
  /* output_permutation(pb); */
  return;
}

/**
 * @brief helper function to preare a parity list of a given pass in a block.
 * 
 * @param d pointer to source buffer
 * @param t pointer to target bffer
 * @param k integer arg for the blocksize to use
 * @param w number of workbits
 */
void prepare_paritylist_basic(unsigned int *d, unsigned int *t, int k, int w) {
  /* the bitindex points to the first and the last bit tested. */
  unsigned int resbuf = 0; /* result buffer */
  int blkidx = 0;          /* contains blockindex */  
  for (int bitidx = 0; bitidx < w; bitidx += k) {
    /* shift parity result in buffer */
    resbuf = (resbuf << 1) + singleLineParity(d, bitidx, bitidx + k - 1);
    if (modulo32(blkidx) == 31) 
      t[wordIndex(blkidx)] = resbuf; /* save in target */
    blkidx++;
  }
  /* cleanup residual parity buffer */
  if (modulo32(blkidx) != 0) 
    t[wordIndex(blkidx)] = resbuf << (32 - modulo32(blkidx));
  return;
}

/** @brief Helper funtion to get a simple one-line parity from a large string.
 * 
 * @param d pointer to the start of the string buffer
 * @param start start index
 * @param end end index
 * @return parity (0 or 1)
 */
int singleLineParity(unsigned int *d, int start, int end) {
  int fi = wordIndex(start);
  int li = wordIndex(end);
  unsigned int fm = firstmask(modulo32(start));
  unsigned int lm = lastmask(modulo32(end));
  unsigned int tmp_par;
  if (li == fi) {
    tmp_par = d[fi] & lm & fm;
  } else {
    tmp_par = (d[fi] & fm) ^ (d[li] & lm);
    for (int ri = fi + 1; ri < li; ri++) tmp_par ^= d[ri];
  } /* tmp_par holds now a combination of bits to be tested */
  return parity(tmp_par);
}

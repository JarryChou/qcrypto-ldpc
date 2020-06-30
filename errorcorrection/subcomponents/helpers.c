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
  for (i = 1; i < 9; i++) target[i] = hexdigits[(v >> (32 - i * 4)) & 15];
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
 * @param kb 
 */
void cleanup_revealed_bits(ProcessBlock *kb) {
  int lastbit = kb->initialBits - 1;
  unsigned int *d = kb->mainBufPtr;    /* data buffer */
  unsigned int *m = kb->testedBitsMarker; /* index for spent bits */
  unsigned int bm;                  /* temp storage of bitmask */
  int i;

  /* find first nonused lastbit */
  while ((lastbit > 0) && (m[lastbit / 32] & bt_mask(lastbit))) 
    lastbit--;

  /* replace spent bits in beginning by untouched bits at end */
  for (i = 0; i <= lastbit; i++) {
    bm = bt_mask(i);
    if (m[i / 32] & bm) { /* this bit is revealed */
      d[i / 32] =
          (d[i / 32] & ~bm) |
          ((d[lastbit / 32] & bt_mask(lastbit)) ? bm : 0); /* transfer bit */
      /* get new lastbit */
      lastbit--;
      while ((lastbit > 0) && (m[lastbit / 32] & bt_mask(lastbit))) 
        lastbit--;
    }
  }
  /* i should now contain the number of good bits */
  kb->workbits = i;

  /* fill rest of buffer with zeros for not loosing any bits */
  d[i / 32] &= ((i & 31) ? (0xffffffff << (32 - (i & 31))) : 0);
  for (i = ((kb->workbits / 32) + 1); i < (kb->initialBits + 31) / 32; i++) {
    d[i] = 0;
    /* printf("   i= %d\n",i); */
  }

  /* update number of lost bits */
  kb->leakageBits = 0;

  return;
}

/**
 * @brief elper function to do generate the permutation array in the kb structure.
 * 
 * oes also re-ordering (in future), and truncates the discussed key to a
   length of multiples of k1so there are noleftover bits in the two passes.
 * 
 * @param kb pointer to processblock
 */
void prepare_permutation(ProcessBlock *kb) {
  /* do bit compression */
  cleanup_revealed_bits(kb);
  int workbits = kb->workbits;

  /* a quick-and-dirty cut for kb1 match, will change to reordering later.
     also: take more care about the leakage_bits here */

  /* assume last k1 block is filled with good bits and zeros */
  workbits = ((workbits / kb->k1) + 1) * kb->k1;
  /* forget the last bits if it is larger than the buffer */
  if (workbits > kb->initialBits) workbits -= kb->k1;

  kb->workbits = workbits;

  /* do first permutation - this is only the initial permutation */
  prepare_permut_core(kb);
  /* now the permutated buffer is renamed and the final permutation is performed */
  // Swap the buffer (addresses)
  unsigned int *tmpbuf = kb->mainBufPtr;
  kb->mainBufPtr = kb->permuteBufPtr;
  kb->permuteBufPtr = tmpbuf;
  
  /* do final permutation */
  prepare_permut_core(kb);
  return;
}

/**
 * @brief permutation core function; is used both for biconf and initial permutation
 * 
 * @param kb ptr to processblock
 */
void prepare_permut_core(ProcessBlock *kb) {
  int i, j, k;
  int workbits = kb->workbits;
  unsigned int rn_order = log2Ceil(workbits);

  #ifdef SYSTPERMUTATION

  /* this prepares a systematic permutation  - seems not to be better, but
     blocknumber must be coprime with 127 - larger primes? */
  for (i = 0; i < workbits; i++) {
    k = (127 * i * kb->k0 + i * kb->k0 / workbits) % workbits;
    kb->permuteIndex[k] = i;
    kb->reverseIndex[i] = k;
  }
  #else
  /* this is prepares a pseudorandom distribution */
  for (i = 0; i < workbits; i++) kb->permuteIndex[i] = 0xffff; /* mark unused */
  /* this routine causes trouble */
  for (i = 0; i < workbits; i++) { /* do permutation  */
    do {                           /* find a permutation index */
      j = PRNG_value2(rn_order, &kb->rngState);
    } while ((j >= workbits) || (kb->permuteIndex[j] != 0xffff)); /* out of range */
    k = j;
    kb->permuteIndex[k] = i;
    kb->reverseIndex[i] = k;
  }

  #endif

  bzero(kb->permuteBufPtr, ((workbits + 31) / 32) * 4); /* clear permuted buffer */
  for (i = 0; i < workbits; i++) {                   /*  do bit permutation  */
    k = kb->permuteIndex[i];
    if (bt_mask(i) & kb->mainBufPtr[i / 32]) kb->permuteBufPtr[k / 32] |= bt_mask(k);
  }

  /* for debug: output that stuff */
  /* output_permutation(kb); */
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
    if ((blkidx & 31) == 31) 
      t[blkidx / 32] = resbuf; /* save in target */
    blkidx++;
  }
  /* cleanup residual parity buffer */
  if (blkidx & 31) 
    t[blkidx / 32] = resbuf << (32 - (blkidx & 31));
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
  int fi = start / 32;
  int li = end / 32;
  unsigned int fm = firstmask(start & 31);
  unsigned int lm = lastmask(end & 31);
  unsigned int tmp_par;
  if (li == fi) {
    tmp_par = d[fi] & lm & fm;
  } else {
    tmp_par = (d[fi] & fm) ^ (d[li] & lm);
    for (int ri = fi + 1; ri < li; ri++) tmp_par ^= d[ri];
  } /* tmp_par holds now a combination of bits to be tested */
  return parity(tmp_par);
}

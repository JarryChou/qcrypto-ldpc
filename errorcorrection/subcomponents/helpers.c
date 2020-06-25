#include "helpers.h"

// GENERAL HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */

/**
 * @brief helper to obtain the smallest power of two to carry a number a
 * 
 * @param a 
 * @return int 
 */
int get_order(int a) {
  unsigned int order = 0xffffffff;
  while ((order & a) == a) order >>= 1;
  return (order << 1) + 1;
}

/**
 * @brief get the number of bits necessary to carry a number x
 * 
 * result is e.g. 3 for parameter 8, 5 for parameter 17 etc. 
 * 
 * @param x 
 * @return int 
 */
int get_order_2(int x) {
  int x2;
  int retval = 0;
  for (x2 = x - 1; x2; x2 >>= 1) retval++;
  return retval;
}

/**
 * @brief count the number of set bits in a longint
 * 
 * @param a 
 * @return int 
 */
int count_set_bits(unsigned int a) {
  int c = 0;
  unsigned int i;
  for (i = 1; i; i <<= 1)
    if (i & a) c++;
  return c;
}

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
void cleanup_revealed_bits(struct keyblock *kb) {
  int lastbit = kb->initialbits - 1;
  unsigned int *d = kb->mainbuf;    /* data buffer */
  unsigned int *m = kb->testmarker; /* index for spent bits */
  unsigned int bm;                  /* temp storage of bitmask */
  int i;

  /* find first nonused lastbit */
  while ((lastbit > 0) && (m[lastbit / 32] & bt_mask(lastbit))) lastbit--;

  /* replace spent bits in beginning by untouched bits at end */
  for (i = 0; i <= lastbit; i++) {
    bm = bt_mask(i);
    if (m[i / 32] & bm) { /* this bit is revealed */
      d[i / 32] =
          (d[i / 32] & ~bm) |
          ((d[lastbit / 32] & bt_mask(lastbit)) ? bm : 0); /* transfer bit */
      /* get new lastbit */
      lastbit--;
      while ((lastbit > 0) && (m[lastbit / 32] & bt_mask(lastbit))) lastbit--;
    }
  }
  /* i should now contain the number of good bits */
  kb->workbits = i;

  /* fill rest of buffer with zeros for not loosing any bits */
  d[i / 32] &= ((i & 31) ? (0xffffffff << (32 - (i & 31))) : 0);
  for (i = ((kb->workbits / 32) + 1); i < (kb->initialbits + 31) / 32; i++) {
    d[i] = 0;
    /* printf("   i= %d\n",i); */
  }

  /* update number of lost bits */
  kb->leakagebits = 0;

  return;
}

/**
 * @brief elper function to do generate the permutation array in the kb structure.
 * 
 * oes also re-ordering (in future), and truncates the discussed key to a
   length of multiples of k1so there are noleftover bits in the two passes.
 * 
 * @param kb pointer to keyblock
 */
void prepare_permutation(struct keyblock *kb) {
  int workbits;
  unsigned int *tmpbuf;

  /* do bit compression */
  cleanup_revealed_bits(kb);
  workbits = kb->workbits;

  /* a quick-and-dirty cut for kb1 match, will change to reordering later.
     also: take more care about the leakage_bits here */

  /* assume last k1 block is filled with good bits and zeros */
  workbits = ((workbits / kb->k1) + 1) * kb->k1;
  /* forget the last bits if it is larger than the buffer */
  if (workbits > kb->initialbits) workbits -= kb->k1;

  kb->workbits = workbits;

  /* do first permutation - this is only the initial permutation */
  prepare_permut_core(kb);
  /* now the permutated buffer is renamed and the final permutation is
     performed */
  tmpbuf = kb->mainbuf;
  kb->mainbuf = kb->permutebuf;
  kb->permutebuf = tmpbuf;
  /* fo final permutation */
  prepare_permut_core(kb);
  return;
}

/**
 * @brief permutation core function; is used both for biconf and initial permutation
 * 
 * @param kb ptr to keyblock
 */
void prepare_permut_core(struct keyblock *kb) {
  int workbits;
  unsigned int rn_order;
  int i, j, k;
  workbits = kb->workbits;
  rn_order = get_order_2(workbits);

  #ifdef SYSTPERMUTATION

  /* this prepares a systematic permutation  - seems not to be better, but
     blocknumber must be coprime with 127 - larger primes? */
  for (i = 0; i < workbits; i++) {
    k = (127 * i * kb->k0 + i * kb->k0 / workbits) % workbits;
    kb->permuteindex[k] = i;
    kb->reverseindex[i] = k;
  }
  #else
  /* this is prepares a pseudorandom distribution */
  for (i = 0; i < workbits; i++) kb->permuteindex[i] = 0xffff; /* mark unused */
  /* this routine causes trouble */
  for (i = 0; i < workbits; i++) { /* do permutation  */
    do {                           /* find a permutation index */
      j = PRNG_value2(rn_order, &kb->RNG_state);
    } while ((j >= workbits) ||
             (kb->permuteindex[j] != 0xffff)); /* out of range */
    k = j;
    kb->permuteindex[k] = i;
    kb->reverseindex[i] = k;
  }

  #endif

  bzero(kb->permutebuf, ((workbits + 31) / 32) * 4); /* clear permuted buffer */
  for (i = 0; i < workbits; i++) {                   /*  do bit permutation  */
    k = kb->permuteindex[i];
    if (bt_mask(i) & kb->mainbuf[i / 32]) kb->permutebuf[k / 32] |= bt_mask(k);
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
  int blkidx;           /* contains blockindex */
  int bitidx;           /* startbit index */
  unsigned int tmp_par; /* for combining parities */
  unsigned int resbuf;  /* result buffer */
  int fi, li, ri;       /* first and last and running bufferindex */
  unsigned int fm, lm;  /* first mask, last mask */

  /* the bitindex points to the first and the last bit tested. */
  resbuf = 0;
  tmp_par = 0;
  blkidx = 0;
  for (bitidx = 0; bitidx < w; bitidx += k) {
    fi = bitidx / 32;
    fm = firstmask(bitidx & 31); /* beginning */
    li = (bitidx + k - 1) / 32;
    lm = lastmask((bitidx + k - 1) & 31); /* end */
    if (li == fi) {                       /* in same word */
      tmp_par = d[fi] & lm & fm;
    } else {
      tmp_par = (d[fi] & fm) ^ (d[li] & lm);
      for (ri = fi + 1; ri < li; ri++) tmp_par ^= d[ri];
    } /* tmp_par holds now a combination of bits to be tested */
    resbuf =
        (resbuf << 1) + parity(tmp_par); /* shift parity result in buffer */
    if ((blkidx & 31) == 31) t[blkidx / 32] = resbuf; /* save in target */
    blkidx++;
  }
  /* cleanup residual parity buffer */
  if (blkidx & 31) t[blkidx / 32] = resbuf << (32 - (blkidx & 31));
  return;
}
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
  while ((lastbit > 0) && (m[wordIndex(lastbit)] & uint32AllZeroExceptAtN(lastbit))) 
    lastbit--;

  /* replace spent bits in beginning by untouched bits at end */
  for (i = 0; i <= lastbit; i++) {
    bm = uint32AllZeroExceptAtN(i);
    if (m[wordIndex(i)] & bm) { /* this bit is revealed */
      d[wordIndex(i)] =
          (d[wordIndex(i)] & ~bm) |
          ((d[wordIndex(lastbit)] & uint32AllZeroExceptAtN(lastbit)) ? bm : 0); /* transfer bit */
      /* get new lastbit */
      lastbit--;
      while ((lastbit > 0) && (m[wordIndex(lastbit)] & uint32AllZeroExceptAtN(lastbit))) 
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
    if (uint32AllZeroExceptAtN(i) & pb->mainBufPtr[wordIndex(i)]) pb->permuteBufPtr[wordIndex(k)] |= uint32AllZeroExceptAtN(k);
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
void prepare_paritylist_basic(unsigned int *srcBuffer, unsigned int *targetBuffer, int k, int workBitCount) {
  /* the bitindex points to the first and the last bit tested. */
  unsigned int resultBuffer = 0;  /* result buffer */
  int blkIndex = 0;               /* contains blockindex */  
  for (int bitIndex = 0; bitIndex < workBitCount; bitIndex += k) {
    /* shift parity result in buffer */
    resultBuffer <<= 1;
    // Add a new parity result
    resultBuffer += singleLineParity(srcBuffer, bitIndex, bitIndex + k - 1);
    if (modulo32(blkIndex) == 31) 
      targetBuffer[wordIndex(blkIndex)] = resultBuffer; /* save in target */
    blkIndex++;
  }
  /* cleanup residual parity buffer by right padding with zeroes */
  if (modulo32(blkIndex) != 0) 
    targetBuffer[wordIndex(blkIndex)] = resultBuffer << (32 - modulo32(blkIndex));
  return;
}

/** @brief Helper funtion to get a simple one-line parity from a large string.
 * 
 * @param bitBuffer pointer to the start of the string buffer
 * @param startBitIndex start index
 * @param endBitIndex end index
 * @return parity (0 or 1)
 */
int singleLineParity(unsigned int *bitBuffer, int startBitIndex, int endBitIndex) {
  int firstWordIndex = wordIndex(startBitIndex);
  int lastWordIndex = wordIndex(endBitIndex);
  // First / last words may not be fully padded. 
  // Masks ensure we only XOR the bits that should be checked.
  unsigned int firstWordMask = uint32AllOnesExceptFirstN(modulo32(startBitIndex));
  unsigned int lastWordMask = uint32AllOnesExceptLastN(modulo32(endBitIndex));
  // Use XOR to combine the entire buffer of bits into an unsigned int to be checked for parity 
  unsigned int tmpXorValues;
  if (lastWordIndex == firstWordIndex) {
    tmpXorValues = bitBuffer[firstWordIndex] & lastWordMask & firstWordMask;
  } else {
    tmpXorValues = (bitBuffer[firstWordIndex] & firstWordMask) ^ (bitBuffer[lastWordIndex] & lastWordMask);
    for (int wordIndex = firstWordIndex + 1; wordIndex < lastWordIndex; wordIndex++) 
      tmpXorValues ^= bitBuffer[wordIndex];
  } /* tmpXorValues holds now a combination of bits to be tested */
  return parity(tmpXorValues);
}

/** @brief Helper funtion to get a simple one-line parity from a large string, but
   this time with a mask buffer to be AND-ed on the string.

   This is currently unused, if it is needed, extend singleLineParity to support this

 * @param d pointer to the start of the string buffer
 * @param m pointer to the start of the mask buffer
 * @param start start index
 * @param end end index
 * @return parity (0 or 1)
 */
/*
int singleLineParityMasked(unsigned int *d, unsigned int *m, int start, int end) {
  unsigned int tmpParity, lm, fm;
  int li, fi, ri;
  fi = wordIndex(start);
  li = wordIndex(end);
  lm = uint32AllOnesExceptLastN(modulo32(end));
  fm = uint32AllOnesExceptFirstN(modulo32(start));
  if (li == fi) {
    tmpParity = d[fi] & lm & fm & m[fi];
  } else {
    tmpParity = (d[fi] & fm & m[fi]) ^ (d[li] & lm & m[li]);
    for (ri = fi + 1; ri < li; ri++) tmpParity ^= (d[ri] & m[ri]);
  } // tmpParity holds now a combination of bits to be tested 
  return parity(tmpParity);
}
*/

/** 
 * @brief Helper for correcting one bit in pass 0 or 1 in their field 
 * 
 * This can be turned into a 
*/
/*
void flipBit(unsigned int *d, int bitindex) {
  d[wordIndex(bitindex)] ^= uint32AllZeroExceptAtN(bitindex); // flip bit
  return;
}
*/
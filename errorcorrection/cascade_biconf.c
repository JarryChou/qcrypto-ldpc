#include "cascade_biconf.h"

// PERMUTATIONS
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS
/* helper to fix the permuted/unpermuted bit changes; decides via a parameter
   in kb->binsearch_depth MSB what polarity to take */
void fix_permutedbits(struct keyblock *kb) {
  int i, k;
  unsigned int *src, *dst;
  unsigned short *idx; /* pointers to data loc and permute idx */
  if (kb->binsearch_depth & RUNLEVEL_LEVELMASK) { /* we are in pass 1 */
    src = kb->permutebuf;
    dst = kb->mainbuf;
    idx = kb->reverseindex;
  } else { /* we are in pass 0 */
    src = kb->mainbuf;
    dst = kb->permutebuf;
    idx = kb->permuteindex;
  }
  bzero(dst, ((kb->workbits + 31) / 32) * 4); /* clear dest */
  for (i = 0; i < kb->workbits; i++) {
    k = idx[i]; /* permuted bit index */
    if (bt_mask(i) & src[i / 32]) dst[k / 32] |= bt_mask(k);
  }
  return;
}

/* helper function to do generate the permutation array in the kb structure.
   does also re-ordering (in future), and truncates the discussed key to a
   length of multiples of k1so there are noleftover bits in the two passes.
   Parameter: pointer to kb structure */
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
// MAIN FUNCTIONS
/* permutation core function; is used both for biconf and initial permutation */
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

// CASCADE BICONF
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS
/* helper function to preare a parity list of a given pass in a block.
   Parameters are a pointer to the sourcebuffer, pointer to the target buffer,
   and an integer arg for the blocksize to use, and the number of workbits */
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

/* helper function to generate a pseudorandom bit pattern into the test bit
   buffer. parameters are a keyblock pointer, and a seed for the RNG.
   the rest is extracted out of the kb structure (for final parity test) */
void generate_selectbitstring(struct keyblock *kb, unsigned int seed) {
  int i;                                    /* number of bits to be set */
  kb->RNG_state = seed;                     /* set new seed */
  for (i = 0; i < (kb->workbits) / 32; i++) /* take care of the full bits */
    kb->testmarker[i] = PRNG_value2_32(&kb->RNG_state);
  kb->testmarker[kb->workbits / 32] = /* prepare last few bits */
      PRNG_value2_32(&kb->RNG_state) & lastmask((kb->workbits - 1) & 31);
  return;
}

/* helper function to generate a pseudorandom bit pattern into the test bit
   buffer AND transfer the permuted key buffer into it for a more compact
   parity generation in the last round.
   Parameters are a keyblock pointer.
   the rest is extracted out of the kb structure (for final parity test) */
void generate_BICONF_bitstring(struct keyblock *kb) {
  int i;                                      /* number of bits to be set */
  for (i = 0; i < (kb->workbits) / 32; i++) { /* take care of the full bits */
    kb->testmarker[i] = PRNG_value2_32(&kb->RNG_state) &
                        kb->permutebuf[i]; /* get permuted bit */
  }
  kb->testmarker[kb->workbits / 32] = /* prepare last few bits */
      PRNG_value2_32(&kb->RNG_state) & lastmask((kb->workbits - 1) & 31) &
      kb->permutebuf[kb->workbits / 32];
  return;
}

/* helper function to preare a parity list of a given pass in a block, compare
   it with the received list and return the number of differing bits  */
int do_paritylist_and_diffs(struct keyblock *kb, int pass) {
  int numberofbits = 0;
  int i, partitions;          /* counting index, num of blocks */
  unsigned int *lp, *rp, *pd; /* local/received & diff parity pointer */
  unsigned int *d;            /* for paritylist */
  int k;
  switch (pass) {
    case 0:
      k = kb->k0;
      d = kb->mainbuf;
      lp = kb->lp0;
      rp = kb->rp0;
      pd = kb->pd0;
      partitions = kb->partitions0;
      break;
    case 1:
      k = kb->k1;
      d = kb->permutebuf;
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
    numberofbits += count_set_bits(pd[i]);
  }
  return numberofbits;
}

/* helper function to prepare parity lists from original and unpermutated key.
   arguments are a pointer to the thread structure, a pointer to the target
   parity buffer 0 and another pointer to paritybuffer 1. No return value,
   as no errors are tested here. */
void prepare_paritylist1(struct keyblock *kb, unsigned int *d0, unsigned int *d1) {
  prepare_paritylist_basic(kb->mainbuf, d0, kb->k0, kb->workbits);
  prepare_paritylist_basic(kb->permutebuf, d1, kb->k1, kb->workbits);
  return;
}

/* helper program to half parity difference intervals ; takes kb and inh_index
   as parameters; no weired stuff should happen. return value is the number
   of initially dead intervals */
void fix_parity_intervals(struct keyblock *kb, unsigned int *inh_idx) {
  int i, fbi, lbi;                       /* running index */
  for (i = 0; i < kb->diffnumber; i++) { /* go through all different blocks */
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

/* helper for correcting one bit in pass 0 or 1 in their field */
void correct_bit(unsigned int *d, int bitindex) {
  d[bitindex / 32] ^= bt_mask(bitindex); /* flip bit */
  return;
}

/* helper funtion to get a simple one-line parity from a large string.
   parameters are the string start buffer, a start and an enx index. returns
   0 or 1 */
int single_line_parity(unsigned int *d, int start, int end) {
  unsigned int tmp_par, lm, fm;
  int li, fi, ri;
  fi = start / 32;
  li = end / 32;
  lm = lastmask(end & 31);
  fm = firstmask(start & 31);
  if (li == fi) {
    tmp_par = d[fi] & lm & fm;
  } else {
    tmp_par = (d[fi] & fm) ^ (d[li] & lm);
    for (ri = fi + 1; ri < li; ri++) tmp_par ^= d[ri];
  } /* tmp_par holds now a combination of bits to be tested */
  return parity(tmp_par);
}

/* helper funtion to get a simple one-line parity from a large string, but
   this time with a mask buffer to be AND-ed on the string.
   parameters are the string buffer, mask buffer, a start and and end index.
   returns  0 or 1 */
int single_line_parity_masked(unsigned int *d, unsigned int *m, int start,
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

// MAIN FUNCTIONS
/* function to process a binarysearch request on alice identity. Installs the
   difference index list in the first run, and performs the parity checks in
   subsequent runs. should work with both passes now
   - work in progress, need do fix bitloss in last round
 */
int process_binsearch_alice(struct keyblock *kb, struct ERRC_ERRDET_5 *in_head) {
  unsigned int *inh_data, *inh_idx;
  int i;
  struct ERRC_ERRDET_5 *out_head; /* for reply message */
  unsigned int *out_parity;       /* pointer to outgoing parity result info */
  unsigned int *out_match;        /* pointer to outgoing matching info */
  unsigned int *d;                /* points to internal key data */
  int k;                          /* keeps blocklength */
  unsigned int matchresult = 0, parityresult = 0; /* for builduing outmsg */
  unsigned int fm, lm, tmp_par;                   /* for parity evaluation */
  int fbi, lbi, mbi, fi, li, ri;                  /* for parity evaluation */
  int lost_bits; /* number of key bits revealed in this round */

  inh_data = (unsigned int *)&in_head[1]; /* parity pattern */

  /* find out if difference index should be installed */
  while (in_head->index_present) {
    if (kb->diffidx) { /* there is already a diffindex */
      if (kb->diffnumber_max >= in_head->number_entries) {
        /* just re-assign */
        kb->diffnumber = in_head->number_entries;
        break;
      }
      /* otherwise: not enough space; remove the old one... */
      free2(kb->diffidx);
      /* ....and continue re-assigning... */
    }
    /* allocate difference idx memory */
    kb->diffnumber = in_head->number_entries; /* from far cons check? */
    kb->diffnumber_max = kb->diffnumber;
    kb->diffidx =
        (unsigned int *)malloc2(kb->diffnumber * sizeof(unsigned int) * 2);
    if (!kb->diffidx) return 54;                 /* can't malloc */
    kb->diffidxe = &kb->diffidx[kb->diffnumber]; /* end of interval */
    break;
  }

  inh_idx = &inh_data[(kb->diffnumber + 31) / 32]; /* index or matching part */

  /* sort out pass-dependent variables */
  if (in_head->runlevel & RUNLEVEL_LEVELMASK) { /* this is pass 1 */
    d = kb->permutebuf;
    k = kb->k1;
  } else { /* this is pass 0 */
    d = kb->mainbuf;
    k = kb->k0;
  }

  /* special case to take care of if this is a BICONF localizing round:
     the variables d and k contain worng values at this point.
     this is taken care now */
  if (in_head->runlevel & RUNLEVEL_BICONF) {
    d = kb->testmarker;
    k = kb->biconflength;
  }

  /* fix index list according to parity info or initial run */
  switch (in_head->index_present) { /* different encodings */
    case 0: /* repair index according to previous basis match */
      fix_parity_intervals(kb, inh_idx);
      break;
    case 1: /* simple unsigned int encoding */
      for (i = 0; i < kb->diffnumber; i++) {
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
  kb->leakagebits += kb->diffnumber; /* for incoming parity bits */
  /* check if this masking is correct? let biconf status survive  */
  kb->binsearch_depth =
      ((in_head->runlevel + 1) & RUNLEVEL_ROUNDMASK) +
      (in_head->runlevel & (RUNLEVEL_LEVELMASK | RUNLEVEL_BICONF));

  /* prepare outgoing message header */
  out_head = make_messagehead_5(kb);
  if (!out_head) return 58;
  out_parity = (unsigned int *)&out_head[1];
  out_match = &out_parity[(kb->diffnumber + 31) / 32];

  lost_bits = kb->diffnumber; /* to keep track of lost bits */

  /* go through all entries */
  for (i = 0; i < kb->diffnumber; i++) {
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
    fi = fbi / 32;
    li = mbi / 32;
    fm = firstmask(fbi & 31);
    lm = lastmask(mbi & 31);
    if (fi == li) { /* in same word */
      tmp_par = d[fi] & fm & lm;
    } else {
      tmp_par = (d[fi] & fm) ^ (d[li] & lm);
      for (ri = fi + 1; ri < li; ri++) tmp_par ^= d[ri];
    } /* still need to parity tmp_par */
    if (((inh_data[i / 32] & bt_mask(i)) ? 1 : 0) == parity(tmp_par)) {
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
    fi = fbi / 32;
    li = mbi / 32;
    fm = firstmask(fbi & 31);
    lm = lastmask(mbi & 31);
    if (fi == li) { /* in same word */
      tmp_par = d[fi] & fm & lm;
    } else {
      tmp_par = (d[fi] & fm) ^ (d[li] & lm);
      for (ri = fi + 1; ri < li; ri++) tmp_par ^= d[ri];
    }                /* still need to parity tmp_par */
    if (lbi < mbi) { /* end of interval, give zero parity */
      tmp_par = 0;
    }
    parityresult |= parity(tmp_par); /* save parity */
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

  /* update outgoing info leakagebits */
  kb->leakagebits += lost_bits;

  /* mark message for sending */
  insert_sendpacket((char *)out_head, out_head->bytelength);

  return 0;
}

/* function to initiate a BICONF procedure on Bob side. Basically sends out a
   package calling for a BICONF reply. Parameter is a thread pointer, and
   the return value is 0 or an error code in case something goes wrong.   */
int initiate_biconf(struct keyblock *kb) {
  struct ERRC_ERRDET_6 *h6; /* header for that message */
  unsigned int seed;        /* seed for permutation */

  h6 = (struct ERRC_ERRDET_6 *)malloc2(sizeof(struct ERRC_ERRDET_6));
  if (!h6) return 60;

  /* prepare seed */
  seed = get_r_seed();

  /* update state variables */
  kb->biconflength = kb->workbits; /* old was /2 - do we still need this? */
  kb->RNG_state = seed;

  /* generate local bit string for test mask */
  generate_BICONF_bitstring(kb);

  /* fill message */
  h6->tag = ERRC_PROTO_tag;
  h6->bytelength = sizeof(struct ERRC_ERRDET_6);
  h6->subtype = ERRC_ERRDET_6_subtype;
  h6->epoch = kb->startepoch;
  h6->number_of_epochs = kb->numberofepochs;
  h6->seed = seed;
  h6->number_of_bits = kb->biconflength;
  kb->binsearch_depth = 0; /* keep it to main buffer TODO: is this relevant? */

  /* submit message */
  insert_sendpacket((char *)h6, h6->bytelength);
  return 0;
}

/* start the parity generation process on Alice side. parameter contains the
   input message. Reply is 0 on success, or an error message. Should create
   a BICONF response message */
int generate_biconfreply(char *receivebuf) {
  struct ERRC_ERRDET_6 *in_head; /* holds received message header */
  struct ERRC_ERRDET_7 *h7;      /* holds response message header */
  struct keyblock *kb;           /* points to thread info */
  int bitlen;                    /* number of bits requested */

  /* get pointers for header...*/
  in_head = (struct ERRC_ERRDET_6 *)receivebuf;

  /* ...and find thread: */
  kb = get_thread(in_head->epoch);
  if (!kb) {
    fprintf(stderr, "epoch %08x: ", in_head->epoch);
    return 49;
  }

  /* update thread status */
  switch (kb->processingstate) {
    case PRS_PERFORMEDPARITY1:                /* just finished BICONF */
      kb->processingstate = PRS_DOING_BICONF; /* update state */
      kb->biconf_round = 0;                   /* first round */
      break;
    case PRS_DOING_BICONF: /* already did a biconf */
      kb->biconf_round++;  /* increment processing round; more checks? */
      break;
  }
  /* extract number of bits and seed */
  bitlen = in_head->number_of_bits; /* do more checks? */
  kb->RNG_state = in_head->seed;    /* check for 0?*/
  kb->biconflength = bitlen;

  /* prepare permutation list */
  /* old: prepare_permut_core(kb); */

  /* generate local (alice) version of test bit section */
  generate_BICONF_bitstring(kb);

  /* fill the response header */
  h7 = (struct ERRC_ERRDET_7 *)malloc2(sizeof(struct ERRC_ERRDET_7));
  if (!h7) return 61;
  h7->tag = ERRC_PROTO_tag;
  h7->bytelength = sizeof(struct ERRC_ERRDET_7);
  h7->subtype = ERRC_ERRDET_7_subtype;
  h7->epoch = kb->startepoch;
  h7->number_of_epochs = kb->numberofepochs;

  /* evaluate the parity (updated to use testbit buffer */
  h7->parity = single_line_parity(kb->testmarker, 0, bitlen - 1);

  /* update bitloss */
  kb->leakagebits++; /* one is lost */

  /* send out response header */
  insert_sendpacket((char *)h7, h7->bytelength);

  return 0; /* return nicely */
}

/* function to generate a single binary search request for a biconf cycle.
   takes a keyblock pointer and a length of the biconf block as a parameter,
   and returns an error or 0 on success.
   Takes currently the subset of the biconf subset and its complement, which
   is not very efficient: The second error could have been found using the
   unpermuted short sample with nuch less bits.
   On success, a binarysearch packet gets emitted with 2 list entries. */
int initiate_biconf_binarysearch(struct keyblock *kb, int biconflength) {
  unsigned int msg5size;          /* size of message */
  struct ERRC_ERRDET_5 *h5;       /* pointer to first message */
  unsigned int *h5_data, *h5_idx; /* data pointers */

  kb->diffnumber = 1;
  kb->diffidx[0] = 0;
  kb->diffidxe[0] = biconflength - 1;

  /* obsolete:
     kb->diffidx[1]=biconflength;kb->diffidxe[1]=kb->workbits-1; */

  kb->binsearch_depth = RUNLEVEL_SECONDPASS; /* only pass 1 */

  /* prepare message buffer for first binsearch message  */
  msg5size =
      sizeof(struct ERRC_ERRDET_5) /* header need */
      + sizeof(unsigned int)       /* parity data need */
      + 2 * sizeof(unsigned int);  /* indexing need for selection and compl */
  h5 = (struct ERRC_ERRDET_5 *)malloc2(msg5size);
  if (!h5) return 55;
  h5_data = (unsigned int *)&h5[1]; /* start of data */
  h5->tag = ERRC_PROTO_tag;
  h5->subtype = ERRC_ERRDET_5_subtype;
  h5->bytelength = msg5size;
  h5->epoch = kb->startepoch;
  h5->number_of_epochs = kb->numberofepochs;
  h5->number_entries = kb->diffnumber;
  h5->index_present = 4; /* NEW this round we have a start/stop table */

  /* keep local status and indicate the BICONF round to Alice */
  h5->runlevel = kb->binsearch_depth | RUNLEVEL_BICONF;

  /* prepare block index list of simple type 1, uncompressed uint32 */
  h5_idx = &h5_data[1];
  /* for index mode 4: */
  h5_idx[0] = 0; /* selected first bits */
  /* this information is IMPLICIT in the round 4 infromation and needs no
     transmission */
  /* h5_idx[2]=biconflength; h5_idx[3] = kb->workbits-biconflength-1;  */

  /* set parity */
  h5_data[0] =
      (single_line_parity(kb->testmarker, 0, biconflength / 2 - 1) << 31);

  /* increment lost bits */
  kb->leakagebits += 1;

  /* send out message */
  insert_sendpacket((char *)h5, msg5size);

  return 0;
}

/* function to proceed with the parity evaluation message. This function
   should start the Binary search machinery.
   Argument is receivebuffer as usual, returnvalue 0 on success or err code.
   Should spit out the first binary search message */

int start_binarysearch(char *receivebuf) {
  struct ERRC_ERRDET_4 *in_head; /* holds received message header */
  struct keyblock *kb;           /* points to thread info */
  int l0, l1;                    /* helpers;  number of words for bitarrays */

  /* get pointers for header...*/
  in_head = (struct ERRC_ERRDET_4 *)receivebuf;

  /* ...and find thread: */
  kb = get_thread(in_head->epoch);
  if (!kb) {
    fprintf(stderr, "epoch %08x: ", in_head->epoch);
    return 49;
  }

  /* prepare local parity info */
  kb->RNG_state = in_head->seed; /* new rng seed */
  prepare_permutation(kb);       /* also updates workbits */

  /* update partition numbers and leakagebits */
  kb->partitions0 = (kb->workbits + kb->k0 - 1) / kb->k0;
  kb->partitions1 = (kb->workbits + kb->k1 - 1) / kb->k1;

  /* freshen up internal info on bit numbers etc */
  kb->leakagebits += kb->partitions0 + kb->partitions1;

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
  kb->diffnumber = do_paritylist_and_diffs(kb, 0);
  if (kb->diffnumber == -1) return 74;
  kb->diffnumber_max = kb->diffnumber;

  /* reserve difference index memory for pass 0 */
  kb->diffidx =
      (unsigned int *)malloc2(kb->diffnumber * sizeof(unsigned int) * 2);
  if (!kb->diffidx) return 54;                 /* can't malloc */
  kb->diffidxe = &kb->diffidx[kb->diffnumber]; /* end of interval */

  /* now hand over to the procedure preoaring the first binsearch msg
     for the first pass 0 */

  return prepare_first_binsearch_msg(kb, 0);
}

/* function to process a binarysearch request. distinguishes between the two
   symmetries in the evaluation. This is onyl a wrapper.
   on alice side, it does a passive check; on bob side, it possibly corrects
   for errors. */
int process_binarysearch(char *receivebuf) {
  struct ERRC_ERRDET_5 *in_head; /* holds received message header */
  struct keyblock *kb;           /* points to thread info */

  /* get pointers for header...*/
  in_head = (struct ERRC_ERRDET_5 *)receivebuf;

  /* ...and find thread: */
  kb = get_thread(in_head->epoch);
  if (!kb) {
    fprintf(stderr, "binsearch 5 epoch %08x: ", in_head->epoch);
    return 49;
  }
  switch (kb->role) {
    case 0: /* alice, passive part in binsearch */
      return process_binsearch_alice(kb, in_head);
    case 1: /* bob role; active part in binsearch */
      return process_binsearch_bob(kb, in_head);
    default:
      return 56; /* illegal role */
  }
  return 0; /* keep compiler happy */
}

/* function to process a binarysearch request on bob identity. Checks parity
   lists and does corrections if necessary.
   initiates the next step (BICONF on pass 1) for the next round if ready.

   Note: uses globalvar biconf_rounds
*/
int process_binsearch_bob(struct keyblock *kb, struct ERRC_ERRDET_5 *in_head) {
  unsigned int *inh_data, *inh_idx;
  int i;
  struct ERRC_ERRDET_5 *out_head; /* for reply message */
  unsigned int *out_parity;       /* pointer to outgoing parity result info */
  unsigned int *out_match;        /* pointer to outgoing matching info */
  unsigned int *d = NULL;         /* points to internal key data */
  unsigned int *d2 = NULL; /* points to secondary to-be-corrected buffer */
  unsigned int matchresult = 0, parityresult = 0; /* for builduing outmsg */
  unsigned int fm, lm, tmp_par;                   /* for parity evaluation */
  int fbi, lbi, mbi, fi, li, ri;                  /* for parity evaluation */
  int lost_bits;  /* number of key bits revealed in this round */
  int thispass;   /* indincates the current pass */
  int biconfmark; /* indicates if this is a biconf round */

  inh_data = (unsigned int *)&in_head[1];          /* parity pattern */
  inh_idx = &inh_data[(kb->diffnumber + 31) / 32]; /* index or matching part */

  /* repair index according to previous basis match */
  fix_parity_intervals(kb, inh_idx);

  /* other stuff in local keyblk to update */
  kb->leakagebits += kb->diffnumber;           /* for incoming parity bits */
  kb->binsearch_depth = in_head->runlevel + 1; /* better some checks? */

  /* prepare outgoing message header */
  out_head = make_messagehead_5(kb);
  if (!out_head) return 58;
  out_parity = (unsigned int *)&out_head[1];
  out_match = &out_parity[((kb->diffnumber + 31) / 32)];

  lost_bits = kb->diffnumber; /* initially we will loose those for outgoing
                                 parity bits */

  /* make pass-dependent settings */
  thispass = (kb->binsearch_depth & RUNLEVEL_LEVELMASK) ? 1 : 0;

  switch (thispass) {
    case 0: /* level 0 */
      d = kb->mainbuf;
      break;
    case 1: /* level 1 */
      d = kb->permutebuf;
  }

  biconfmark = 0; /* default is no biconf */

  /* select test buffer in case this is a BICONF test round */
  if (kb->binsearch_depth & RUNLEVEL_BICONF) {
    biconfmark = 1;
    d = kb->testmarker;
    d2 = kb->permutebuf; /* for repairing also the permuted buffer */
  }

  /* go through all entries */
  for (i = 0; i < kb->diffnumber; i++) {
    matchresult <<= 1;
    parityresult <<= 1; /* make room for next bits */
    /* first, determine parity on local inverval */
    fbi = kb->diffidx[i];
    lbi = kb->diffidxe[i]; /* old bitindices */

    if (fbi > lbi) {   /* this is an empty message , don't count or correct */
      lost_bits -= 2;  /* No initial parity, no outgoing */
      goto skipparity; /* no more parity evaluation, skip rest */
    }
    if (fbi == lbi) { /* we have found the bit error */
      if (biconfmark) correct_bit(d2, fbi);
      correct_bit(d, fbi);
      kb->correctederrors++;
      lost_bits -= 2;           /* No initial parity, no outgoing */
      kb->diffidx[i] = fbi + 1; /* mark as emty */
      goto skipparity;          /* no more parity evaluation, skip rest */
    }
    mbi = fbi + (lbi - fbi + 1) / 2 - 1; /* new lower mid bitidx */
    fi = fbi / 32;
    li = mbi / 32;
    fm = firstmask(fbi & 31);
    lm = lastmask(mbi & 31);
    if (fi == li) { /* in same word */
      tmp_par = d[fi] & fm & lm;
    } else {
      tmp_par = (d[fi] & fm) ^ (d[li] & lm);
      for (ri = fi + 1; ri < li; ri++) tmp_par ^= d[ri];
    } /* still need to parity tmp_par */
    if (((inh_data[i / 32] & bt_mask(i)) ? 1 : 0) == parity(tmp_par)) {
      /* same parity, take upper half */
      fbi = mbi + 1;
      kb->diffidx[i] = fbi; /* update first bit idx */
      matchresult |= 1;     /* indicate match with incoming parity */
    } else {
      lbi = mbi;
      kb->diffidxe[i] = lbi; /* update last bit idx */
    }
    if (fbi == lbi) { /* end of interval, correct for error */
      if (biconfmark) correct_bit(d2, fbi);
      correct_bit(d, fbi);
      kb->correctederrors++;
      lost_bits--; /* we don't reveal anything on this one anymore */
      goto skipparity;
    }
    /* now, prepare new parity bit */
    mbi = fbi + (lbi - fbi + 1) / 2 - 1; /* new lower mid bitidx */
    fi = fbi / 32;
    li = mbi / 32;
    fm = firstmask(fbi & 31);
    lm = lastmask(mbi & 31);
    if (fi == li) { /* in same word */
      tmp_par = d[fi] & fm & lm;
    } else {
      tmp_par = (d[fi] & fm) ^ (d[li] & lm);
      for (ri = fi + 1; ri < li; ri++) tmp_par ^= d[ri];
    }                                /* still need to parity tmp_par */
    parityresult |= parity(tmp_par); /* save parity */
  skipparity:
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

  /* a blocklength k decides on a max number of rounds */
  if ((kb->binsearch_depth & RUNLEVEL_ROUNDMASK) <
      get_order_2((kb->processingstate == PRS_DOING_BICONF)
                      ? (kb->biconflength)
                      : (thispass ? kb->k1 : kb->k0))) {
    /* need to continue with this search; make packet 5 ready to send */
    kb->leakagebits += lost_bits;
    insert_sendpacket((char *)out_head, out_head->bytelength);
    return 0;
  }

  kb->leakagebits += lost_bits; /* correction for unreceived parity bits and
                                   nonsent parities */

  /* cleanup changed bits in the other permuted field */
  fix_permutedbits(kb);

  /* prepare for alternate round; start with re-evaluation of parity. */
  while (1) { /* just a break construction.... */
    kb->binsearch_depth = thispass ? RUNLEVEL_FIRSTPASS : RUNLEVEL_SECONDPASS;
    kb->diffnumber =
        do_paritylist_and_diffs(kb, 1 - thispass);       /* new differences */
    if (kb->diffnumber == -1) return 74;                 /* wrong pass */
    if ((kb->diffnumber == 0) && (thispass == 1)) break; /* no more errors */
    if (kb->diffnumber > kb->diffnumber_max) {           /* need more space */
      free2(kb->diffidx); /* re-assign diff buf */
      kb->diffnumber_max = kb->diffnumber;
      kb->diffidx =
          (unsigned int *)malloc2(kb->diffnumber * sizeof(unsigned int) * 2);
      if (!kb->diffidx) return 54;                 /* can't malloc */
      kb->diffidxe = &kb->diffidx[kb->diffnumber]; /* end of interval */
    }

    /* do basically a start_binarysearch for next round */
    return prepare_first_binsearch_msg(kb, 1 - thispass);
  }

  /* now we have finished a consecutive the second round; there are no more
     errors in both passes.  */

  /* check for biconf reply  */
  if (kb->processingstate ==
      PRS_DOING_BICONF) { /* we are finally finished
                                                                   with the
                             BICONF corrections */
    /* update biconf status */
    kb->biconf_round++;

    /* eventully generate new biconf request */
    if (kb->biconf_round < biconf_rounds) {
      return initiate_biconf(kb); /* request another one */
    }
    /* initiate the privacy amplificaton */
    return initiate_privacyamplification(kb);
  }

  /* we have no more errors in both passes, and we were not yet
     in BICONF mode */

  /* initiate the BICONF state */
  kb->processingstate = PRS_DOING_BICONF;
  kb->biconf_round = 0; /* first BICONF round */
  return initiate_biconf(kb);
}

/* start the parity generation process on bob's side. Parameter contains the
   parity reply form Alice. Reply is 0 on success, or an error message.
   Should either initiate a binary search, re-issue a BICONF request or
   continue to the parity evaluation. */
int receive_biconfreply(char *receivebuf) {
  struct ERRC_ERRDET_7 *in_head; /* holds received message header */
  struct keyblock *kb;           /* points to thread info */
  int localparity;

  /* get pointers for header...*/
  in_head = (struct ERRC_ERRDET_7 *)receivebuf;

  /* ...and find thread: */
  kb = get_thread(in_head->epoch);
  if (!kb) {
    fprintf(stderr, "epoch %08x: ", in_head->epoch);
    return 49;
  }

  kb->binsearch_depth = RUNLEVEL_SECONDPASS; /* use permuted buf */

  /* update incoming bit leakage */
  kb->leakagebits++;

  /* evaluate local parity */
  localparity = single_line_parity(kb->testmarker, 0, kb->biconflength - 1);

  /* eventually start binary search */
  if (localparity != in_head->parity) {
    return initiate_biconf_binarysearch(kb, kb->biconflength);
  }
  /* this location gets ONLY visited if there is no error in BICONF search */

  /* update biconf status */
  kb->biconf_round++;

  /* eventully generate new biconf request */
  if (kb->biconf_round < biconf_rounds) {
    return initiate_biconf(kb); /* request another one */
  }
  /* initiate the privacy amplificaton */
  return initiate_privacyamplification(kb);
}

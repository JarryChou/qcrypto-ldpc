#include "comms.h"

// COMMUNICATIONS HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */

/**
 * @brief Helper to insert a send packet in the sendpacket queue. 
 * 
 * This function has been modified to take in ptrs to last / next packet to reduce
 * reliance on global variables.
 * 
 * @param message Pointer to the structure
 * @param length length of strructure
 * @return int 0 if success, otherwise error code if malloc failure
 */
int insert_sendpacket(char *message, int length) {
  struct packet_to_send *newpacket, *lp;
  // pidx++; // Unused global int variable
  newpacket = (struct packet_to_send *)malloc2(sizeof(struct packet_to_send));
  if (!newpacket) return 43;

  newpacket->length = length;
  newpacket->packet = message; /* content */
  newpacket->next = NULL;

  lp = last_packet_to_send;
  if (lp) lp->next = newpacket; /* insetr in chain */
  last_packet_to_send = newpacket;
  if (!next_packet_to_send) next_packet_to_send = newpacket;

  /* for debug: send message, take identity from first available slot */
  /*dumpmsg(blocklist->content, message); */

  return 0; /* success */
}

/**
 * @brief Helper function to prepare a message containing a given sample of bits.
 * 
 * Modified to tell the other side about the Bell value for privacy amp in
 * the device indep mode
 * 
 * @param kb Pointer to the processblock
 * @param bitsneeded 
 * @param errormode 0 for normal error est, err*2^16 forskip
 * @param BellValue 
 * @return struct ERRC_ERRDET_0* pointer tht emessage, NULL on error
 */
struct ERRC_ERRDET_0 *fillsamplemessage(ProcessBlock *kb, int bitsneeded,
                                        int errormode, float BellValue) {
  int msgsize;                 /* keeps size of message */
  struct ERRC_ERRDET_0 *msg1;  /* for header to be sent */
  unsigned int *msg1_data;     /* pointer to data array */
  int i, bipo, rn_order;       /* counting index, bit position */
  unsigned int localdata, bpm; /* for bit extraction */

  /* prepare a structure to be sent out to the other side */
  /* first: get buffer.... */
  msgsize = sizeof(struct ERRC_ERRDET_0) + 4 * ((bitsneeded + 31) / 32);
  msg1 = (struct ERRC_ERRDET_0 *)malloc2(msgsize);
  if (!msg1) return NULL; /* cannot malloc */
  /* ...extract pointer to data structure... */
  msg1_data = (unsigned int *)&msg1[1];
  /* ..fill header.... */
  msg1->tag = ERRC_PROTO_tag;
  msg1->bytelength = msgsize;
  msg1->subtype = ERRC_ERRDET_0_subtype;
  msg1->epoch = kb->startepoch;
  msg1->number_of_epochs = kb->numberofepochs;
  msg1->seed = kb->RNG_state; /* this is the seed */
  msg1->numberofbits = bitsneeded;
  msg1->errormode = errormode;
  msg1->BellValue = BellValue;

  /* determine random number order needed for given bitlength */
  /* can this go into the processblock preparation ??? */
  rn_order = get_order_2(kb->initialbits);
  /* mark selected bits in this stream and fill this structure with bits */
  localdata = 0;                     /* storage for bits */
  for (i = 0; i < bitsneeded; i++) { /* count through all needed bits */
    do {                             /* generate a bit position */
      bipo = PRNG_value2(rn_order, &kb->RNG_state);
      if (bipo > kb->initialbits) continue;          /* out of range */
      bpm = bt_mask(bipo);                           /* bit mask */
      if (kb->testmarker[bipo / 32] & bpm) continue; /* already used */
      /* got finally a bit */
      kb->testmarker[bipo / 32] |= bpm; /* mark as used */
      if (kb->mainbuf[bipo / 32] & bpm) localdata |= bt_mask(i);
      if ((i & 31) == 31) {
        msg1_data[i / 32] = localdata;
        localdata = 0; /* reset buffer */
      }
      break; /* end rng search cycle */
    } while (1);
  }

  /* fill in last localdata */
  if (i & 31) {
    msg1_data[i / 32] = localdata;
  } /* there was something left */

  /* update thread structure with used bits */
  kb->leakagebits += bitsneeded;
  kb->processingstate = PRS_WAITRESPONSE1;

  return msg1; /* pointer to message */
}

/**
 * @brief Helper function for binsearch replies; mallocs and fills msg header
 * 
 * @param kb 
 * @return struct ERRC_ERRDET_5* 
 */
struct ERRC_ERRDET_5 *make_messagehead_5(ProcessBlock *kb) {
  int msglen; /* length of outgoing structure (data+header) */
  struct ERRC_ERRDET_5 *out_head;                 /* return value */
  msglen = ((kb->diffnumber + 31) / 32) * 4 * 2 + /* two bitfields */
           sizeof(struct ERRC_ERRDET_5);          /* ..plus one header */
  out_head = (struct ERRC_ERRDET_5 *)malloc2(msglen);
  if (!out_head) return NULL;
  out_head->tag = ERRC_PROTO_tag;
  out_head->bytelength = msglen;
  out_head->subtype = ERRC_ERRDET_5_subtype;
  out_head->epoch = kb->startepoch;
  out_head->number_of_epochs = kb->numberofepochs;
  out_head->number_entries = kb->diffnumber;
  out_head->index_present = 0;              /* this is an ordidary reply */
  out_head->runlevel = kb->binsearch_depth; /* next round */

  return out_head;
}

/**
 * @brief Function to prepare the first message head (header_5) for a binary search.
 * 
 *  This assumes
   that all the parity buffers have been malloced and the remote parities
   reside in the proper arrays. This function will be called several times for
   different passes; it expexts local parities to be evaluated already.
 * 
 * @param kb processblock ptr
 * @param pass pass number
 * @return int 0 on success, error code otherwise
 */
int prepare_first_binsearch_msg(ProcessBlock *kb, int pass) {
  int i, j;                             /* index variables */
  int k;                                /* length of processblocks */
  unsigned int *pd;                     /* parity difference bitfield pointer */
  unsigned int msg5size;                /* size of message */
  struct ERRC_ERRDET_5 *h5;             /* pointer to first message */
  unsigned int *h5_data, *h5_idx;       /* data pointers */
  unsigned int *d;                      /* temporary pointer on parity data */
  unsigned int resbuf, tmp_par, lm, fm; /* parity determination variables */
  int kdiff, fbi, lbi, fi, li, ri;      /* working variables for parity eval */
  int partitions; /* local partitions o go through for diff idx */

  switch (pass) { /* sort out specifics */
    case 0:       /* unpermutated pass */
      pd = kb->pd0;
      k = kb->k0;
      partitions = kb->partitions0;
      d = kb->mainbuf; /* unpermuted key */
      break;
    case 1: /* permutated pass */
      pd = kb->pd1;
      k = kb->k1;
      partitions = kb->partitions1;
      d = kb->permutebuf; /* permuted key */
      break;
    default:     /* illegal */
      return 59; /* illegal pass arg */
  }

  /* fill difference index memory */
  j = 0; /* index for mismatching blocks */
  for (i = 0; i < partitions; i++) {
    if (bt_mask(i) & pd[i / 32]) {       /* this block is mismatched */
      kb->diffidx[j] = i * k;            /* store bit index, not block index */
      kb->diffidxe[j] = i * k + (k - 1); /* last block */
      j++;
    }
  }
  /* mark pass/round correctly in kb */
  kb->binsearch_depth = (pass == 0 ? RUNLEVEL_FIRSTPASS : RUNLEVEL_SECONDPASS) |
                        0; /* first round */

  /* prepare message buffer for first binsearch message  */
  msg5size = sizeof(struct ERRC_ERRDET_5) /* header need */
             + ((kb->diffnumber + 31) / 32) *
                   sizeof(unsigned int)               /* parity data need */
             + kb->diffnumber * sizeof(unsigned int); /* indexing need */
  h5 = (struct ERRC_ERRDET_5 *)malloc2(msg5size);
  if (!h5) return 55;
  h5_data = (unsigned int *)&h5[1]; /* start of data */
  h5->tag = ERRC_PROTO_tag;
  h5->subtype = ERRC_ERRDET_5_subtype;
  h5->bytelength = msg5size;
  h5->epoch = kb->startepoch;
  h5->number_of_epochs = kb->numberofepochs;
  h5->number_entries = kb->diffnumber;
  h5->index_present = 1;              /* this round we have an index table */
  h5->runlevel = kb->binsearch_depth; /* keep local status */

  /* prepare block index list of simple type 1, uncompressed uint32 */
  h5_idx = &h5_data[((kb->diffnumber + 31) / 32)];
  for (i = 0; i < kb->diffnumber; i++) h5_idx[i] = kb->diffidx[i];

  /* prepare parity results */
  resbuf = 0;
  tmp_par = 0;
  for (i = 0; i < kb->diffnumber; i++) {          /* go through all processblocks */
    kdiff = kb->diffidxe[i] - kb->diffidx[i] + 1; /* left length */
    fbi = kb->diffidx[i];
    lbi = fbi + kdiff / 2 - 1; /* first and last bitidx */
    fi = fbi / 32;
    fm = firstmask(fbi & 31); /* beginning */
    li = lbi / 32;
    lm = lastmask(lbi & 31); /* end */
    if (li == fi) {          /* in same word */
      tmp_par = d[fi] & lm & fm;
    } else {
      tmp_par = (d[fi] & fm) ^ (d[li] & lm);
      for (ri = fi + 1; ri < li; ri++) tmp_par ^= d[ri];
    } /* tmp_par holds now a combination of bits to be tested */
    resbuf = (resbuf << 1) + parity(tmp_par);
    if ((i & 31) == 31) {
      h5_data[i / 32] = resbuf;
    }
  }
  if (i & 31)
    h5_data[i / 32] = resbuf << (32 - (i & 31)); /* last parity bits */

  /* increment lost bits */
  kb->leakagebits += kb->diffnumber;

  /* send out message */
  insert_sendpacket((char *)h5, msg5size);

  return 0;
}

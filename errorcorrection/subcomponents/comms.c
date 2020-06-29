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
int comms_insertSendPacket(char *message, int length) {
  struct packet_to_send *newpacket, *lp;
  // pidx++; // Unused global int variable
  newpacket = (struct packet_to_send *)malloc2(sizeof(struct packet_to_send));
  if (!newpacket) return 43;

  newpacket->length = length;
  newpacket->packet = message; /* content */
  newpacket->next = NULL;

  lp = lastPacketToSend;
  if (lp) lp->next = newpacket; /* insetr in chain */
  lastPacketToSend = newpacket;
  if (!nextPacketToSend) nextPacketToSend = newpacket;

  /* for debug: send message, take identity from first available slot */
  /*dumpmsg(processBlockDeque->content, message); */

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
 * @param bellValue 
 * @return EcPktHdr_QberEstBits* pointer tht emessage, NULL on error
 */
EcPktHdr_QberEstBits *fillsamplemessage(ProcessBlock *kb, int bitsneeded,
                                        int errormode, float bellValue) {
  int msgsize;                 /* keeps size of message */
  EcPktHdr_QberEstBits *msg1;  /* for header to be sent */
  unsigned int *msg1_data;     /* pointer to data array */
  int i, bipo, rn_order;       /* counting index, bit position */
  unsigned int localdata, bpm; /* for bit extraction */

  /* prepare a structure to be sent out to the other side */
  /* first: get buffer.... */
  msgsize = sizeof(EcPktHdr_QberEstBits) + 4 * ((bitsneeded + 31) / 32);
  msg1 = (EcPktHdr_QberEstBits *)malloc2(msgsize);
  if (!msg1) return NULL; /* cannot malloc */
  /* ...extract pointer to data structure... */
  msg1_data = (unsigned int *)&msg1[1];
  /* ..fill header.... */
  msg1->base.tag = EC_PACKET_TAG;
  msg1->base.totalLengthInBytes = msgsize;
  msg1->base.subtype = SUBTYPE_QBER_EST_BITS;
  msg1->base.epoch = kb->startEpoch;
  msg1->base.numberOfEpochs = kb->numberOfEpochs;
  msg1->seed = kb->rngState; /* this is the seed */
  msg1->numberofbits = bitsneeded;
  msg1->fixedErrorRate = errormode;
  msg1->bellValue = bellValue;

  /* determine random number order needed for given bitlength */
  /* can this go into the processblock preparation ??? */
  rn_order = get_order_2(kb->initialBits);
  /* mark selected bits in this stream and fill this structure with bits */
  localdata = 0;                     /* storage for bits */
  for (i = 0; i < bitsneeded; i++) { /* count through all needed bits */
    do {                             /* generate a bit position */
      bipo = PRNG_value2(rn_order, &kb->rngState);
      if (bipo > kb->initialBits) continue;          /* out of range */
      bpm = bt_mask(bipo);                           /* bit mask */
      if (kb->testedBitsMarker[bipo / 32] & bpm) continue; /* already used */
      /* got finally a bit */
      kb->testedBitsMarker[bipo / 32] |= bpm; /* mark as used */
      if (kb->mainBufPtr[bipo / 32] & bpm) localdata |= bt_mask(i);
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

  /* update processblock structure with used bits */
  kb->leakageBits += bitsneeded;
  kb->processingState = PRS_WAITRESPONSE1;

  return msg1; /* pointer to message */
}

/**
 * @brief Helper function for binsearch replies; mallocs and fills msg header
 * 
 * @param kb 
 * @return EcPktHdr_CascadeBinSearchMsg* 
 */
EcPktHdr_CascadeBinSearchMsg *make_messagehead_5(ProcessBlock *kb) {
  int msglen; /* length of outgoing structure (data+header) */
  EcPktHdr_CascadeBinSearchMsg *out_head;                 /* return value */
  msglen = ((kb->diffBlockCount + 31) / 32) * 4 * 2 + /* two bitfields */
           sizeof(EcPktHdr_CascadeBinSearchMsg);          /* ..plus one header */
  out_head = (EcPktHdr_CascadeBinSearchMsg *)malloc2(msglen);
  if (!out_head) return NULL;
  out_head->base.tag = EC_PACKET_TAG;
  out_head->base.totalLengthInBytes = msglen;
  out_head->base.subtype = SUBTYPE_CASCADE_BIN_SEARCH_MSG;
  out_head->base.epoch = kb->startEpoch;
  out_head->base.numberOfEpochs = kb->numberOfEpochs;
  out_head->number_entries = kb->diffBlockCount;
  out_head->index_present = 0;              /* this is an ordidary reply */
  out_head->runlevel = kb->binarySearchDepth; /* next round */

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
  EcPktHdr_CascadeBinSearchMsg *h5;             /* pointer to first message */
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
      d = kb->mainBufPtr; /* unpermuted key */
      break;
    case 1: /* permutated pass */
      pd = kb->pd1;
      k = kb->k1;
      partitions = kb->partitions1;
      d = kb->permuteBufPtr; /* permuted key */
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
  kb->binarySearchDepth = (pass == 0 ? RUNLEVEL_FIRSTPASS : RUNLEVEL_SECONDPASS) |
                        0; /* first round */

  /* prepare message buffer for first binsearch message  */
  msg5size = sizeof(EcPktHdr_CascadeBinSearchMsg) /* header need */
             + ((kb->diffBlockCount + 31) / 32) *
                   sizeof(unsigned int)               /* parity data need */
             + kb->diffBlockCount * sizeof(unsigned int); /* indexing need */
  h5 = (EcPktHdr_CascadeBinSearchMsg *)malloc2(msg5size);
  if (!h5) return 55;
  h5_data = (unsigned int *)&h5[1]; /* start of data */
  h5->base.tag = EC_PACKET_TAG;
  h5->base.subtype = SUBTYPE_CASCADE_BIN_SEARCH_MSG;
  h5->base.totalLengthInBytes = msg5size;
  h5->base.epoch = kb->startEpoch;
  h5->base.numberOfEpochs = kb->numberOfEpochs;
  h5->number_entries = kb->diffBlockCount;
  h5->index_present = 1;              /* this round we have an index table */
  h5->runlevel = kb->binarySearchDepth; /* keep local status */

  /* prepare block index list of simple type 1, uncompressed uint32 */
  h5_idx = &h5_data[((kb->diffBlockCount + 31) / 32)];
  for (i = 0; i < kb->diffBlockCount; i++) h5_idx[i] = kb->diffidx[i];

  /* prepare parity results */
  resbuf = 0;
  tmp_par = 0;
  for (i = 0; i < kb->diffBlockCount; i++) {          /* go through all processblocks */
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
  kb->leakageBits += kb->diffBlockCount;

  /* send out message */
  comms_insertSendPacket((char *)h5, msg5size);

  return 0;
}

/**
 * @brief Helper function to create a message header.
 * 
 * It creates the buffer for you based on subtype & populates the packet with information that all packets include.
 * 
 * @param resultingBufferPtr this function will create a buffer based on the subtype. This is the address of the pointer you want to use to refer to the buffer.
 * @param subtype 
 * @param totalLengthInBytes 
 * @param epoch 
 * @param numberOfEpochs 
 * @return int 0 if succcess otherwise error code
 */
int comms_createHeader(char** resultingBufferPtr, enum EcSubtypes subtype, unsigned int epoch, unsigned int numberOfEpochs) {
  EcPktHdr_Base* tmpBaseHdr; // Temporary pointer to point to the base
  unsigned int size;
  // Initialize a buffer 
  switch (subtype) {
    case SUBTYPE_QBER_EST_BITS_ACK:
      size = sizeof(EcPktHdr_QberEstBitsAck);
      *resultingBufferPtr = (EcPktHdr_QberEstBitsAck *)malloc2(size);
      break;
    case SUBTYPE_QBER_EST_REQ_MORE_BITS:
      size = sizeof(EcPktHdr_QberEstReqMoreBits);
      *resultingBufferPtr = (EcPktHdr_QberEstReqMoreBits *)malloc2(size);
      break;
  }
  // If buffer not initialized
  if (!(*resultingBufferPtr)) return 43;
  // Initialize the base of the header
  tmpBaseHdr = (EcPktHdr_Base*)(*resultingBufferPtr);
  tmpBaseHdr->tag = EC_PACKET_TAG;
  tmpBaseHdr->subtype = subtype;
  tmpBaseHdr->totalLengthInBytes = size;
  tmpBaseHdr->epoch = epoch;
  tmpBaseHdr->numberOfEpochs = numberOfEpochs;
  // Return success
  return 0;
}
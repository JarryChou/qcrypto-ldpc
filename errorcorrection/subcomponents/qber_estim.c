#include "qber_estim.h"

// ERROR ESTIMATION HELPER FUNCTIONS
/* ------------------------------------------------------------------------ */
/**
 * @brief helper function to prepare parity lists from original and unpermutated key.
 * 
 * No return value,
   as no errors are tested here.
 * 
 * @param kb pointer to processblock
 * @param d0 pointer to target parity buffer 0
 * @param d1 pointer to paritybuffer 1
 */
void prepare_paritylist1(ProcessBlock *kb, unsigned int *d0, unsigned int *d1) {
  prepare_paritylist_basic(kb->mainBufPtr, d0, kb->k0, kb->workbits);
  prepare_paritylist_basic(kb->permuteBufPtr, d1, kb->k1, kb->workbits);
  return;
}

// ERROR ESTIMATION MAIN FUNCTIONS
/* ------------------------------------------------------------------------ */
/**
 * @brief function to provide the number of bits needed in the initial error estimation.
 * 
 * Uses
   the maximum for either estimating the range for k0 with the desired error,
   or a sufficient separation from the no-error-left area. IS that fair?
 * 
 * @param e local error (estimated or guessed)
 * @return int returns a number of bits
 */
int testbits_needed(float e) {
  float ldi;
  int bn;
  ldi = USELESS_ERRORBOUND - e; /* difference to useless bound */
  bn = MAX((int)(e * INI_EST_SIGMA / ldi / ldi + .99),
           (int)(1. / e / DESIRED_K0_ERROR / DESIRED_K0_ERROR));
  return bn;
}

/**
 * @brief function to initiate the error estimation procedure.
 * 
 * Note: uses globalvar skip_qber_estimation
 * 
 * @param epoch start epoch
 * @return int 0 on success, error code otherwise
 */
int errorest_1(unsigned int epoch) {
  ProcessBlock *kb;        /* points to current processblock */
  float f_inierr, f_di;       /* for error estimation */
  int bits_needed;            /* number of bits needed to send */
  EcPktHdr_QberEstBits *msg1; /* for header to be sent */

  if (!(kb = getProcessBlock(epoch))) return 73; /* cannot find key block */

  /* set role in block to alice (initiating the seed) in keybloc struct */
  kb->processorRole = ALICE;
  /* seed the rng, (the state has to be kept with the processblock, use a lock
     system for the rng in case several ) */
  // kb->RNG_usage = 0; /* use simple RNG */
  if (!(kb->rngState = get_r_seed())) return 39;

  /*  evaluate how many bits are needed in this round */
  f_inierr = kb->initialError / 65536.; /* float version */

  if (arguments.skip_qber_estimation) { /* don't do error estimation */
    kb->skipQberEstim = 1;
    msg1 = fillsamplemessage(kb, 1, kb->initialError, kb->bellValue);
  } else {
    kb->skipQberEstim = 0;
    f_di = USELESS_ERRORBOUND - f_inierr;
    if (f_di <= 0) return 41; /* no error extractable */
    bits_needed = testbits_needed(f_inierr);

    if (bits_needed >= kb->initialBits) return 42; /* not possible */
    /* fill message with sample bits */
    msg1 = fillsamplemessage(kb, bits_needed, 0, kb->bellValue);
  }
  if (!msg1) return 43; /* a malloc error occured */

  /* send this structure to the other side */
  comms_insertSendPacket((char *)msg1, msg1->base.totalLengthInBytes);

  /* go dormant again.  */
  return 0;
}

/**
 * @brief function to process the first error estimation packet.
 * 
 * Initiates the error estimation, and prepares the next  package for transmission.
 *  Currently, it assumes only PRNG-based bit selections.
 * 
 * @param receivebuf pointer to the receivebuffer with both the header and the data section.
 * @return int 0 on success, otherwise error code
 */
int processReceivedQberEstBits(char *receivebuf) {
  EcPktHdr_QberEstBits *in_head; /* holds header */
  ProcessBlock *kb;           /* points to processblock info */
  unsigned int *in_data;         /* holds input data bits */
  /* int retval; */
  int i, rn_order, bipo;
  unsigned int bpm;
  EcPktHdr_QberEstReqMoreBits *h2; /* for more requests */
  EcPktHdr_QberEstBitsAck *h3; /* reply message */
  int replymode;            /* 0: terminate, 1: more bits, 2: continue */
  float localerror, ldi;
  int newbitsneeded = 0; /* to keep compiler happy */
  int overlapreply;

  /* get convenient pointers */
  in_head = (EcPktHdr_QberEstBits *)receivebuf;
  in_data = (unsigned int *)(&receivebuf[sizeof(EcPktHdr_QberEstBits)]);

  /* try to find overlap with existing files */
  overlapreply = check_epochoverlap(in_head->base.epoch, in_head->base.numberOfEpochs);
  if (overlapreply) {
    if (in_head->seed) return 46; // conflict
    if (!(kb = getProcessBlock(in_head->base.epoch))) return 48; // process block missing
  } else {
    if (!(in_head->seed)) return 51;

    /* create a processblock with the loaded files, get thead handle */
    if ((i = create_processblock(in_head->base.epoch, in_head->base.numberOfEpochs, 0.0, 0.0))) {
      fprintf(stderr, "create_processblock return code: %d epoch: %08x, number:%d\n", i, in_head->base.epoch, in_head->base.numberOfEpochs);
      return i; /* no success */
    }

    kb = getProcessBlock(in_head->base.epoch);
    if (!kb) return 48; /* should not happen */

    /* initialize the processblock with the type status, and with the info from the other side */
    kb->rngState = in_head->seed;
    kb->leakageBits = 0;
    kb->estimatedSampleSize = 0;
    kb->estimatedError = 0;
    kb->processorRole = BOB;
    kb->bellValue = in_head->bellValue;
  }

  // Update processblock with info from the packet
  kb->leakageBits += in_head->numberofbits;
  kb->estimatedSampleSize += in_head->numberofbits;

  /* do the error estimation */
  rn_order = get_order_2(kb->initialBits);
  for (i = 0; i < (in_head->numberofbits); i++) {
    while (True) { /* generate a bit position */
      bipo = PRNG_value2(rn_order, &kb->rngState);
      if (bipo > kb->initialBits) continue;          /* out of range */
      bpm = bt_mask(bipo);                           /* bit mask */
      if (kb->testedBitsMarker[bipo / 32] & bpm) continue; /* already used */
      /* got finally a bit */
      kb->testedBitsMarker[bipo / 32] |= bpm; /* mark as used */
      if (((kb->mainBufPtr[bipo / 32] & bpm) ? 1 : 0) ^
          ((in_data[i / 32] & bt_mask(i)) ? 1 : 0)) { /* error */
         kb->estimatedError += 1;
      }
      break;
    }
  }

  if (in_head->fixedErrorRate) { /* skip the error estimation */
    kb->skipQberEstim = True;
    localerror = (float)in_head->fixedErrorRate / 65536.0;
    replymode = replyMode_continue; /* skip error est part */
  } else {
    kb->skipQberEstim = False;
    /* make decision if to ask for more bits */
    localerror = (float)(kb->estimatedError) / (float)(kb->estimatedSampleSize);

    ldi = USELESS_ERRORBOUND - localerror;
    if (ldi <= 0.) { /* ignore key bits : send error number to terminate */
      replymode = replyMode_terminate;
    } else {
      newbitsneeded = testbits_needed(localerror);
      if (newbitsneeded > kb->initialBits) { /* will never work */
        replymode = replyMode_terminate;
      } else {
        if (newbitsneeded > kb->estimatedSampleSize) { /*  more bits */
          replymode = replyMode_moreBits;
        } else { /* send confirmation message */
          replymode = replyMode_continue;
        }
      }
    }
  }

  #ifdef DEBUG
  printf("processReceivedQberEstBits: estErr: %d errMode: %d \
 lclErr: %.4f estSampleSize: %d newBitsNeeded: %d initialBits: %d\n",
    kb->estimatedError, kb->skipQberEstim, 
    localerror, kb->estimatedSampleSize, newbitsneeded, kb->initialBits);
  #endif

  /* prepare reply message */
  if (replymode == replyMode_continue || replyMode_terminate) {
    // Prepare & send message
    i = comms_createHeader(&h3, SUBTYPE_QBER_EST_BITS_ACK, kb->startEpoch, kb->numberOfEpochs);
    if (i) return i;
    h3->tested_bits = kb->leakageBits;
    h3->number_of_errors = kb->estimatedError;
    comms_insertSendPacket((char *)h3, h3->base.totalLengthInBytes); /* error trap? */

    if (replymode == replyMode_terminate) {
      #ifdef DEBUG
      printf("Kill the processblock due to excessive errors\n");
      fflush(stdout);
      #endif
      remove_processblock(kb->startEpoch);
    } else { // replymode == replyMode_continue
      kb->processingState = PRS_KNOWMYERROR;
      kb->estimatedSampleSize = kb->leakageBits; /* is this needed? */
      /****** more to do here *************/
      /* calculate k0 and k1 for further uses */
      if (localerror < 0.01444) { kb->k0 = 64; } /* min bitnumber */
      else { kb->k0 = (int)(0.92419642 / localerror); }
      kb->k1 = 3 * kb->k0; /* block length second array */
    }
  } else if (replymode == replyMode_moreBits) {
      // Prepare & send message
      i = comms_createHeader(&h2, SUBTYPE_QBER_EST_REQ_MORE_BITS, kb->startEpoch, kb->numberOfEpochs);
      if (i) return i;
      h2->requestedbits = newbitsneeded - kb->estimatedSampleSize;
      comms_insertSendPacket((char *)h2, h2->base.totalLengthInBytes);
      // Set processblock params
      kb->skipQberEstim = 1;
      kb->processingState = PRS_GETMOREEST;
  } else { // logic error in code
    return 80;
  }

  return 0; /* everything went well */
}

/**
 * @brief function to reply to a request for more estimation bits. 
 * Currently, it uses only the PRNG method.
 * 
 * Just sends over a bunch of more estimaton bits.
 * 
 * @param receivebuf pointer to the receive buffer containing the message.
 * @return int 0 on success, error code otherwise
 */
int send_more_esti_bits(char *receivebuf) {
  EcPktHdr_QberEstReqMoreBits *in_head; /* holds header */
  ProcessBlock *kb;           /* poits to processblock info */
  int bitsneeded;                /* number of bits needed to send */
  EcPktHdr_QberEstBits *msg1;    /* for header to be sent */

  /* get pointers for header...*/
  in_head = (EcPktHdr_QberEstReqMoreBits *)receivebuf;

  /* ...and find processblock: */
  kb = getProcessBlock(in_head->base.epoch);
  if (!kb) {
    fprintf(stderr, "epoch %08x: ", in_head->base.epoch);
    return 49;
  }
  /* extract relevant information from processblock */
  bitsneeded = in_head->requestedbits;

  /* prepare a response message block / fill the response with sample bits */
  msg1 = fillsamplemessage(kb, bitsneeded, 0, kb->bellValue);
  if (!msg1) return 43; /* a malloc error occured */

  /* adjust message reply to hide the seed/indicate a second reply */
  msg1->seed = 0;
  /* send this structure to outgoing mailbox */
  comms_insertSendPacket((char *)msg1, msg1->base.totalLengthInBytes);

  /* everything is fine */
  return 0;
}

/**
 * @brief function to proceed with the error estimation reply.
 * 
 * Estimates if the
   block deserves to be considered further, and if so, prepares the permutation
   array of the key, and determines the parity functions of the first key.
 * 
 * @param receivebuf pointer to the receive buffer containing the message.
 * @return int 0 on success, error code otherwise
 */
int prepare_dualpass(char *receivebuf) {
  EcPktHdr_QberEstBitsAck *in_head; /* holds header */
  ProcessBlock *kb;           /* poits to processblock info */
  float localerror, ldi;
  int errormark, newbitsneeded;
  unsigned int newseed; /* seed for permutation */
  int msg4datalen;
  EcPktHdr_CascadeParityList *h4;    /* header pointer */
  unsigned int *h4_d0, *h4_d1; /* pointer to data tracks  */
  int retval;

  /* get pointers for header...*/
  in_head = (EcPktHdr_QberEstBitsAck *)receivebuf;

  /* ...and find processblock: */
  kb = getProcessBlock(in_head->base.epoch);
  if (!kb) {
    fprintf(stderr, "epoch %08x: ", in_head->base.epoch);
    return 49;
  }
  /* extract error information out of message */
  if (in_head->tested_bits != kb->leakageBits) return 52;
  kb->estimatedSampleSize = in_head->tested_bits;
  kb->estimatedError = in_head->number_of_errors;

  /* decide if to proceed */
  if (kb->skipQberEstim) {
    localerror = (float)kb->initialError / 65536.;
  } else {
    localerror = (float)kb->estimatedError / (float)kb->estimatedSampleSize;
    ldi = USELESS_ERRORBOUND - localerror;
    errormark = 0;
    if (ldi <= 0.) {
      errormark = 1; /* will not work */
    } else {
      newbitsneeded = testbits_needed(localerror);
      if (newbitsneeded > kb->initialBits) { /* will never work */
        errormark = 1;
      }
    }
    #ifdef DEBUG
    printf("prepare_dualpass kb. estSampleSize: %d estErr: %d errMode: %d lclErr: %.4f \
 ldi: %.4f newBitsNeeded: %d initBits: %d errMark: %d\n",
      kb->estimatedSampleSize, kb->estimatedError, kb->skipQberEstim, 
      localerror, ldi, newbitsneeded, kb->initialBits, errormark);
    fflush(stdout);
    #endif
    if (errormark) { /* not worth going */
      remove_processblock(kb->startEpoch);
      return 0;
    }
  }

  /* determine process variables */
  kb->processingState = PRS_KNOWMYERROR;

  kb->estimatedSampleSize = kb->leakageBits; /* is this needed? */

  /****** more to do here *************/
  /* calculate k0 and k1 for further uses */
  if (localerror < 0.01444) {
    kb->k0 = 64; /* min bitnumber */
  } else {
    kb->k0 = (int)(0.92419642 / localerror);
  }
  kb->k1 = 3 * kb->k0; /* block length second array */

  /* install new seed */
  // kb->RNG_usage = 0; /* use simple RNG */
  if (!(newseed = get_r_seed())) return 39;
  kb->rngState = newseed; /* get new seed for RNG */

  /* prepare permutation array */
  prepare_permutation(kb);

  /* prepare message 5 frame - this should go into prepare_permutation? */
  kb->partitions0 = (kb->workbits + kb->k0 - 1) / kb->k0;
  kb->partitions1 = (kb->workbits + kb->k1 - 1) / kb->k1;

  /* get raw buffer */
  msg4datalen = ((kb->partitions0 + 31) / 32 + (kb->partitions1 + 31) / 32) * 4;
  h4 = (EcPktHdr_CascadeParityList *)malloc2(sizeof(EcPktHdr_CascadeParityList) +
                                       msg4datalen);
  if (!h4) return 43; /* cannot malloc */
  /* both data arrays */
  h4_d0 = (unsigned int *)&h4[1];
  h4_d1 = &h4_d0[(kb->partitions0 + 31) / 32];
  h4->base.tag = EC_PACKET_TAG;
  h4->base.totalLengthInBytes = sizeof(EcPktHdr_CascadeParityList) + msg4datalen;
  h4->base.subtype = SUBTYPE_CASCADE_PARITY_LIST;
  h4->base.epoch = kb->startEpoch;
  h4->base.numberOfEpochs = kb->numberOfEpochs; /* length of the block */
  h4->seed = newseed;                        /* permutator seed */

  /* these are optional; should we drop them? */
  h4->k0 = kb->k0;
  h4->k1 = kb->k1;
  h4->totalbits = kb->workbits;

  /* evaluate parity in blocks */
  prepare_paritylist1(kb, h4_d0, h4_d1);

  /* update status */
  kb->processingState = PRS_PERFORMEDPARITY1;
  kb->leakageBits += kb->partitions0 + kb->partitions1;

  /* transmit message */
  retval = comms_insertSendPacket((char *)h4, h4->base.totalLengthInBytes);
  if (retval) return retval;

  return 0; /* go dormant again... */
}
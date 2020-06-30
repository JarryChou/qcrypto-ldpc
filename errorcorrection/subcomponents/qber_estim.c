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
void prepareParityList1(ProcessBlock *processBlock, unsigned int *d0, unsigned int *d1) {
  prepare_paritylist_basic(processBlock->mainBufPtr, d0, processBlock->k0, processBlock->workbits);
  prepare_paritylist_basic(processBlock->permuteBufPtr, d1, processBlock->k1, processBlock->workbits);
  return;
}

/**
 * @brief Helper function to calculate k0 and k1
 * 
 * @param processBlock 
 * @param localerror
 */
void setStateKnowMyErrorAndCalculatek0andk1(ProcessBlock *processBlock, float localerror) {
  /* determine process variables */
  processBlock->processingState = PRS_KNOWMYERROR;
  processBlock->estimatedSampleSize = processBlock->leakageBits; /* is this needed? */
  /****** more to do here *************/
  /* calculate k0 and k1 for further uses */
  if (localerror < 0.01444) { /* min bitnumber */
    processBlock->k0 = 64; 
  } else { 
    processBlock->k0 = (int)(0.92419642 / localerror); 
  }
  processBlock->k1 = 3 * processBlock->k0; /* block length second array */
}

/**
 * @brief Calculate local error and also return reply mode & new bits needed via pointers
 * 
 * @param processBlock 
 * @param replyModeResult pointer to replymode variable to return result to
 * @param newBitsNeededResult pointer to newbitsneeded variable to return result to
 * @return float local error
 */
float calculateLocalError(ProcessBlock *processBlock, enum REPLY_MODE* replyModeResult, int *newBitsNeededResult) {
  float localError, ldi;
  if (processBlock->skipQberEstim) { /* skip the error estimation */
    localError = (float)processBlock->initialError / 65536.0;
    *replyModeResult = REPLYMODE_CONTINUE; /* skip error est part */
  } else {
    processBlock->skipQberEstim = False;
    /* make decision if to ask for more bits */
    localError = (float)(processBlock->estimatedError) / (float)(processBlock->estimatedSampleSize);

    ldi = USELESS_ERRORBOUND - localError;
    if (ldi <= 0.) { /* ignore key bits : send error number to terminate */
      *replyModeResult = REPLYMODE_TERMINATE;
    } else {
      *newBitsNeededResult = testBitsNeeded(localError);
      if (*newBitsNeededResult > processBlock->initialBits) { /* will never work */
        *replyModeResult = REPLYMODE_TERMINATE;
      } else {
        if (*newBitsNeededResult > processBlock->estimatedSampleSize) { /*  more bits */
          *replyModeResult = REPLYMODE_MORE_BITS;
        } else { /* send confirmation message */
          *replyModeResult = REPLYMODE_CONTINUE;
        }
      }
    }
  }
  return localError;
}

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
int testBitsNeeded(float e) {
  float ldi;
  int bn;
  ldi = USELESS_ERRORBOUND - e; /* difference to useless bound */
  bn = MAX((int)(e * INI_EST_SIGMA / ldi / ldi + .99),
           (int)(1. / e / DESIRED_K0_ERROR / DESIRED_K0_ERROR));
  return bn;
}

// ERROR ESTIMATION MAIN FUNCTIONS
/* ------------------------------------------------------------------------ */
/**
 * @brief function to initiate the error estimation procedure.
 * 
 * Note: uses globalvar skipQberEstimation
 * 
 * @param epoch start epoch
 * @return int 0 on success, error code otherwise
 */
int qber_beginErrorEstimation(unsigned int epoch) {
  ProcessBlock *processBlock; /* points to current processblock */
  float f_inierr;       /* for error estimation */
  int bits_needed;            /* number of bits needed to send */
  EcPktHdr_QberEstBits *msg1; /* for header to be sent */

  if (!(processBlock = getProcessBlock(epoch))) return 73; /* cannot find key block */

  /* set role in block to alice (initiating the seed) in keybloc struct */
  processBlock->processorRole = INITIATOR;
  processBlock->skipQberEstim = arguments.skipQberEstimation;
  /* seed the rng, (the state has to be kept with the processblock, use a lock system for the rng in case several ) */
  // processBlock->RNG_usage = 0; /* use simple RNG */
  if (!(processBlock->rngState = generateRngSeed())) return 39;

  if (processBlock->skipQberEstim) {
    msg1 = comms_createQberEstBitsMsg(processBlock, 1, processBlock->initialError, processBlock->bellValue);
  } else {
    /* do a very rough estimation on how many bits are needed in this round */
    f_inierr = processBlock->initialError / 65536.; /* float version */
    if (USELESS_ERRORBOUND - f_inierr <= 0) return 41; /* no error extractable */
    bits_needed = testBitsNeeded(f_inierr);
    if (bits_needed >= processBlock->initialBits) return 42; /* not possible */
    /* fill message with sample bits */
    msg1 = comms_createQberEstBitsMsg(processBlock, bits_needed, 0, processBlock->bellValue);
  }

  if (!msg1) return 43; /* a malloc error occured */

  /* send this structure to the other side */
  comms_insertSendPacket((char *)msg1, msg1->base.totalLengthInBytes);

  return 0; /* go dormant again.  */
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
int qber_processReceivedQberEstBits(char *receivebuf) {
  EcPktHdr_QberEstBits *in_head; /* holds header */
  ProcessBlock *processBlock;           /* points to processblock info */
  unsigned int *in_data;         /* holds input data bits */
  /* int retval; */
  int i, rn_order, bipo;
  unsigned int bpm;
  EcPktHdr_QberEstReqMoreBits *h2; /* for more requests */
  EcPktHdr_QberEstBitsAck *h3; /* reply message */
  enum REPLY_MODE replymode;            /* 0: terminate, 1: more bits, 2: continue */
  float localerror = 0;
  int newbitsneeded = 0; /* to keep compiler happy */
  int overlapreply;

  /* get convenient pointers */
  in_head = (EcPktHdr_QberEstBits *)receivebuf;
  in_data = (unsigned int *)(&receivebuf[sizeof(EcPktHdr_QberEstBits)]);

  /* try to find overlap with existing files */
  overlapreply = check_epochoverlap(in_head->base.epoch, in_head->base.numberOfEpochs);
  if (overlapreply) {
    if (in_head->seed) return 46; // conflict
    if (!(processBlock = getProcessBlock(in_head->base.epoch))) return 48; // process block missing
  } else {
    if (!(in_head->seed)) return 51;

    /* create a processblock with the loaded files, get thead handle */
    if ((i = create_processblock(in_head->base.epoch, in_head->base.numberOfEpochs, 0.0, 0.0))) {
      fprintf(stderr, "create_processblock return code: %d epoch: %08x, number:%d\n", i, in_head->base.epoch, in_head->base.numberOfEpochs);
      return i; /* no success */
    }

    processBlock = getProcessBlock(in_head->base.epoch);
    if (!processBlock) return 48; /* should not happen */

    /* initialize the processblock with the type status, and with the info from the other side */
    processBlock->rngState = in_head->seed;
    processBlock->leakageBits = 0;
    processBlock->estimatedSampleSize = 0;
    processBlock->estimatedError = 0;
    processBlock->processorRole = FOLLOWER;
    processBlock->bellValue = in_head->bellValue;
  }

  // Update processblock with info from the packet
  processBlock->leakageBits += in_head->numberofbits;
  processBlock->estimatedSampleSize += in_head->numberofbits;

  /* do the error estimation */
  rn_order = log2Ceil(processBlock->initialBits);
  for (i = 0; i < (in_head->numberofbits); i++) {
    while (True) { /* generate a bit position */
      bipo = PRNG_value2(rn_order, &processBlock->rngState);
      if (bipo > processBlock->initialBits) continue;          /* out of range */
      bpm = bt_mask(bipo);                           /* bit mask */
      if (processBlock->testedBitsMarker[bipo / 32] & bpm) continue; /* already used */
      /* got finally a bit */
      processBlock->testedBitsMarker[bipo / 32] |= bpm; /* mark as used */
      if (((processBlock->mainBufPtr[bipo / 32] & bpm) ? 1 : 0) ^
          ((in_data[i / 32] & bt_mask(i)) ? 1 : 0)) { /* error */
         processBlock->estimatedError += 1;
      }
      break;
    }
  }
  processBlock->skipQberEstim = (in_head->fixedErrorRate) ? True : False;
  processBlock->initialError = in_head->fixedErrorRate;
  localerror = calculateLocalError(processBlock, &replymode, &newbitsneeded);

  #ifdef DEBUG
  printf("qber_processReceivedQberEstBits: estErr: %d errMode: %d \
    lclErr: %.4f estSampleSize: %d newBitsNeeded: %d initialBits: %d\n",
    processBlock->estimatedError, processBlock->skipQberEstim, 
    localerror, processBlock->estimatedSampleSize, newbitsneeded, processBlock->initialBits);
  #endif

  /* prepare reply message */
  if (replymode == REPLYMODE_CONTINUE || REPLYMODE_TERMINATE) {
    // Prepare & send message
    i = comms_createHeader((char**)&h3, SUBTYPE_QBER_EST_BITS_ACK, 0, processBlock);
    if (i) return i;
    h3->tested_bits = processBlock->leakageBits;
    h3->number_of_errors = processBlock->estimatedError;
    comms_insertSendPacket((char *)h3, h3->base.totalLengthInBytes); /* error trap? */

    if (replymode == REPLYMODE_TERMINATE) {
      #ifdef DEBUG
      printf("Kill the processblock due to excessive errors\n");
      fflush(stdout);
      #endif
      remove_processblock(processBlock->startEpoch);
    } else { // replymode == REPLYMODE_CONTINUE
      setStateKnowMyErrorAndCalculatek0andk1(processBlock, localerror);
    }
  } else if (replymode == REPLYMODE_MORE_BITS) {
      // Prepare & send message
      i = comms_createHeader((char**)&h2, SUBTYPE_QBER_EST_REQ_MORE_BITS, 0, processBlock);
      if (i) return i;
      h2->requestedbits = newbitsneeded - processBlock->estimatedSampleSize;
      comms_insertSendPacket((char *)h2, h2->base.totalLengthInBytes);
      // Set processblock params
      processBlock->skipQberEstim = 1;
      processBlock->processingState = PRS_GETMOREEST;
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
 * @param processBlock pointer to non null processblock
 * @param receivebuf pointer to the receive buffer containing the message.
 * @return int 0 on success, error code otherwise
 */
int qber_processMoreEstBitsReq(ProcessBlock *processBlock, char *receivebuf) {
  EcPktHdr_QberEstReqMoreBits *in_head; /* holds header */
  int bitsneeded;                /* number of bits needed to send */
  EcPktHdr_QberEstBits *msg1;    /* for header to be sent */

  /* get pointers for header...*/
  in_head = (EcPktHdr_QberEstReqMoreBits *)receivebuf;

  /* extract relevant information from processblock */
  bitsneeded = in_head->requestedbits;

  /* prepare a response message block / fill the response with sample bits */
  msg1 = comms_createQberEstBitsMsg(processBlock, bitsneeded, 0, processBlock->bellValue);
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
 * @param processBlock pointer to non null processblock
 * @param receivebuf pointer to the receive buffer containing the message.
 * @return int 0 on success, error code otherwise
 */
int qber_prepareDualPass(ProcessBlock *processBlock, char *receivebuf) {
  EcPktHdr_QberEstBitsAck *in_head; /* holds header */
  float localerror;
  enum REPLY_MODE replymode;
  int newbitsneeded, errorCode;
  unsigned int newseed; /* seed for permutation */
  int msg4datalen;
  EcPktHdr_CascadeParityList *h4;    /* header pointer */
  unsigned int *h4_d0, *h4_d1; /* pointer to data tracks  */
  int retval;

  /* get pointers for header...*/
  in_head = (EcPktHdr_QberEstBitsAck *)receivebuf;

  /* extract error information out of message */
  if (in_head->tested_bits != processBlock->leakageBits) return 52;
  processBlock->estimatedSampleSize = in_head->tested_bits;
  processBlock->estimatedError = in_head->number_of_errors;

  localerror = calculateLocalError(processBlock, &replymode, &newbitsneeded);

  #ifdef DEBUG
  printf("qber_prepareDualPass kb. estSampleSize: %d estErr: %d errMode: %d lclErr: %.4f \
      newBitsNeeded: %d initBits: %d replymode: %d\n",
      processBlock->estimatedSampleSize, processBlock->estimatedError, processBlock->skipQberEstim, 
      localerror, newbitsneeded, processBlock->initialBits, replymode);
  fflush(stdout);
  #endif

  if (replymode == REPLYMODE_TERMINATE) { /* not worth going */
    remove_processblock(processBlock->startEpoch);
    return 0;
  }

  setStateKnowMyErrorAndCalculatek0andk1(processBlock, localerror);

  /* install new seed */
  // processBlock->RNG_usage = 0; /* use simple RNG */
  if (!(newseed = generateRngSeed())) return 39;
  processBlock->rngState = newseed; /* get new seed for RNG */

  /* prepare permutation array */
  prepare_permutation(processBlock);

  /* prepare message 5 frame - this should go into prepare_permutation? */
  processBlock->partitions0 = (processBlock->workbits + processBlock->k0 - 1) / processBlock->k0;
  processBlock->partitions1 = (processBlock->workbits + processBlock->k1 - 1) / processBlock->k1;

  /* get raw buffer */
  msg4datalen = ((processBlock->partitions0 + 31) / 32 + (processBlock->partitions1 + 31) / 32) * 4;
  errorCode = comms_createHeader((char **)&h4, SUBTYPE_CASCADE_PARITY_LIST, msg4datalen, processBlock);
  if (errorCode) return errorCode;
  /* both data arrays */
  h4_d0 = (unsigned int *)&h4[1];
  h4_d1 = &h4_d0[(processBlock->partitions0 + 31) / 32];
  h4->seed = newseed;                        /* permutator seed */
  
  /* these are optional; should we drop them? */
  h4->k0 = processBlock->k0;
  h4->k1 = processBlock->k1;
  h4->totalbits = processBlock->workbits;

  /* evaluate parity in blocks */
  prepareParityList1(processBlock, h4_d0, h4_d1);

  /* update status */
  processBlock->processingState = PRS_PERFORMEDPARITY1;
  processBlock->leakageBits += processBlock->partitions0 + processBlock->partitions1;

  /* transmit message */
  retval = comms_insertSendPacket((char *)h4, h4->base.totalLengthInBytes);
  if (retval) return retval;

  return 0; /* go dormant again... */
}
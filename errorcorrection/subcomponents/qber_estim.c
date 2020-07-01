#include "qber_estim.h"

// ERROR ESTIMATION HELPER FUNCTIONS
/* ------------------------------------------------------------------------ */
/**
 * @brief helper function to prepare parity lists from original and unpermutated key.
 * 
 * No return value,
   as no errors are tested here.
 * 
 * @param pb pointer to processblock
 * @param d0 pointer to target parity buffer 0
 * @param d1 pointer to paritybuffer 1
 */
void prepareParityList1(ProcessBlock *processBlock, unsigned int *d0, unsigned int *d1) {
  helper_prepParityList(processBlock->mainBufPtr, d0, processBlock->k0, processBlock->workbits);
  helper_prepParityList(processBlock->permuteBufPtr, d1, processBlock->k1, processBlock->workbits);
  return;
}

/**
 * @brief Helper function to calculate k0 and k1
 * 
 * @param processBlock 
 */
void setStateKnowMyErrorAndCalculatek0andk1(ProcessBlock *processBlock) {
  /* determine process variables */
  processBlock->processingState = PSTATE_ERR_KNOWN;
  processBlock->estimatedSampleSize = processBlock->leakageBits; /* is this needed? */
  /****** more to do here *************/
  /* calculate k0 and k1 for further uses */
  if (processBlock->localError < 0.01444) { /* min bitnumber */
    processBlock->k0 = 64; 
  } else { 
    processBlock->k0 = (int)(0.92419642 / (processBlock->localError)); 
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
    localError = (float)processBlock->initialErrRate / 65536.0;
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
 * Sends SUBTYPE_QBER_EST_BITS packet
 * 
 * @param epoch start epoch
 * @return int 0 on success, error code otherwise
 */
int qber_beginErrorEstimation(unsigned int epoch) {
  ProcessBlock *processBlock; /* points to current processblock */
  float f_inierr;       /* for error estimation */
  int bits_needed;            /* number of bits needed to send */
  EcPktHdr_QberEstBits *msg1; /* for header to be sent */

  if (!(processBlock = pBlkMgmt_getProcessBlock(epoch))) return 73; /* cannot find key block */

  /* set role in block to alice (initiating the seed) in keybloc struct */
  processBlock->processorRole = QBER_EST_INITIATOR;
  processBlock->skipQberEstim = arguments.skipQberEstimation;
  /* seed the rng, (the state has to be kept with the processblock, use a lock system for the rng in case several ) */
  // processBlock->RNG_usage = 0; /* use simple RNG */
  if (rnd_generateRngSeed(&(processBlock->rngState))) return 39; // if an error code was produced

  if (processBlock->skipQberEstim) {
    msg1 = comms_createQberEstBitsMsg(processBlock, 1, processBlock->initialErrRate, processBlock->bellValue);
  } else {
    /* do a very rough estimation on how many bits are needed in this round */
    /* float version */
    f_inierr = processBlock->initialErrRate / 65536.;

    /* if no error extractable */
    if (USELESS_ERRORBOUND - f_inierr <= 0) 
      return 41; 

    bits_needed = testBitsNeeded(f_inierr);

    /* if not possible */
    if (bits_needed >= processBlock->initialBits) 
      return 42; 

    /* fill message with sample bits */
    msg1 = comms_createQberEstBitsMsg(processBlock, bits_needed, 0, processBlock->bellValue);
  }

  if (!msg1) return 43; /* a malloc error occured */

  /* send this structure to the other side */
  return comms_insertSendPacket((char *)msg1, msg1->base.totalLengthInBytes);
}

/**
 * @brief function to process the first error estimation packet.
 * 
 * Initiates the error estimation, and prepares the next  package for transmission.
 *  Currently, it assumes only PRNG-based bit selections.
 * 
 * @param receivebuf pointer to the receivebuffer with both the header and the data section.
 * @param actionResultPtr ptr to action result which will contain meta info on the outcome of this function call
 * @return int 0 on success, otherwise error code
 */
int qber_processReceivedQberEstBits(char *receivebuf, ActionResult *actionResultPtr) {
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
    if (!(processBlock = pBlkMgmt_getProcessBlock(in_head->base.epoch))) return 48; // process block missing
  } else {
    if (!(in_head->seed)) return 51;

    /* create a processblock with the loaded files, get thead handle */
    if ((i = pBlkMgmt_createProcessBlock(in_head->base.epoch, in_head->base.numberOfEpochs, 0.0, 0.0))) {
      fprintf(stderr, "pBlkMgmt_createProcessBlock return code: %d epoch: %08x, number:%d\n", i, in_head->base.epoch, in_head->base.numberOfEpochs);
      return i; /* no success */
    }

    processBlock = pBlkMgmt_getProcessBlock(in_head->base.epoch);
    if (!processBlock) return 48; /* should not happen */

    /* initialize the processblock with the type status, and with the info from the other side */
    processBlock->rngState = in_head->seed;
    processBlock->leakageBits = 0;
    processBlock->estimatedSampleSize = 0;
    processBlock->estimatedError = 0;
    processBlock->processorRole = QBER_EST_FOLLOWER;
    processBlock->bellValue = in_head->bellValue;
  }

  // Update processblock with info from the packet
  processBlock->leakageBits += in_head->numberofbits;
  processBlock->estimatedSampleSize += in_head->numberofbits;

  /* do the error estimation */
  rn_order = log2Ceil(processBlock->initialBits);
  for (i = 0; i < (in_head->numberofbits); i++) {
    while (True) { /* generate a bit position */
      bipo = rnd_getPrngValue2(rn_order, &processBlock->rngState);
      if (bipo > processBlock->initialBits) continue;          /* out of range */
      bpm = uint32AllZeroExceptAtN(bipo);                           /* bit mask */
      if (processBlock->testedBitsMarker[wordIndex(bipo)] & bpm) continue; /* already used */
      /* got finally a bit */
      processBlock->testedBitsMarker[wordIndex(bipo)] |= bpm; /* mark as used */
      if (((processBlock->mainBufPtr[wordIndex(bipo)] & bpm) ? 1 : 0) ^
          ((in_data[wordIndex(i)] & uint32AllZeroExceptAtN(i)) ? 1 : 0)) { /* error */
         processBlock->estimatedError += 1;
      }
      break;
    }
  }
  processBlock->skipQberEstim = (in_head->fixedErrorRate) ? True : False;
  processBlock->initialErrRate = in_head->fixedErrorRate;
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
    i = comms_createEcHeader((char**)&h3, SUBTYPE_QBER_EST_BITS_ACK, 0, processBlock);
    if (i) return i;
    h3->testedBits = processBlock->leakageBits;
    h3->numberOfErrors = processBlock->estimatedError;

    if (replymode == REPLYMODE_TERMINATE) {
      #ifdef DEBUG
      printf("Kill the processblock due to excessive errors\n");
      fflush(stdout);
      #endif
      pBlkMgmt_removeProcessBlock(processBlock->startEpoch);
      return comms_insertSendPacket((char *)h3, h3->base.totalLengthInBytes); /* error trap? */
    } else { // replymode == REPLYMODE_CONTINUE
      processBlock->localError = localerror;
      actionResultPtr->nextActionEnum = AR_DECISION_INVOLVING_PREFILLED_DATA;
      actionResultPtr->bufferToSend = (char *)h3;
      actionResultPtr->bufferLengthInBytes = h3->base.totalLengthInBytes;
      return 0;
    }
  } else if (replymode == REPLYMODE_MORE_BITS) {
    // Prepare & send message
    i = comms_createEcHeader((char**)&h2, SUBTYPE_QBER_EST_REQ_MORE_BITS, 0, processBlock);
    if (i) return i;
    h2->requestedbits = newbitsneeded - processBlock->estimatedSampleSize;
    // Set processblock params
    processBlock->skipQberEstim = 1;
    processBlock->processingState = PSTATE_AWAIT_ERR_EST_MORE_BITS;

    return comms_insertSendPacket((char *)h2, h2->base.totalLengthInBytes);
  } else { // logic error in code
    return 80;
  }
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
int qber_replyWithMoreBits(ProcessBlock *processBlock, char *receivebuf) {
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

  /* get pointers for header...*/
  in_head = (EcPktHdr_QberEstBitsAck *)receivebuf;

  /* extract error information out of message */
  if (in_head->testedBits != processBlock->leakageBits) return 52;
  processBlock->estimatedSampleSize = in_head->testedBits;
  processBlock->estimatedError = in_head->numberOfErrors;

  localerror = calculateLocalError(processBlock, &replymode, &newbitsneeded);

  #ifdef DEBUG
  printf("qber_prepareDualPass kb. estSampleSize: %d estErr: %d errMode: %d lclErr: %.4f \
      newBitsNeeded: %d initBits: %d replymode: %d\n",
      processBlock->estimatedSampleSize, processBlock->estimatedError, processBlock->skipQberEstim, 
      localerror, newbitsneeded, processBlock->initialBits, replymode);
  fflush(stdout);
  #endif

  if (replymode == REPLYMODE_TERMINATE) { /* not worth going */
    pBlkMgmt_removeProcessBlock(processBlock->startEpoch);
    return 0;
  }

  // else we are continuing to thhe next phase: error correction
  processBlock->localError = localerror;
  setStateKnowMyErrorAndCalculatek0andk1(processBlock);

  /* install new seed */
  // processBlock->RNG_usage = 0; /* use simple RNG */
  if (rnd_generateRngSeed(&newseed)) 
    return 39; // if there was an error code produced
  processBlock->rngState = newseed; /* get new seed for RNG */

  /* prepare permutation array */
  helper_prepPermutationWrapper(processBlock);

  /* prepare message 5 frame - this should go into helper_prepPermutationWrapper? */
  processBlock->partitions0 = (processBlock->workbits + processBlock->k0 - 1) / processBlock->k0;
  processBlock->partitions1 = (processBlock->workbits + processBlock->k1 - 1) / processBlock->k1;

  /* get raw buffer */
  msg4datalen = (wordCount(processBlock->partitions0) + wordCount(processBlock->partitions1)) * WORD_SIZE;
  errorCode = comms_createEcHeader((char **)&h4, SUBTYPE_CASCADE_PARITY_LIST, msg4datalen, processBlock);
  if (errorCode) return errorCode;
  /* both data arrays */
  h4_d0 = (unsigned int *)&h4[1];
  h4_d1 = &h4_d0[wordCount(processBlock->partitions0)];
  h4->seed = newseed;                        /* permutator seed */
  
  /* these are optional; should we drop them? */
  h4->k0 = processBlock->k0;
  h4->k1 = processBlock->k1;
  h4->totalbits = processBlock->workbits;

  /* evaluate parity in blocks */
  prepareParityList1(processBlock, h4_d0, h4_d1);

  /* update status */
  processBlock->processingState = PSTATE_PERFORMED_PARITY;
  processBlock->leakageBits += processBlock->partitions0 + processBlock->partitions1;

  /* transmit message */
  return comms_insertSendPacket((char *)h4, h4->base.totalLengthInBytes);
}
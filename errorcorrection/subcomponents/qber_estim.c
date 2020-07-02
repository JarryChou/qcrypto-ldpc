#include "qber_estim.h"

// ERROR ESTIMATION HELPER FUNCTIONS
/* ------------------------------------------------------------------------ */

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
  // Obtain qber estimation specific data 
  QberData *qberData = (QberData *)(processBlock->algorithmDataPtr);
  /* If skip the error estimation */
  if (qberData->skipQberEstim) {
    localError = (float)processBlock->initialErrRate / 65536.0;
     /* skip error est part */
    *replyModeResult = REPLYMODE_CONTINUE;
  } else {
    qberData->skipQberEstim = False;
    /* make decision if to ask for more bits */
    localError = (float)(qberData->estimatedError) / (float)(qberData->estimatedSampleSize);

    ldi = USELESS_ERRORBOUND - localError;
    /* ignore key bits : send error number to terminate */
    if (ldi <= 0.) { 
      *replyModeResult = REPLYMODE_TERMINATE;
    } else {
      *newBitsNeededResult = testBitsNeeded(localError);
      /* if it will never work */
      if (*newBitsNeededResult > processBlock->initialBits) { 
        *replyModeResult = REPLYMODE_TERMINATE;
      } else {
        /*  more bits needed */
        if (*newBitsNeededResult > qberData->estimatedSampleSize) { 
          *replyModeResult = REPLYMODE_MORE_BITS;
        } else { 
          /* send confirmation message */
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
  QberData *qberData;  // ptr to qber data in process block
  float f_inierr;       /* for error estimation */
  int bits_needed;            /* number of bits needed to send */
  EcPktHdr_QberEstBits *msg1; /* for header to be sent */

  if (!(processBlock = pBlkMgmt_getProcessBlk(epoch))) return 73; /* cannot find key block */
  qberData = (QberData *)(processBlock->algorithmDataPtr);

  /* set role in block to alice (initiating the seed) in keybloc struct */
  processBlock->processorRole = PROC_ROLE_QBER_EST_INITIATOR;
  qberData->skipQberEstim = arguments.skipQberEstimation;
  /* seed the rng, (the state has to be kept with the processblock, use a lock system for the rng in case several ) */
  // processBlock->RNG_usage = 0; /* use simple RNG */
  if (rnd_generateRngSeed(&(processBlock->rngState))) return 39; // if an error code was produced

  if (qberData->skipQberEstim) {
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
 * @param processBlock pointer to the processBlock. Note that as this variable may be null, the function currently doesn't use the passed in value.
 * @param receivebuf pointer to the receivebuffer with both the header and the data section.
 * @return int 0 on success, otherwise error code
 */
int qber_processReceivedQberEstBits(ProcessBlock *processBlock, char *receivebuf) {
  QberData *qberData;           // ptr to qber data in process block
  EcPktHdr_QberEstBits *in_head; /* holds header */
  // ProcessBlock *processBlock;           /* points to processblock info */
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
    if (!(processBlock = pBlkMgmt_getProcessBlk(in_head->base.epoch))) return 48; // process block missing
    qberData = (QberData *)(processBlock->algorithmDataPtr);
  } else {
    if (!(in_head->seed)) return 51;

    /* create a processblock with the loaded files, get thead handle */
    if ((i = pBlkMgmt_createProcessBlk(in_head->base.epoch, in_head->base.numberOfEpochs, 0.0, 0.0, False))) {
      fprintf(stderr, "pBlkMgmt_createProcessBlk return code: %d epoch: %08x, number:%d\n", i, in_head->base.epoch, in_head->base.numberOfEpochs);
      return i; /* no success */
    }

    processBlock = pBlkMgmt_getProcessBlk(in_head->base.epoch);
    if (!processBlock) return 48; /* should not happen */
    qberData = (QberData *)(processBlock->algorithmDataPtr);

    /* initialize the processblock with the type status, and with the info from the other side */
    processBlock->rngState = in_head->seed;
    processBlock->leakageBits = 0;
    qberData->estimatedSampleSize = 0;
    qberData->estimatedError = 0;
    processBlock->processorRole = PROC_ROLE_QBER_EST_FOLLOWER;
    processBlock->bellValue = in_head->bellValue;
  }

  // Update processblock with info from the packet
  processBlock->leakageBits += in_head->numberofbits;
  qberData->estimatedSampleSize += in_head->numberofbits;

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
         qberData->estimatedError += 1;
      }
      break;
    }
  }
  qberData->skipQberEstim = (in_head->fixedErrorRate) ? True : False;
  processBlock->initialErrRate = in_head->fixedErrorRate;
  localerror = calculateLocalError(processBlock, &replymode, &newbitsneeded);

  #ifdef DEBUG
  printf("qber_processReceivedQberEstBits: estErr: %d errMode: %d \
    lclErr: %.4f estSampleSize: %d newBitsNeeded: %d initialBits: %d\n",
    qberData->estimatedError, qberData->skipQberEstim, 
    localerror, qberData->estimatedSampleSize, newbitsneeded, processBlock->initialBits);
  #endif

  /* prepare reply message */
  if (replymode == REPLYMODE_CONTINUE || REPLYMODE_TERMINATE) {
    // Prepare & send message
    i = comms_createEcHeader((char**)&h3, SUBTYPE_QBER_EST_BITS_ACK, 0, processBlock);
    if (i) return i;
    h3->testedBits = processBlock->leakageBits;
    h3->numberOfErrors = qberData->estimatedError;

    if (replymode == REPLYMODE_TERMINATE) {
      // If not enough bits, so terminating
      #ifdef DEBUG
      printf("Kill the processblock due to excessive errors\n");
      fflush(stdout);
      #endif
      pBlkMgmt_removeProcessBlk(processBlock->startEpoch);
      return comms_insertSendPacket((char *)h3, h3->base.totalLengthInBytes); /* error trap? */
    } else { 
      // Else if we have successfully estimated QBER and continuing 
      processBlock->localError = localerror;
      return chooseEcAlgorithmAsQberFollower(processBlock, (char *)h3, h3->base.totalLengthInBytes);
    }
  } else if (replymode == REPLYMODE_MORE_BITS) {
    // Prepare & send message
    i = comms_createEcHeader((char**)&h2, SUBTYPE_QBER_EST_REQ_MORE_BITS, 0, processBlock);
    if (i) return i;
    h2->requestedbits = newbitsneeded - qberData->estimatedSampleSize;
    // Set processblock params
    qberData->skipQberEstim = True;
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
 * @brief Contains body for code to decide what algorithm to use after QBER estimation as the QBER follower
 * 
 * @param processBlock 
 * @param bufferToSend 
 * @param bufferLengthInBytes 
 * @return int error code
 */
int chooseEcAlgorithmAsQberFollower(ProcessBlock *processBlock, char *bufferToSend, unsigned int bufferLengthInBytes) {
  ALGORITHM_DECISION chosenAlgorithm;
  int errorCode = 0;

  // If we've reached this stage QBER is complete, so wrap up
  pBlkMgmt_finishQberEst(processBlock);

  // The chosenAlgorithm by the QBER_FOLLOWER is hardcoded (originally is ALG_CASCADE_CONTINUE_ROLES)
  // chosenAlgorithm = ALG_CASCADE_CONTINUE_ROLES;
  chosenAlgorithm = ALG_CASCADE_FLIP_ROLES;

  // Fill in the algorithm for the message
  ((EcPktHdr_QberEstBitsAck *)(bufferToSend))->algorithmEnum = chosenAlgorithm;

  // Perform whatever preparation work we need to do for the chosenAlgorithm
  switch (chosenAlgorithm) {
    case ALG_CASCADE_CONTINUE_ROLES:
      processBlock->processorRole = PROC_ROLE_EC_FOLLOWER;

      // Initialize cascade data and subtype packet management
      processBlock->algorithmPktMngr = (ALGORITHM_PKT_MNGR *)&ALG_PKT_MNGR_CASCADE_FOLLOWER;
      processBlock->algorithmDataMngr = (ALGORITHM_DATA_MNGR *)&ALG_DATA_MNGR_CASCADE;
      errorCode = processBlock->algorithmDataMngr->initData(processBlock);
      if (errorCode) return errorCode;

      cascade_calck0k1(processBlock);
      // Insert the packet
      return comms_insertSendPacket((char *)(bufferToSend), bufferLengthInBytes);
    case ALG_CASCADE_FLIP_ROLES:
      // QBER_FOLLOWER will now be the one initiating the error correction procedure
      processBlock->processorRole = PROC_ROLE_EC_INITIATOR;

      // Initialize cascade data and subtype packet management
      processBlock->algorithmPktMngr = (ALGORITHM_PKT_MNGR *)&ALG_PKT_MNGR_CASCADE_INITIATOR;
      processBlock->algorithmDataMngr = (ALGORITHM_DATA_MNGR *)&ALG_DATA_MNGR_CASCADE;
      errorCode = processBlock->algorithmDataMngr->initData(processBlock);
      if (errorCode) return errorCode;

      // Send the packet first
      errorCode = comms_insertSendPacket((char *)(bufferToSend), bufferLengthInBytes);
      if (errorCode) 
        return errorCode;
      // Then send another packet that the EC_INITIATOR would send
      return cascade_initiateAfterQber(processBlock);
    case ALG_LDPC_CONTINUE_ROLES:
      return 81;
    case ALG_LDPC_FLIP_ROLES:
      return 81;
    default:
      fprintf(stderr, "Err 81 at chooseEcAlgorithmAsQberFollower\n");
      return 81;
  }
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
int qber_prepareErrorCorrection(ProcessBlock *processBlock, char *receivebuf) {
  QberData *qberData;  // ptr to qber data in process block
  EcPktHdr_QberEstBitsAck *in_head; /* holds header */
  enum REPLY_MODE replymode;
  int newbitsneeded;
  int errorCode;

  /* get pointers for header...*/
  in_head = (EcPktHdr_QberEstBitsAck *)receivebuf;

  // get qber data from process block
  qberData = (QberData *)(processBlock->algorithmDataPtr);

  /* extract error information out of message */
  if (in_head->testedBits != processBlock->leakageBits) return 52;
  qberData->estimatedSampleSize = in_head->testedBits;
  qberData->estimatedError = in_head->numberOfErrors;

  processBlock->localError = calculateLocalError(processBlock, &replymode, &newbitsneeded);

  #ifdef DEBUG
  printf("qber_prepareErrorCorrection kb. estSampleSize: %d estErr: %d errMode: %d lclErr: %.4f \
      newBitsNeeded: %d initBits: %d replymode: %d\n",
      qberData->estimatedSampleSize, qberData->estimatedError, qberData->skipQberEstim, 
      processBlock->localError, newbitsneeded, processBlock->initialBits, replymode);
  fflush(stdout);
  #endif

  if (replymode == REPLYMODE_TERMINATE) { /* not worth going */
    pBlkMgmt_removeProcessBlk(processBlock->startEpoch);
    return 0;
  }

  // else we are continuing to the next phase: error correction
  // This contains packet and data management
  pBlkMgmt_finishQberEst(processBlock);

  // NICE TO HAVE: Abstract this section out to reduce linkages to other subcomponents
  switch (in_head->algorithmEnum) {
    case ALG_CASCADE_CONTINUE_ROLES:
      processBlock->processorRole = PROC_ROLE_EC_INITIATOR;

      // Initialize cascade data and subtype packet management
      processBlock->algorithmPktMngr = (ALGORITHM_PKT_MNGR *)&ALG_PKT_MNGR_CASCADE_INITIATOR;
      processBlock->algorithmDataMngr = (ALGORITHM_DATA_MNGR *)&ALG_DATA_MNGR_CASCADE;
      errorCode = processBlock->algorithmDataMngr->initData(processBlock);
      if (errorCode) return errorCode;

      return cascade_initiateAfterQber(processBlock);
    case ALG_CASCADE_FLIP_ROLES:
      // QBER_INITIATOR is now the EC_FOLLOWER
      processBlock->processorRole = PROC_ROLE_EC_FOLLOWER;

      // Initialize cascade data and subtype packet management
      processBlock->algorithmPktMngr = (ALGORITHM_PKT_MNGR *)&ALG_PKT_MNGR_CASCADE_FOLLOWER;
      processBlock->algorithmDataMngr = (ALGORITHM_DATA_MNGR *)&ALG_DATA_MNGR_CASCADE;
      errorCode = processBlock->algorithmDataMngr->initData(processBlock);
      if (errorCode) return errorCode;

      cascade_calck0k1(processBlock);
      // Do nothing, await the cascade message from the now EC_INITIATOR
      return 0;
    case ALG_LDPC_CONTINUE_ROLES:
      return 81;
    case ALG_LDPC_FLIP_ROLES:
      return 81;
    default:
      fprintf(stderr, "Err 81 at qber_prepareErrorCorrection\n");
      return 81;
  }
}
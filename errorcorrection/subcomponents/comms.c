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
  PacketToSendNode *newpacket;
  // pidx++; // Unused global int variable
  newpacket = (PacketToSendNode*)malloc2(sizeof(PacketToSendNode));
  if (!newpacket) return 43;

  newpacket->length = length;
  newpacket->packet = message; /* content */
  newpacket->next = NULL;

  if (lastPacketToSend) 
    lastPacketToSend->next = newpacket; /* insert into chain */

  lastPacketToSend = newpacket;

  if (!nextPacketToSend) 
    nextPacketToSend = newpacket;

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
 * @param pb Pointer to the processblock
 * @param bitsneeded 
 * @param errormode 0 for normal error est, err*2^16 forskip
 * @param bellValue 
 * @return EcPktHdr_QberEstBits* pointer tht emessage, NULL on error
 */
EcPktHdr_QberEstBits *comms_createQberEstBitsMsg(ProcessBlock *processBlock, int bitsneeded, int errormode, float bellValue) {
  EcPktHdr_QberEstBits *msg1;  /* for header to be sent */
  unsigned int *msg1_data;     /* pointer to data array */
  int i, bipo, rn_order;       /* counting index, bit position */
  unsigned int localdata, bpm; /* for bit extraction */

  /* prepare a structure to be sent out to the other side */
  /* first: get buffer.... */
  i = comms_createEcHeader((char **)&msg1, SUBTYPE_QBER_EST_BITS, WORD_SIZE * wordCount(bitsneeded), processBlock);
  if (i) return NULL; /* cannot malloc */
  /* ..fill header.... */
  msg1->seed = processBlock->rngState; /* this is the seed */
  msg1->numberofbits = bitsneeded;
  msg1->fixedErrorRate = errormode;
  msg1->bellValue = bellValue;
  /* ...extract pointer to data structure... */
  msg1_data = (unsigned int *)&msg1[1];

  /* determine random number order needed for given bitlength */
  /* can this go into the processblock preparation ??? */
  rn_order = log2Ceil(processBlock->initialBits);
  /* mark selected bits in this stream and fill this structure with bits */
  localdata = 0;                     /* storage for bits */
  for (i = 0; i < bitsneeded; i++) { /* count through all needed bits */
    while (True) {                             /* generate a bit position */
      bipo = rnd_getPrngValue2(rn_order, &processBlock->rngState);
      if (bipo > processBlock->initialBits) continue;          /* out of range */
      bpm = uint32AllZeroExceptAtN(bipo);                                    /* bit mask */
      if (processBlock->testedBitsMarker[wordIndex(bipo)] & bpm) continue; /* already used */
      /* got finally a bit */
      processBlock->testedBitsMarker[wordIndex(bipo)] |= bpm; /* mark as used */
      if (processBlock->mainBufPtr[wordIndex(bipo)] & bpm) localdata |= uint32AllZeroExceptAtN(i);
      if ((modulo32(i)) == 31) {
        msg1_data[wordIndex(i)] = localdata;
        localdata = 0; /* reset buffer */
      }
      break; /* end rng search cycle */
    }
  }

  /* fill in last localdata */
  if (modulo32(i) != 0) {
    msg1_data[wordIndex(i)] = localdata;
  } /* there was something left */

  /* update processblock structure with used bits */
  processBlock->leakageBits += bitsneeded;
  processBlock->processingState = PSTATE_AWAIT_ERR_EST_RESP;

  return msg1; /* pointer to message */
}

/**
 * @brief Helper function for binsearch replies; mallocs and fills msg header
 * 
 * @param processBlock 
 * @return EcPktHdr_CascadeBinSearchMsg* 
 */
EcPktHdr_CascadeBinSearchMsg *comms_makeBinSrchMsgHead(ProcessBlock *processBlock, unsigned int indexPresent) {
  unsigned int sizeOfMsgBody;
  int errorCode;
  EcPktHdr_CascadeBinSearchMsg *outgoingMsgHead;
  switch (indexPresent) {
    case 0: 
      sizeOfMsgBody = wordCount(processBlock->diffBlockCount) * WORD_SIZE * 2; /* two bitfields */
      break;
    case 1:
      sizeOfMsgBody = wordCount(processBlock->diffBlockCount) * WORD_SIZE     // parity data need
          + processBlock->diffBlockCount * WORD_SIZE;                    // indexing need
      break;
    case 4:
      sizeOfMsgBody = 3 * WORD_SIZE;       /* parity data need and indexing need for selection and compl */
      break;
    default:
      fprintf(stderr, "Used unsupported indexPresent index %d", indexPresent);
      return NULL;
  }
  errorCode = comms_createEcHeader((char **)&outgoingMsgHead, SUBTYPE_CASCADE_BIN_SEARCH_MSG, sizeOfMsgBody, processBlock);
  if (errorCode) 
    return NULL;
  outgoingMsgHead->number_entries = processBlock->diffBlockCount;
  outgoingMsgHead->runlevel = processBlock->binarySearchDepth; /* next round */
  outgoingMsgHead->indexPresent = indexPresent; // 0: ordinary reply, 1: first msg, 4: this round we have a start/stop table
  return outgoingMsgHead;
}

/**
 * @brief Helper function to create a message header.
 * 
 * It creates the buffer for you based on subtype & populates the packet with information that all packets include.
 * 
 * @param resultingBufferPtr this function will create a buffer based on the subtype. This is the address of the pointer you want to use to refer to the buffer.
 * @param subtype 
 * @param additionalByteLength The header size will automatically be factored in. This includes any extra byte length beyond the header.
 * @param processBlock processBlock to obtain epoch metadata from
 * @return int 0 if succcess otherwise error code
 */
int comms_createEcHeader(char** resultingBufferPtr, enum EcSubtypes subtype, unsigned int additionalByteLength, ProcessBlock *processBlock) {
  EcPktHdr_Base* tmpBaseHdr; // Temporary pointer to point to the base
  unsigned int size = additionalByteLength; // Not good practice to reuse params, but this can be removed for micro-optimization
  switch (subtype) {
    case SUBTYPE_QBER_EST_BITS:               size += sizeof(EcPktHdr_QberEstBits);             break;
    case SUBTYPE_QBER_EST_BITS_ACK:           size += sizeof(EcPktHdr_QberEstBitsAck);          break;
    case SUBTYPE_QBER_EST_REQ_MORE_BITS:      size += sizeof(EcPktHdr_QberEstReqMoreBits);      break;
    case SUBTYPE_CASCADE_PARITY_LIST:         size += sizeof(EcPktHdr_CascadeParityList);       break;
    case SUBTYPE_CASCADE_BIN_SEARCH_MSG:      size += sizeof(EcPktHdr_CascadeBinSearchMsg);     break;
    case SUBTYPE_CASCADE_BICONF_INIT_REQ:     size += sizeof(EcPktHdr_CascadeBiconfInitReq);    break;
    case SUBTYPE_CASCADE_BICONF_PARITY_RESP:  size += sizeof(EcPktHdr_CascadeBiconfParityResp); break;
    case SUBTYPE_START_PRIV_AMP:              size += sizeof(EcPktHdr_StartPrivAmp);            break;
    default:
      #ifdef DEBUG
      printf("comms_createEcHeader processed unsupported type %d", subtype);
      fflush(stdout);
      #endif
      return 81;
  }
  // Initialize buffer
  *resultingBufferPtr = malloc2(size);
  // If buffer not initialized
  if (!(*resultingBufferPtr)) return 43;
  // Initialize the base of the header. Base is expected to be the first variable.
  tmpBaseHdr = (EcPktHdr_Base*)(*resultingBufferPtr);
  tmpBaseHdr->tag = EC_PACKET_TAG;
  tmpBaseHdr->subtype = subtype;
  tmpBaseHdr->totalLengthInBytes = size;
  tmpBaseHdr->epoch = processBlock->startEpoch;
  tmpBaseHdr->numberOfEpochs = processBlock->numberOfEpochs;
  // Return success
  return 0;
}
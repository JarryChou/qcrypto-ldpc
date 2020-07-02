#include "ecd2.h"

// HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */
/**
 * @brief Print error message based on err code
 * 
 * @param code 
 * @return int 
 */
int emsg(int code) {
  fprintf(stderr, "%s\n", errormessage[code]);
  return code;
}

/**
 * @brief Parse arguments on initialization
 * 
 * @param argc argument count
 * @param argv arguments from main
 * @return int error code, 0 if success
 */
int parse_options(int argc, char *argv[]) {
  int i, opt;
  float biconf_BER = 0;
  while ((opt = getopt(argc, argv, "c:s:r:d:f:l:q:Q:e:E:kJ:T:V:Ipb:B:i")) != EOF) {
    i = 0; /* for pairing filename-containing options */
    switch (opt) {
      case 'V': /* verbosity parameter */
        if (1 != sscanf(optarg, "%d", (int *)&arguments.verbosityLevel)) return -emsg(1);
        break;
      case 'q': i++; /* respondpipe, idx=7 */
      case 'Q': i++; /* querypipe, idx=6 */
      case 'l': i++; /* notify pipe, idx=5 */
      case 'f': i++; /* finalkeydir, idx=4 */
      case 'd': i++; /* rawkeydir, idx=3 */
      case 'r': i++; /* commreceivepipe, idx=2 */
      case 's': i++; /* commsendpipe, idx=1 */
      case 'c':      /* commandpipe, idx=0 */
        if (1 != sscanf(optarg, FNAM_FORMAT, arguments.fname[i])) return -emsg(2 + i);
        arguments.fname[i][FNAME_LENGTH - 1] = 0; /* security termination */
        break;
      case 'e': /* read in error threshold */
        if (1 != sscanf(optarg, "%f", &arguments.errorMargin)) return -emsg(10);
        if ((arguments.errorMargin < MIN_ERR_MARGIN) || (arguments.errorMargin > MAX_ERR_MARGIN))
          return -emsg(11);
        break;
      case 'E': /* expected error rate */
        if (1 != sscanf(optarg, "%f", &arguments.initialErrRate)) return -emsg(12);
        if ((arguments.initialErrRate < MIN_INI_ERR) || (arguments.initialErrRate > MAX_INI_ERR))
          return -emsg(13);
        break;
      case 'k': arguments.removeRawKeysAfterUse = 1; /* kill mode for raw files */
        break;
      case 'J': /* error rate generated outside eavesdropper */
        if (1 != sscanf(optarg, "%f", &arguments.intrinsicErrRate)) return -emsg(14);
        if ((arguments.intrinsicErrRate < 0) || (arguments.intrinsicErrRate > MAX_INTRINSIC))
          return -emsg(15);
        break;
      case 'T': /* runtime error behaviour */
        if (1 != sscanf(optarg, "%d", (int *)&arguments.runtimeErrorMode)) return -emsg(16);
        if ((arguments.runtimeErrorMode < 0) || (arguments.runtimeErrorMode > MAX_RUNTIME_ERROR_CONFIG))
          return -emsg(16);
        break;
      case 'I': arguments.skipQberEstimation = 1; /* skip initial error measurement */
        break;
      case 'i': arguments.bellmode = 1; /* expect a bell value for sneakage estimation */
        break;
      case 'p': arguments.disablePrivAmp = 1; /* disable privacy amplification */
        break;
      case 'b': /* set BICONF rounds */
        if (1 != sscanf(optarg, "%d", &arguments.biconfRounds)) return -emsg(76);
        if ((arguments.biconfRounds <= 0) || (arguments.biconfRounds > MAX_BICONF_ROUNDS))
          return -emsg(77);
        break;
      case 'B': /* take BER argument to determine biconf rounds */
        if (1 != sscanf(optarg, "%f", &biconf_BER)) return -emsg(78);
        if ((biconf_BER <= 0) || (biconf_BER > 1)) return -emsg(79);
        arguments.biconfRounds = (int)(-log(biconf_BER / AVG_BINSEARCH_ERR) / log(2));
        if (arguments.biconfRounds <= 0) arguments.biconfRounds = 1; /* at least one */
        if (arguments.biconfRounds > MAX_BICONF_ROUNDS) return -emsg(77);
        printf("biconf rounds used: %d\n", arguments.biconfRounds);
        break;
    }
  }

  /* checking parameter cosistency */
  for (i = 0; i < 8; i++)
    if (arguments.fname[i][0] == 0)
      return -emsg(17); /* all files and pipes specified ? */
  
  return 0;
}

/**
 * @brief Open file pipelines
 * 
 * @return int error code, 0 if success
 */
int open_pipelines() {
  struct stat cmdstat;          /* for probing pipe */
  /* command pipeline */
  if (stat(arguments.fname[handleId_commandPipe], &cmdstat)) return -emsg(18);
  if (!S_ISFIFO(cmdstat.st_mode)) return -emsg(19);
  if (!(arguments.fhandle[handleId_commandPipe] = fopen(arguments.fname[handleId_commandPipe], "r+"))) return -emsg(18);
  arguments.handle[handleId_commandPipe] = fileno(arguments.fhandle[handleId_commandPipe]);

  /* send pipeline */
  if (stat(arguments.fname[handleId_sendPipe], &cmdstat)) return -emsg(20);
  if (!S_ISFIFO(cmdstat.st_mode)) return -emsg(21);
  if ((arguments.handle[handleId_sendPipe] = open(arguments.fname[handleId_sendPipe], FIFO_OUTMODE)) == -1) return -emsg(20);

  /* receive pipeline */
  if (stat(arguments.fname[handleId_receivePipe], &cmdstat)) return -emsg(22);
  if (!S_ISFIFO(cmdstat.st_mode)) return -emsg(23);
  if ((arguments.handle[handleId_receivePipe] = open(arguments.fname[handleId_receivePipe], FIFO_INMODE)) == -1) return -emsg(22);

  /* notify pipeline */
  if (!(arguments.fhandle[handleId_notifyPipe] = fopen(arguments.fname[handleId_notifyPipe], "w+")))
    return -emsg(24);
  arguments.handle[handleId_notifyPipe] = fileno(arguments.fhandle[handleId_notifyPipe]);

  /* query pipeline */
  /*
  if (stat(arguments.fname[handleId_queryPipe], &cmdstat)) return -emsg(25);
  if (!S_ISFIFO(cmdstat.st_mode)) return -emsg(26);
  if (!(arguments.fhandle[handleId_queryPipe] = fopen(arguments.fname[handleId_queryPipe], "r+"))) return -emsg(25);
  arguments.handle[handleId_queryPipe] = fileno(arguments.fhandle[handleId_queryPipe]);
  */

  /* query response pipe */
  if (!(arguments.fhandle[handleId_queryRespPipe] = fopen(arguments.fname[handleId_queryRespPipe], "w+")))
    return -emsg(27);
  arguments.handle[handleId_queryRespPipe] = fileno(arguments.fhandle[handleId_queryRespPipe]);

  arguments.handle[handleId_rawKeyDir] = 0;
  arguments.handle[handleId_finalKeyDir] = 0;

  return 0;
}

/**
 * @brief wait for pipe event (with timeout) using select syscall
 * 
 * @param readqueue_ptr 
 * @param writequeue_ptr 
 * @param selectmax 
 * @param hasContentToSend 
 * @param timeout 
 * @return result from select syscall
 */
int has_pipe_event(fd_set* readqueue_ptr, fd_set* writequeue_ptr, int selectmax, Boolean hasContentToSend, struct timeval timeout) {
  /* prepare select call */
  FD_ZERO(readqueue_ptr);
  FD_ZERO(writequeue_ptr);
  // FD_SET(arguments.handle[handleId_queryPipe],readqueue_ptr); /* query pipe */
  FD_SET(arguments.handle[handleId_receivePipe],readqueue_ptr); /* receive pipe */
  FD_SET(arguments.handle[handleId_commandPipe],readqueue_ptr); /* command pipe */
  if (hasContentToSend) {
    FD_SET(arguments.handle[handleId_sendPipe], writequeue_ptr); /* content to send */
  }
  return select(selectmax, readqueue_ptr, writequeue_ptr, (fd_set *)0, &timeout);
}

/**
 * @brief write data from nextPacketToSend into sendpipe 
 * 
 * Uses static variable sendIndex
 * 
 * @return 0 if success, otherwise error code
 */
int write_into_sendpipe() {
  // #ifdef DEBUG
  // printf("Writing packet to sendpipe\n");
  // fflush(stdout);
  // #endif
  int i = nextPacketToSend->length - sendIndex;
  int retval = write(arguments.handle[handleId_sendPipe], &nextPacketToSend->packet[sendIndex], i);
  if (retval == -1) return -emsg(29);
  if (retval == i) { /* packet is sent */
    free2(nextPacketToSend->packet);
    PacketToSendNode*tmp_packetpointer = nextPacketToSend;
    nextPacketToSend = nextPacketToSend->next;
    if (lastPacketToSend == tmp_packetpointer)
      lastPacketToSend = NULL;
    free2(tmp_packetpointer); /* remove packet pointer */
    sendIndex = 0;           /* not digesting packet anymore */
  } else {
    sendIndex += retval;
  }

  return 0;
}

/**
 * @brief read command from cmdpipe into cmdInput
 * 
 * Uses static variable: input_last_index
 * 
 * @param cmdInput 
 * @return -1 if clean exit requested (empty cmd sent), 0 if success, otherwise error code
 */
int readFromCmdPipe(char* cmdInput) {
  int retval = read(arguments.handle[handleId_commandPipe], &cmdInput[input_last_index], 
      CMD_INBUF_LEN - 1 - input_last_index);
  if (retval < 0) return -1;
  input_last_index += retval;
  cmdInput[input_last_index] = '\0';
  // #ifdef DEBUG
  // printf("Rd from cmd pipe. cmdInput: %s\n", cmdInput);
  // fflush(stdout);
  // #endif
  if (input_last_index >= CMD_INBUF_LEN) return 75; /* overflow, parse later... */

  return 0;
}

/**
 * @brief Create a processBlock based on command read into cmdInput (terminated with 0, from the command pipe)
 * 
 * Uses static variable: input_last_index
 * 
 * @param dpnt 
 * @param cmdInput 
 * @param newEpochPtr pointer to return newEpoch value to.
 * @return 0 if success otherwise error code 
 */
int parseCmdAndInitProcessBlk(char* dpnt, char* cmdInput, unsigned int *newEpochPtr) {
  int i, sl;
  int fieldsAssigned;
  int consecutiveEpochs;
  float newesterror = 0; /* for initial parsing of a block */
  float bellValue;       /* for Ekert-type protocols */
  int errorCode = 0;

  /* parse command string */
  dpnt = index(cmdInput, '\n');
  if (dpnt) { /* we got a newline */
    dpnt[0] = 0;
    sl = strlen(cmdInput);
    fieldsAssigned = sscanf(cmdInput, "%x %i %f %f", newEpochPtr, &consecutiveEpochs, &newesterror, &bellValue);
    printf("got cmd: epoch: %08x, num: %d, esterr: %f fieldsAssigned: %d\n", *newEpochPtr, 
        consecutiveEpochs, newesterror, fieldsAssigned);
    #ifdef DEBUG
    fflush(stdout);
    #endif
    // Assign default values based on how many fields assigned
    switch (fieldsAssigned) {
      case 0:                                     /* no conversion */
        if (arguments.runtimeErrorMode != END_ON_ERR) break;/* nocomplain */
        return 30;                                /* not enough arguments */
      case 1: consecutiveEpochs = 1;                 /* use default epoch number */
      case 2: newesterror = arguments.initialErrRate; // use default initial error rate, or error rate passed in
      case 3: bellValue = PERFECT_BELL;           /* assume perfect Bell */
      case 4:                                     /* everything is there */
        // Parameter validation
        if (newesterror < 0 || newesterror > MAX_INI_ERR) { // error rate out of bounds
          if (arguments.runtimeErrorMode != END_ON_ERR) break;
          return 31;
        } else if (consecutiveEpochs < 1) { // epoch num out of bounds
          if (arguments.runtimeErrorMode != END_ON_ERR) break;
          return 32;
        } else if (check_epochoverlap(*newEpochPtr, consecutiveEpochs)) { // ensure start epoch & epoch num has not been used
          if (arguments.runtimeErrorMode != END_ON_ERR) break;
          return 33;
        }

        /* create new processBlock */
        if ((errorCode = pBlkMgmt_createProcessBlk(*newEpochPtr, consecutiveEpochs, newesterror, bellValue, True))) {
          if (arguments.runtimeErrorMode != END_ON_ERR) break;
          return errorCode; /* error reading files */
        }

        printf("got a processBlock and will send msg1\n");
        #ifdef DEBUG
        fflush(stdout);
        #endif
    }

    /* cmdInput and dpnt (move back rest) */
    for (i = 0; i < input_last_index - sl - 1; i++) cmdInput[i] = dpnt[i + 1];
    input_last_index -= sl + 1;
    cmdInput[input_last_index] = '\0'; /* repair index */
  }

  return errorCode;
}

/**
 * @brief read header from receivepipe into msgprotobuf, then malloc a buffer and point to it with readbuf
 *
 * Uses static variables: msgprotobuf, receiveIndex, readbuf
 * 
 * @return 0 if success, otherwise error message 
 */
int readHeaderFromReceivePipe() {
  #ifdef DEBUG
  printf("Read header from rcv pipe\n");
  fflush(stdout);
  #endif
  int retval = read(arguments.handle[handleId_receivePipe], &((char *)&msgprotobuf)[receiveIndex],
      sizeof(msgprotobuf) - receiveIndex);
  if (retval == -1) return 36; /* can that be better? */
  receiveIndex += retval;
  if (receiveIndex == sizeof(msgprotobuf)) {
    /* prepare for new buffer */
    readbuf = (char *)malloc2(msgprotobuf.totalLengthInBytes);
    if (!readbuf) return 37;
    /* transfer header */
    memcpy(readbuf, &msgprotobuf, sizeof(msgprotobuf));
  }

  return 0;
}

/**
 * @brief read message body into buffer pointed by readbuf
 * 
 * Uses static variables: msgprotobuf, receiveIndex, readbuf
 * 
 * @return 0  if success,  otherwise error message
 */
int readBodyFromReceivePipe() {
  #ifdef DEBUG
  printf("Read body from rcv pipe\n");
  fflush(stdout);
  #endif
  int retval = read(arguments.handle[handleId_receivePipe], &readbuf[receiveIndex],
      msgprotobuf.totalLengthInBytes - receiveIndex);
  if (retval == -1) return 36; /* can that be better? */
  receiveIndex += retval;
  if (receiveIndex == msgprotobuf.totalLengthInBytes) { /* got all */
    ReceivedPacketNode *msgp = (ReceivedPacketNode *)malloc2(sizeof(ReceivedPacketNode));
    if (!msgp) return 38;
    /* insert message in message chain */
    msgp->next = NULL;
    msgp->length = receiveIndex;
    msgp->packet = readbuf;
    ReceivedPacketNode *tmpRecvdPktNode = receivedPacketLinkedList;
    if (tmpRecvdPktNode) {
      while (tmpRecvdPktNode->next) tmpRecvdPktNode = tmpRecvdPktNode->next;
      tmpRecvdPktNode->next = msgp;
    } else {
      receivedPacketLinkedList = msgp;
    }
    receiveIndex = 0; /* ready for next one */
  }

  return 0;
}

// MAIN FUNCTION
/* ------------------------------------------------------------------------- */

/**
 * @brief Main code. Contains the main loop.
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[]) {
  // Variables
  int retval, errorCode = 0;                          // for checking if error occured in function, or return values in general
  int i;                                              // Helper iterator value
  // For select syscall system 
  fd_set readqueue, writequeue;                       /* for main event loop */
  int selectmax;                                      /* keeps largest handle for select call */
  struct timeval HALFSECOND = {0, 500000};
  struct timeval TENMILLISEC = {0, 10000};
  /* Buffers or pointers for packet receive or send management */
  ReceivedPacketNode *tmpRecvdPktNode = NULL;  /* index to go through the linked list */
  nextPacketToSend = NULL;                            /* no packets to be sent */
  lastPacketToSend = NULL;                   
  sendIndex = 0;                                      /* index of next packet to send */
  receiveIndex = 0;                                   /* index for reading in a longer packet */
  processBlockDeque = NULL;                           /* no active key blocks in memory */
  receivedPacketLinkedList = NULL;                    /* no receive packet s in queue */
  EcPktHdr_Base *tmpBaseHead = NULL;
  ProcessBlock *tmpPrcBlk = NULL;
  // Variables for command input
  char cmdInput[CMD_INBUF_LEN];                       /* For temporary storage of cmd input */
  cmdInput[0] = '\0';                                 /* buffer for commands */
  input_last_index = 0;                               /* input parsing */


  // Parameter passing
  errorCode = parse_options(argc, argv);
  if (errorCode) { return errorCode; }

  // Opening pipe and file handles
  errorCode = open_pipelines();
  if (errorCode) { return errorCode; }

  // Find largest handle for select call
  selectmax = 0;
  for (i = 0; i < handleId_numberOfHandles; i++)
    if (selectmax < arguments.handle[i]) selectmax = arguments.handle[i];
  selectmax += 1;

  // Main loop
  // Note: I decided to compartmentalize the code to make it more usable for its users (who are mostly not from
  // a programming background). This can translate to a slight dip in performance because of function calls and pointer 
  // usage, but I honestly don't think it will make a significant impact (if any) on performance, and at this point 
  // readability is much more important. May want to take this into account at the benchmarking phase of development 
  // though. Can also make the params global.
  while (True) {
    // Part 1 of the loop: use the select syscall to wait on events from multiple pipes 
    // -------------------------------------------------------------
    retval = has_pipe_event(&readqueue, &writequeue, selectmax, 
        nextPacketToSend || sendIndex, 
        (cmdInput[0] || receivedPacketLinkedList) ? TENMILLISEC : HALFSECOND);

    // Part 2 of the loop: if an error happened, retval would be -1
    // -------------------------------------------------------------
    // This is a pretty critical error, so runtimeErrorMode shouldn't apply here
    if (retval == -1) return -emsg(28);

    // If retval is not zero, there is something to read / write for the pipes
    if (retval != 0) {
      // If there is something to write into send pipe
      if (FD_ISSET(arguments.handle[handleId_sendPipe], &writequeue)) {
        errorCode = write_into_sendpipe(&sendIndex);
        if (errorCode && arguments.runtimeErrorMode == END_ON_ERR) {
          return -emsg(errorCode);
        }
      } 

      // If there is something to read from cmd pipeline
      if (FD_ISSET(arguments.handle[handleId_commandPipe], &readqueue)) {
        // Read from cmd pipeline into cmdInput
        errorCode = readFromCmdPipe((char *) &cmdInput);
        if (errorCode) {
          if (errorCode == -1) {
            break; // Clean exit program
          } else if (arguments.runtimeErrorMode == END_ON_ERR) {
            return -emsg(errorCode);
          }
        }
        // Process cmdInput
        unsigned int newEpoch = 0;
        errorCode = parseCmdAndInitProcessBlk(dpnt, (char *) &cmdInput, &newEpoch);
        // Begin qber estimation and send bits over 
        if (!errorCode) {
          errorCode = qber_beginErrorEstimation(newEpoch);
          // No decision is required, communications is encapsulated in the function above
          // Sends message of subtype SUBTYPE_QBER_EST_BITS
        }
        // If an error occured on either step, sound off if needed
        if (errorCode && arguments.runtimeErrorMode == END_ON_ERR) {
          return -emsg(errorCode);
        }
      }

      // If there is something to read from receive pipeline
      if (FD_ISSET(arguments.handle[handleId_receivePipe], &readqueue)) {
        if (receiveIndex < sizeof(EcPktHdr_Base)) {
          // Read header (tag & length)
          errorCode = readHeaderFromReceivePipe();
        } else {
          // Read body (subtype & data)
          errorCode = readBodyFromReceivePipe();
        }
        if (errorCode && arguments.runtimeErrorMode == END_ON_ERR) {
          return -emsg(errorCode);
        }
      }
      // If there is something to read from query pipeline (commented out query pipe)
      // if (FD_ISSET(arguments.handle[handleId_queryPipe], &readqueue)) { }

      // If errorCode was set but ignored, then arguments.runtimeErrorMode must  not be END_ON_ERR
      if (errorCode) {
        emsg(errorCode); // print error but do nothing
        errorCode = 0;
      }
    }

    // Part 3 of the loop: processing packets in the receivedPacketLinkedList
    // -------------------------------------------------------------
    // If there is something to process
    if ((tmpRecvdPktNode = receivedPacketLinkedList)) {
      // If packet is not an error correction packet based on tag, then throw error
      if (((unsigned int *)tmpRecvdPktNode->packet)[0] != EC_PACKET_TAG) { 
        errorCode = 44; 
      } else {
        // Set helper header
        tmpBaseHead = (EcPktHdr_Base *)tmpRecvdPktNode->packet;
        // Print debug message
        #ifdef DEBUG
        printf("rcv msg subtype: %d, len: %d\n", tmpBaseHead->subtype, tmpBaseHead->totalLengthInBytes);
        fflush(stdout);
        #endif

        // If it is the very first message that one would receive, thus no process block for it yet
        if (tmpBaseHead->subtype == SUBTYPE_QBER_EST_BITS) {
          errorCode = qber_processReceivedQberEstBits(NULL, tmpRecvdPktNode->packet);
        } else {
          // Extract the process block
          tmpPrcBlk = pBlkMgmt_getProcessBlk(tmpBaseHead->epoch);
          // If processblock is empty
          if (!tmpPrcBlk) {
            errorCode = 48;
          } else {
            // Validate the subtype number
            if (tmpBaseHead->subtype < tmpPrcBlk->algorithmPktMngr->FIRST_SUBTYPE
                || tmpBaseHead->subtype > tmpPrcBlk->algorithmPktMngr->LAST_SUBTYPE) {
              // Subtype received for the processBlock is outside that of the subtypes supported by the
              // algorithm in use
              errorCode = 45;
            } else {
              #ifdef DEBUG
              printf("pkt handler min: %d max: %d\n", 
                  tmpPrcBlk->algorithmPktMngr->FIRST_SUBTYPE, 
                  tmpPrcBlk->algorithmPktMngr->LAST_SUBTYPE);
              fflush(stdout);
              #endif
              // What's going on here:
              // Pass it into the current packet function handler 
              // (See definitions/algorithms/algorithms.c and algorithms/packet_manager.h)
              // tmpPrcBlk->algorithmPktMngr points to a const algorithm packet manager defined in algorithms.c
              // that pkt manager contains an array of pointers to functions which handle the incoming packet.
              // They are sorted by subtype.
              // So we use the subtype - the first subtype to get the index of the function handler in the array,
              // then we pass in the processBlock & packet into that function handler.
              (*(tmpPrcBlk->algorithmPktMngr->FUNC_HANDLERS))
                  [tmpBaseHead->subtype - tmpPrcBlk->algorithmPktMngr->FIRST_SUBTYPE](tmpPrcBlk, tmpRecvdPktNode->packet);
              #ifdef DEBUG
              printf("Pkt handler exec");
              fflush(stdout);
              #endif
            }
          }
        }  
      }

      if (errorCode) { /* an error occured */
        emsg(errorCode); // Always print errors
        if (arguments.runtimeErrorMode == IGNORE_ERRS_ON_OTHER_END) {
          errorCode = 0; // reset
        } else {
          return -errorCode;
        }
      }
      
      // Remove packet from receivedPacketLinkedList after processing it
      receivedPacketLinkedList = tmpRecvdPktNode->next;  /* update packet pointer */
      free2(tmpRecvdPktNode->packet);                    /* free data section... */
      free2(tmpRecvdPktNode);                            /* ...and pointer entry */
    }
  }
  
  // close nicely
  fclose(arguments.fhandle[handleId_commandPipe]);
  close(arguments.handle[handleId_sendPipe]);
  close(arguments.handle[handleId_receivePipe]);
  fclose(arguments.fhandle[handleId_notifyPipe]);
  // fclose(arguments.fhandle[handleId_queryPipe]);
  // fclose(arguments.fhandle[handleId_queryRespPipe]);
  return 0;
}
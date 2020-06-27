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
        if (1 != sscanf(optarg, "%d", &arguments.verbosity_level)) return -emsg(1);
        break;
      case 'q': i++; /* respondpipe, idx=7 */
      case 'Q': i++; /* querypipe, idx=6 */
      case 'l': i++; /* notify pipe, idx=5 */
      case 'f': i++; /* finalkeydir, idx=4 */
      case 'd': i++; /* rawkeydir, idx=3 */
      case 'r': i++; /* commreceivepipe, idx=2 */
      case 's': i++; /* commsendpipe, idx=1 */
      case 'c':      /* commandpipe, idx=0 */
        if (1 != sscanf(optarg, FNAMFORMAT, arguments.fname[i])) return -emsg(2 + i);
        arguments.fname[i][FNAMELENGTH - 1] = 0; /* security termination */
        break;
      case 'e': /* read in error threshold */
        if (1 != sscanf(optarg, "%f", &arguments.errormargin)) return -emsg(10);
        if ((arguments.errormargin < MIN_ERR_MARGIN) || (arguments.errormargin > MAX_ERR_MARGIN))
          return -emsg(11);
        break;
      case 'E': /* expected error rate */
        if (1 != sscanf(optarg, "%f", &arguments.initialerr)) return -emsg(12);
        if ((arguments.initialerr < MIN_INI_ERR) || (arguments.initialerr > MAX_INI_ERR))
          return -emsg(13);
        break;
      case 'k': arguments.remove_raw_keys_after_use = 1; /* kill mode for raw files */
        break;
      case 'J': /* error rate generated outside eavesdropper */
        if (1 != sscanf(optarg, "%f", &arguments.intrinsicerr)) return -emsg(14);
        if ((arguments.intrinsicerr < 0) || (arguments.intrinsicerr > MAX_INTRINSIC))
          return -emsg(15);
        break;
      case 'T': /* runtime error behaviour */
        if (1 != sscanf(optarg, "%d", &arguments.runtimeerrormode)) return -emsg(16);
        if ((arguments.runtimeerrormode < 0) || (arguments.runtimeerrormode > MAXRUNTIMEERROR))
          return -emsg(16);
        break;
      case 'I': arguments.skip_qber_estimation = 1; /* skip initial error measurement */
        break;
      case 'i': arguments.bellmode = 1; /* expect a bell value for sneakage estimation */
        break;
      case 'p': arguments.disable_privacyamplification = 1; /* disable privacy amplification */
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
  if ((arguments.handle[handleId_sendPipe] = open(arguments.fname[handleId_sendPipe], FIFOOUTMODE)) == -1) return -emsg(20);

  /* receive pipeline */
  if (stat(arguments.fname[handleId_receivePipe], &cmdstat)) return -emsg(22);
  if (!S_ISFIFO(cmdstat.st_mode)) return -emsg(23);
  if ((arguments.handle[handleId_receivePipe] = open(arguments.fname[handleId_receivePipe], FIFOINMODE)) == -1) return -emsg(22);

  /* notify pipeline */
  if (!(arguments.fhandle[handleId_notifyPipe] = fopen(arguments.fname[handleId_notifyPipe], "w+")))
    return -emsg(24);
  arguments.handle[handleId_notifyPipe] = fileno(arguments.fhandle[handleId_notifyPipe]);

  /* query pipeline */
  if (stat(arguments.fname[handleId_queryPipe], &cmdstat)) return -emsg(25);
  if (!S_ISFIFO(cmdstat.st_mode)) return -emsg(26);
  if (!(arguments.fhandle[handleId_queryPipe] = fopen(arguments.fname[handleId_queryPipe], "r+"))) return -emsg(25);
  arguments.handle[handleId_queryPipe] = fileno(arguments.fhandle[handleId_queryPipe]);

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
  FD_SET(arguments.handle[handleId_queryPipe],readqueue_ptr); /* query pipe */
  FD_SET(arguments.handle[handleId_receivePipe],readqueue_ptr); /* receive pipe */
  FD_SET(arguments.handle[handleId_commandPipe],readqueue_ptr); /* command pipe */
  if (hasContentToSend) {
    FD_SET(arguments.handle[handleId_sendPipe], writequeue_ptr); /* content to send */
  }
  return select(selectmax, readqueue_ptr, writequeue_ptr, (fd_set *)0, &timeout);
}

/**
 * @brief write data from next_packet_to_send into sendpipe 
 * 
 * Uses static variable send_index
 * 
 * @return 0 if success, otherwise error code
 */
int write_into_sendpipe() {
  #ifdef DEBUG
  printf("Writing packet to sendpipe\n");
  fflush(stdout);
  #endif
  int i = next_packet_to_send->length - send_index;
  int retval = write(arguments.handle[handleId_sendPipe], &next_packet_to_send->packet[send_index], i);
  if (retval == -1) return -emsg(29);
  if (retval == i) { /* packet is sent */
    free2(next_packet_to_send->packet);
    struct packet_to_send *tmp_packetpointer = next_packet_to_send;
    next_packet_to_send = next_packet_to_send->next;
    if (last_packet_to_send == tmp_packetpointer)
      last_packet_to_send = NULL;
    free2(tmp_packetpointer); /* remove packet pointer */
    send_index = 0;           /* not digesting packet anymore */
  } else {
    send_index += retval;
  }

  return 0;
}

/**
 * @brief read command from cmdpipe into cmd_input
 * 
 * Uses static variable: input_last_index
 * 
 * @param cmd_input 
 * @return -1 if clean exit requested (empty cmd sent), 0 if success, otherwise error code
 */
int read_from_cmdpipe(char* cmd_input) {
  int retval = read(arguments.handle[handleId_commandPipe], &cmd_input[input_last_index], 
      CMD_INBUFLEN - 1 - input_last_index);
  if (retval < 0) return -1;
  #ifdef DEBUG
  printf("Read something from cmd pipe\n");
  fflush(stdout);
  #endif
  input_last_index += retval;
  cmd_input[input_last_index] = '\0';
  if (input_last_index >= CMD_INBUFLEN) return -emsg(75); /* overflow, parse later... */

  return 0;
}

/**
 * @brief Create a thread and start qber based on the cmd stored in cimd_input
 * 
 * Uses static variable: input_last_index
 * 
 * @param dpnt 
 * @param cmd_input 
 * @return 0 if success otherwise error code 
 */
int create_thread_and_start_qber_using_cmd(char* dpnt, char* cmd_input) {
  int i, errcode, sl;
  /* parse command string */
  dpnt = index(cmd_input, '\n');
  if (dpnt) { /* we got a newline */
    dpnt[0] = 0;
    sl = strlen(cmd_input);
    errcode = process_command(cmd_input);
    if (errcode && (arguments.runtimeerrormode == END_ON_ERR)) {
      return -emsg(errcode); /* complain */
    }
    /* move back rest */
    for (i = 0; i < input_last_index - sl - 1; i++) cmd_input[i] = dpnt[i + 1];
    input_last_index -= sl + 1;
    cmd_input[input_last_index] = '\0'; /* repair index */
  }

  return 0;
}

/**
 * @brief process an input string, terminated with 0, from the command pipe
 * 
 * When you process a command, you spawn a new thread and also begin the QBER estimation process.
 * 
 * @param cmd_input buffer for command
 * @return 0 if success, otherwise return error code 
 */
int process_command(char *cmd_input) {
  int fieldsAssigned, errcode;
  unsigned int newepoch; /* command parser */
  int newepochnumber;
  float newesterror = 0; /* for initial parsing of a block */
  float bellValue;       /* for Ekert-type protocols */

  fieldsAssigned = sscanf(cmd_input, "%x %i %f %f", &newepoch, &newepochnumber, &newesterror, &bellValue);
  printf("got cmd: epoch: %08x, num: %d, esterr: %f fieldsAssigned: %d\n", newepoch, newepochnumber, newesterror, fieldsAssigned);
  #ifdef DEBUG
  fflush(stdout);
  #endif
  // Assign default values based on how many fields assigned
  switch (fieldsAssigned) {
    case 0:                                     /* no conversion */
      if (arguments.runtimeerrormode != END_ON_ERR) break;/* nocomplain */
      return -emsg(30);                         /* not enough arguments */
    case 1: newepochnumber = 1;                 /* use default epoch number */
    case 2: newesterror = arguments.initialerr; // use default initial error rate, or error rate passed in
    case 3: bellValue = PERFECT_BELL;           /* assume perfect Bell */
    case 4:                                     /* everything is there */
      // Parameter validation
      if (newesterror < 0 || newesterror > MAX_INI_ERR) { // error rate out of bounds
        if (arguments.runtimeerrormode != END_ON_ERR) break;
        return -emsg(31);
      } else if (newepochnumber < 1) { // epoch num out of bounds
        if (arguments.runtimeerrormode != END_ON_ERR) break;
        return -emsg(32);
      } else if (check_epochoverlap(newepoch, newepochnumber)) { // ensure start epoch & epoch num has not been used
        if (arguments.runtimeerrormode != END_ON_ERR) break;
        return -emsg(33);
      }

      /* create new thread */
      if ((errcode = create_thread(newepoch, newepochnumber, newesterror, bellValue))) {
        if (arguments.runtimeerrormode != END_ON_ERR) break;
        return errcode; /* error reading files */
      }

      /* Initiate first step of error estimation */
      if ((errcode = errorest_1(newepoch))) {
        if (arguments.runtimeerrormode != END_ON_ERR) break;
        return errcode; /* error initiating err est */
      }

      printf("got a thread and will send msg1\n");
      #ifdef DEBUG
      fflush(stdout);
      #endif
  }
  return 0;
}

/**
 * @brief read header from receivepipe into msgprotobuf, then malloc a buffer and point to it with readbuf
 *
 * Uses static variables: msgprotobuf, receive_index, readbuf
 * 
 * @return 0 if success, otherwise error message 
 */
int read_header_from_receivepipe() {
  #ifdef DEBUG
  printf("Read header from rcv pipe\n");
  fflush(stdout);
  #endif
  int retval = read(arguments.handle[handleId_receivePipe], &((char *)&msgprotobuf)[receive_index],
      sizeof(msgprotobuf) - receive_index);
  if (retval == -1) return -emsg(36); /* can that be better? */
  receive_index += retval;
  if (receive_index == sizeof(msgprotobuf)) {
    /* prepare for new buffer */
    readbuf = (char *)malloc2(msgprotobuf.bytelength);
    if (!readbuf) return -emsg(37);
    /* transfer header */
    memcpy(readbuf, &msgprotobuf, sizeof(msgprotobuf));
  }

  return 0;
}

/**
 * @brief read message body into buffer pointed by readbuf
 * 
 * Uses static variables: msgprotobuf, receive_index, readbuf
 * 
 * @return 0  if success,  otherwise error message
 */
int read_body_from_receivepipe() {
  #ifdef DEBUG
  printf("Read body from rcv pipe\n");
  fflush(stdout);
  #endif
  int retval = read(arguments.handle[handleId_receivePipe], &readbuf[receive_index],
      msgprotobuf.bytelength - receive_index);
  if (retval == -1) return -emsg(36); /* can that be better? */
  receive_index += retval;
  if (receive_index == msgprotobuf.bytelength) { /* got all */
    struct packet_received *msgp = (struct packet_received *)malloc2(
        sizeof(struct packet_received));
    if (!msgp) return -emsg(38);
    /* insert message in message chain */
    msgp->next = NULL;
    msgp->length = receive_index;
    msgp->packet = readbuf;
    struct packet_received *sbfp = rec_packetlist;
    if (sbfp) {
      while (sbfp->next) sbfp = sbfp->next;
      sbfp->next = msgp;
    } else {
      rec_packetlist = msgp;
    }
    receive_index = 0; /* ready for next one */
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
  int retval, errcode;          // for checking if error occured in function, or return values in general
  int i;                        // Helper iterator value
  // For select syscall system 
  fd_set readqueue, writequeue;  /* for main event loop */
  int selectmax;                 /* keeps largest handle for select call */
  struct timeval HALFSECOND = {0, 500000};
  struct timeval TENMILLISEC = {0, 10000};
  /* Buffers or pointers for packet receive / send management */
  struct packet_received *sbfp = NULL;  /* index to go through the linked list */
  next_packet_to_send = NULL; /* no packets to be sent */
  last_packet_to_send = NULL;
  send_index = 0;        /* index of next packet to send */
  receive_index = 0;     /* index for reading in a longer packet */
  blocklist = NULL;      /* no active key blocks in memory */
  rec_packetlist = NULL; /* no receive packet s in queue */
  // Variables for command input
  char cmd_input[CMD_INBUFLEN];  /* For temporary storage of cmd input */
  cmd_input[0] = '\0';    /* buffer for commands */
  input_last_index = 0;   /* input parsing */


  // Parameter passing
  errcode = parse_options(argc, argv);
  if (errcode) { return errcode; }

  // Opening pipe and file handles
  errcode = open_pipelines();
  if (errcode) { return errcode; }

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
        next_packet_to_send || send_index, 
        (cmd_input[0] || rec_packetlist) ? TENMILLISEC : HALFSECOND);

    // Part 2 of the loop: if an error happened, retval would be -1
    // -------------------------------------------------------------
    if (retval == -1) return -emsg(28);

    // If retval is not zero, there is something to read / write for the pipes
    if (retval != 0) {
      // If there is something to write into send pipe
      if (FD_ISSET(arguments.handle[handleId_sendPipe], &writequeue)) {
        errcode = write_into_sendpipe(&send_index);
        if (errcode) return errcode;
      } 

      // If there is something to read from cmd pipeline
      if (FD_ISSET(arguments.handle[handleId_commandPipe], &readqueue)) {
        // Read from cmd pipeline into cmd_input
        errcode = read_from_cmdpipe((char *) &cmd_input);
        if (errcode) {
          if (errcode == -1) break; // Clean exit program
          else return errcode;      // Otherwise an error really occurred
        }
        // Process cmd_input
        errcode = create_thread_and_start_qber_using_cmd(dpnt, (char *) &cmd_input);
        if (errcode) return errcode;
      }

      // If there is something to read from receive pipeline
      if (FD_ISSET(arguments.handle[handleId_receivePipe], &readqueue)) {
        if (receive_index < sizeof(struct ERRC_PROTO)) {
          // Read header (tag & length)
          errcode = read_header_from_receivepipe();
        } else {
          // Read body (subtype & data)
          errcode = read_body_from_receivepipe();
        }
        if (errcode) return errcode;
      }

      // If there is something to read from query pipeline (commented out because the body was empty)
      // if (FD_ISSET(arguments.handle[handleId_queryPipe], &readqueue)) { }
    }

    // Part 3 of the loop: processing packets in the rec_packetlist
    // -------------------------------------------------------------
    // If there is something to process
    if ((sbfp = rec_packetlist)) {
      // Get pointer to the packet buffer
      packet_to_process_buf = sbfp->packet;
      // If packet is not an error correction packet based on tag, then throw error
      if (((unsigned int *)packet_to_process_buf)[0] != ERRC_PROTO_tag) { return -emsg(44); }
      // Print debug message
      #ifdef DEBUG
      printf("received message, subtype: %d, len: %d\n", ((unsigned int *)packet_to_process_buf)[2], ((unsigned int *)packet_to_process_buf)[1]);
      fflush(stdout);
      #endif
      // Process packet based on the subtype
      switch (((unsigned int *)packet_to_process_buf)[2]) {
        case SUBTYPE_QBER_ESTIM: /* received an error estimation packet */
          errcode = process_esti_message_0(packet_to_process_buf);
          if (errcode) { /* an error occured */
            if (arguments.runtimeerrormode == IGNORE_ERRS_ON_OTHER_END) break;
            return -emsg(errcode);
          }
          break;

        case SUBTYPE_QBER_ESTIM_REQ_MORE_SAMPLES: /* received request for more bits */
          errcode = send_more_esti_bits(packet_to_process_buf);
          if (errcode) { /* an error occured */
            if (arguments.runtimeerrormode == IGNORE_ERRS_ON_OTHER_END) break;
            return -emsg(errcode);
          }
          break;

        case SUBTYPE_QBER_ESTIM_ACK: /* reveived error confirmation message */
          errcode = prepare_dualpass(packet_to_process_buf);
          if (errcode) { /* an error occured */
            if (arguments.runtimeerrormode == IGNORE_ERRS_ON_OTHER_END) break;
            return -emsg(errcode);
          }
          break;

        case SUBTYPE_CASCADE_PARITY_LIST: /* reveived parity list message */
          errcode = start_binarysearch(packet_to_process_buf);
          if (errcode) { /* an error occured */
            if (arguments.runtimeerrormode == IGNORE_ERRS_ON_OTHER_END) break;
            return -emsg(errcode);
          }
          break;

        case SUBTYPE_CASCADE_BIN_SEARCH_MSG: /* reveive a binarysearch message */
          errcode = process_binarysearch(packet_to_process_buf);
          if (errcode) { /* an error occured */
            if (arguments.runtimeerrormode == IGNORE_ERRS_ON_OTHER_END) break;
            return -emsg(errcode);
          }
          break;

        case SUBTYPE_CASCADE_BICONF_INIT_REQ: /* receive a BICONF initiating request */
          errcode = generate_biconfreply(packet_to_process_buf);
          if (errcode) { /* an error occured */
            if (arguments.runtimeerrormode == IGNORE_ERRS_ON_OTHER_END) break;
            return -emsg(errcode);
          }
          break;

        case SUBTYPE_CASCADE_BICONF_PARITY_RESP: /* receive a BICONF parity response */
          errcode = receive_biconfreply(packet_to_process_buf);
          if (errcode) { /* an error occured */
            if (arguments.runtimeerrormode == IGNORE_ERRS_ON_OTHER_END) break;
            return -emsg(errcode);
          }
          break;

        case SUBTYPE_START_PRIV_AMP: /* receive a privacy amplification start msg */
          errcode = receive_privamp_msg(packet_to_process_buf);
          if (errcode) { /* an error occured */
            if (arguments.runtimeerrormode == IGNORE_ERRS_ON_OTHER_END) break;
            return -emsg(errcode);
          }
          break;

        default: /* packet subtype not known */
          fprintf(stderr, "received subtype %d; ", ((unsigned int *)packet_to_process_buf)[2]);
          return -emsg(45);
      }

      /* printf("receive packet successfully digested\n");
         fflush(stdout); */
      
      // Remove packet from rec_packetlist
      rec_packetlist = sbfp->next; /* update packet pointer */
      free2(packet_to_process_buf);           /* free data section... */
      free2(sbfp);                 /* ...and pointer entry */
    }
  }
  
  // close nicely
  fclose(arguments.fhandle[handleId_commandPipe]);
  close(arguments.handle[handleId_sendPipe]);
  close(arguments.handle[handleId_receivePipe]);
  fclose(arguments.fhandle[handleId_notifyPipe]);
  fclose(arguments.fhandle[handleId_queryPipe]);
  fclose(arguments.fhandle[handleId_queryRespPipe]);
  return 0;
}

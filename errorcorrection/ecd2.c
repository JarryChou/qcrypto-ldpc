#include "ecd2.h"

// HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */
//  ERROR MANAGEMENT
int emsg(int code) {
  fprintf(stderr, "%s\n", errormessage[code]);
  return code;
}

// MAIN FUNCTION DECLARATIONS (OTHERS)
/*------------------------------------------------------------------------- */
/* process an input string, terminated with 0 */
int process_command(char *in) {
  int retval, retval2;
  unsigned int newepoch; /* command parser */
  int newepochnumber;
  float newesterror = 0; /* for initial parsing of a block */
  float BellValue;       /* for Ekert-type protocols */

  retval = sscanf(in, "%x %i %f %f", &newepoch, &newepochnumber, &newesterror,
                  &BellValue);
  printf("got cmd: epoch: %08x, num: %d, esterr: %f retval: %d\n", newepoch,
         newepochnumber, newesterror, retval);
  #ifdef DEBUG
  fflush(stdout);
  #endif
  switch (retval) {
    case 0:                            /* no conversion */
      if (runtimeerrormode > 0) break; /* nocomplain */
      return -emsg(30);                /* not enough arguments */
    case 1:                            /* no number and error */
      newepochnumber = 1;
    case 2: /* no error */
      newesterror = initialerr;
    case 3:                      /* only error is supplied */
      BellValue = 2. * sqrt(2.); /* assume perfect Bell */
    case 4:                      /* everything is there */
      if (newesterror < 0 || newesterror > MAX_INI_ERR) {
        if (runtimeerrormode > 0) break;
        return 31;
      }
      if (newepochnumber < 1) {
        if (runtimeerrormode > 0) break;
        return 32;
      }
      /* ok, we have a sensible command; check existing */
      if (check_epochoverlap(newepoch, newepochnumber)) {
        if (runtimeerrormode > 0) break;
        return 33;
      }
      /* create new thread */
      if ((retval2 = create_thread(newepoch, newepochnumber, newesterror,
                                   BellValue))) {
        if (runtimeerrormode > 0) break;
        return retval2; /* error reading files */
      }
      /* initiate first step of error estimation */
      if ((retval2 = errorest_1(newepoch))) {
        if (runtimeerrormode > 0) break;
        return retval2; /* error initiating err est */
      }

      printf("got a thread and will send msg1\n");
      #ifdef DEBUG
      fflush(stdout);
      #endif
  }
  return 0;
}

/* main code */
int main(int argc, char *argv[]) {
  int opt;
  int i, noshutdown;
  struct stat cmdstat;          /* for probing pipe */
  fd_set readqueue, writequeue; /* for main event loop */
  int retval, retval2;
  int selectmax; /* keeps largest handle for select call */
  struct timeval HALFSECOND = {0, 500000};
  struct timeval TENMILLISEC = {0, 10000};
  struct timeval timeout; /* for select command */
  int send_index;         /* for sending out packets */
  struct packet_to_send *tmp_packetpointer;
  int receive_index;             /* for receiving packets */
  struct ERRC_PROTO msgprotobuf; /* for reading header of receive packet */
  char *tmpreadbuf = NULL;       /* pointer to hold initial read buffer */
  struct packet_received *msgp;  /* temporary storage of message header */
  struct packet_received *sbfp;  /* index to go through the linked list */
  char instring[CMD_INBUFLEN];   /* for parsing commands */
  int ipt, sl;                   /* cmd input variables */
  char *dpnt;                    /* ditto */
  char *receivebuf;              /* pointer to the currently processed packet */
  float biconf_BER;              /* to keep biconf argument */

  /* parsing parameters */
  opterr = 0;
  while ((opt = getopt(argc, argv, "c:s:r:d:f:l:q:Q:e:E:kJ:T:V:Ipb:B:i")) !=
         EOF) {
    i = 0; /* for paring filename-containing options */
    switch (opt) {
      case 'V': /* verbosity parameter */
        if (1 != sscanf(optarg, "%d", &verbosity_level)) return -emsg(1);
        break;
      case 'q': i++; /* respondpipe, idx=7 */
      case 'Q': i++; /* querypipe, idx=6 */
      case 'l': i++; /* notify pipe, idx=5 */
      case 'f': i++; /* finalkeydir, idx=4 */
      case 'd': i++; /* rawkeydir, idx=3 */
      case 'r': i++; /* commreceivepipe, idx=2 */
      case 's': i++;    /* commsendpipe, idx=1 */
      case 'c': /* commandpipe, idx=0 */
        if (1 != sscanf(optarg, FNAMFORMAT, fname[i])) return -emsg(2 + i);
        fname[i][FNAMELENGTH - 1] = 0; /* security termination */
        break;
      case 'e': /* read in error threshold */
        if (1 != sscanf(optarg, "%f", &errormargin)) return -emsg(10);
        if ((errormargin < MIN_ERR_MARGIN) || (errormargin > MAX_ERR_MARGIN))
          return -emsg(11);
        break;
      case 'E': /* expected error rate */
        if (1 != sscanf(optarg, "%f", &initialerr)) return -emsg(12);
        if ((initialerr < MIN_INI_ERR) || (initialerr > MAX_INI_ERR))
          return -emsg(13);
        break;
      case 'k': /* kill mode for raw files */
        killmode = 1;
        break;
      case 'J': /* error rate generated outside eavesdropper */
        if (1 != sscanf(optarg, "%f", &intrinsicerr)) return -emsg(14);
        if ((intrinsicerr < 0) || (intrinsicerr > MAX_INTRINSIC))
          return -emsg(15);
        break;
      case 'T': /* runtime error behaviour */
        if (1 != sscanf(optarg, "%d", &runtimeerrormode)) return -emsg(16);
        if ((runtimeerrormode < 0) || (runtimeerrormode > MAXRUNTIMEERROR))
          return -emsg(16);
        break;
      case 'I': /* skip initial error measurement */
        ini_err_skipmode = 1;
        break;
      case 'i': /* expect a bell value for sneakage estimation */
        bellmode = 1;
        break;
      case 'p': /* disable privacy amplification */
        disable_privacyamplification = 1;
        break;
      case 'b': /* set BICONF rounds */
        if (1 != sscanf(optarg, "%d", &biconf_rounds)) return -emsg(76);
        if ((biconf_rounds <= 0) || (biconf_rounds > MAX_BICONF_ROUNDS))
          return -emsg(77);
        break;
      case 'B': /* take BER argument to determine biconf rounds */
        if (1 != sscanf(optarg, "%f", &biconf_BER)) return -emsg(78);
        if ((biconf_BER <= 0) || (biconf_BER > 1)) return -emsg(79);
        biconf_rounds = (int)(-log(biconf_BER / AVG_BINSEARCH_ERR) / log(2));
        if (biconf_rounds <= 0) biconf_rounds = 1; /* at least one */
        if (biconf_rounds > MAX_BICONF_ROUNDS) return -emsg(77);
        printf("biconf rounds used: %d\n", biconf_rounds);
        break;
    }
  }
  /* checking parameter cosistency */
  for (i = 0; i < 8; i++)
    if (fname[i][0] == 0)
      return -emsg(17); /* all files and pipes specified ? */

  /* open pipelines */
  if (stat(fname[handleId_commandPipe], &cmdstat)) return -emsg(18); /* command pipeline */
  if (!S_ISFIFO(cmdstat.st_mode)) return -emsg(19);
  if (!(fhandle[handleId_commandPipe] = fopen(fname[handleId_commandPipe], "r+"))) return -emsg(18);
  handle[handleId_commandPipe] = fileno(fhandle[handleId_commandPipe]);

  if (stat(fname[handleId_sendPipe], &cmdstat)) return -emsg(20); /* send pipeline */
  if (!S_ISFIFO(cmdstat.st_mode)) return -emsg(21);
  if ((handle[handleId_sendPipe] = open(fname[handleId_sendPipe], FIFOOUTMODE)) == -1) return -emsg(20);

  if (stat(fname[handleId_receivePipe], &cmdstat)) return -emsg(22); /* receive pipeline */
  if (!S_ISFIFO(cmdstat.st_mode)) return -emsg(23);
  if ((handle[handleId_receivePipe] = open(fname[handleId_receivePipe], FIFOINMODE)) == -1) return -emsg(22);

  if (!(fhandle[handleId_notifyPipe] = fopen(fname[handleId_notifyPipe], "w+")))
    return -emsg(24); /* notify pipeline */
  handle[handleId_notifyPipe] = fileno(fhandle[handleId_notifyPipe]);

  if (stat(fname[handleId_queryPipe], &cmdstat)) return -emsg(25); /* query pipeline */
  if (!S_ISFIFO(cmdstat.st_mode)) return -emsg(26);
  if (!(fhandle[handleId_queryPipe] = fopen(fname[handleId_queryPipe], "r+"))) return -emsg(25);
  handle[handleId_queryPipe] = fileno(fhandle[handleId_queryPipe]);

  if (!(fhandle[handleId_queryRespPipe] = fopen(fname[handleId_queryRespPipe], "w+")))
    return -emsg(27); /* query response pipe */
  handle[handleId_queryRespPipe] = fileno(fhandle[handleId_queryRespPipe]);

  /* find largest handle for select call */
  handle[handleId_rawKeyDir] = 0;
  handle[handleId_finalKeyDir] = 0;
  selectmax = 0;
  for (i = 0; i < 8; i++)
    if (selectmax < handle[i]) selectmax = handle[i];
  selectmax += 1;

  /* initializing buffers */
  next_packet_to_send = NULL; /* no packets to be sent */
  last_packet_to_send = NULL;
  send_index = 0;        /* index of next packet to send */
  receive_index = 0;     /* index for reading in a longer packet */
  blocklist = NULL;      /* no active key blocks in memory */
  rec_packetlist = NULL; /* no receive packet s in queue */

  /* main loop */
  noshutdown = 1; /* keep thing running */
  instring[0] = 0;
  ipt = 0; /* input parsing */
  do {
    /* prepare select call */
    FD_ZERO(&readqueue);
    FD_ZERO(&writequeue);
    FD_SET(handle[handleId_queryPipe], &readqueue); /* query pipe */
    FD_SET(handle[handleId_receivePipe], &readqueue); /* receive pipe */
    FD_SET(handle[handleId_commandPipe], &readqueue); /* command pipe */
    if (next_packet_to_send || send_index) {
      FD_SET(handle[handleId_sendPipe], &writequeue); /* content to send */
    }
    /* keep timeout short if there is work to do */
    timeout = ((instring[0] || rec_packetlist) ? TENMILLISEC : HALFSECOND);
    retval = select(selectmax, &readqueue, &writequeue, (fd_set *)0, &timeout);

    if (retval == -1) return -emsg(28);
    // If there was a pendeing request from select
    if (retval) {

      // If there is something to write to send pipe
      if (FD_ISSET(handle[handleId_sendPipe], &writequeue)) {
        #ifdef DEBUG
        printf("Writing packet to sendpipe\n");
        fflush(stdout);
        #endif
        i = next_packet_to_send->length - send_index;
        retval = write(handle[handleId_sendPipe], &next_packet_to_send->packet[send_index], i);
        if (retval == -1) return -emsg(29);
        if (retval == i) { /* packet is sent */
          free2(next_packet_to_send->packet);
          tmp_packetpointer = next_packet_to_send;
          next_packet_to_send = next_packet_to_send->next;
          if (last_packet_to_send == tmp_packetpointer)
            last_packet_to_send = NULL;
          free2(tmp_packetpointer); /* remove packet pointer */
          send_index = 0;           /* not digesting packet anymore */
        } else {
          send_index += retval;
        }
      }
      
      // If there is something to read from cmd pipeline
      if (FD_ISSET(handle[handleId_commandPipe], &readqueue)) {
        retval = read(handle[handleId_commandPipe], &instring[ipt], CMD_INBUFLEN - 1 - ipt);
        if (retval < 0) break;
        #ifdef DEBUG
        printf("Read something from cmd pipe\n");
        fflush(stdout);
        #endif
        ipt += retval;
        instring[ipt] = 0;
        if (ipt >= CMD_INBUFLEN) return -emsg(75); /* overflow */
                                                   /* parse later... */
      }

      /* parse input string */
      dpnt = index(instring, '\n');
      if (dpnt) { /* we got a newline */
        dpnt[0] = 0;
        sl = strlen(instring);
        retval2 = process_command(instring);
        if (retval2 && (runtimeerrormode == 0)) {
          return -emsg(retval2); /* complain */
        }
        /* move back rest */
        for (i = 0; i < ipt - sl - 1; i++) instring[i] = dpnt[i + 1];
        ipt -= sl + 1;
        instring[ipt] = 0; /* repair index */
      }

      // If there is something to read from receive pipeline
      if (FD_ISSET(handle[handleId_receivePipe], &readqueue)) {
        #ifdef DEBUG
        printf("Read something from rcv pipe\n");
        fflush(stdout);
        #endif
        if (receive_index < sizeof(struct ERRC_PROTO)) {
          retval = read(handle[handleId_receivePipe], &((char *)&msgprotobuf)[receive_index],
                        sizeof(msgprotobuf) - receive_index);
          if (retval == -1) return -emsg(36); /* can that be better? */
          receive_index += retval;
          if (receive_index == sizeof(msgprotobuf)) {
            /* prepare for new buffer */
            tmpreadbuf = (char *)malloc2(msgprotobuf.bytelength);
            if (!tmpreadbuf) return -emsg(37);
            /* transfer header */
            memcpy(tmpreadbuf, &msgprotobuf, sizeof(msgprotobuf));
          }
        } else { /* we are reading the main message now */
          retval = read(handle[handleId_receivePipe], &tmpreadbuf[receive_index],
                        msgprotobuf.bytelength - receive_index);
          if (retval == -1) return -emsg(36); /* can that be better? */
          receive_index += retval;
          if (receive_index == msgprotobuf.bytelength) { /* got all */
            msgp = (struct packet_received *)malloc2(
                sizeof(struct packet_received));
            if (!msgp) return -emsg(38);
            /* insert message in message chain */
            msgp->next = NULL;
            msgp->length = receive_index;
            msgp->packet = tmpreadbuf;
            sbfp = rec_packetlist;
            if (sbfp) {
              while (sbfp->next) sbfp = sbfp->next;
              sbfp->next = msgp;
            } else {
              rec_packetlist = msgp;
            }
            receive_index = 0; /* ready for next one */
          }
        }
      }

      // If there is something to read from query pipeline
      if (FD_ISSET(handle[handleId_queryPipe], &readqueue)) {
      }
    }
    /* enter working routines for packets here */
    if ((sbfp = rec_packetlist)) { /* got one message */
      receivebuf = sbfp->packet;   /* get pointer */
      if (((unsigned int *)receivebuf)[0] != ERRC_PROTO_tag) {
        return -emsg(44);
      }

      #ifdef DEBUG
      printf("received message, subtype: %d, len: %d\n",
             ((unsigned int *)receivebuf)[2],
             ((unsigned int *)receivebuf)[1]);
      fflush(stdout);
      #endif

      switch (((unsigned int *)receivebuf)[2]) { /* subtype */
        case 0: /* received an error estimation packet */
          retval = process_esti_message_0(receivebuf);
          if (retval) { /* an error occured */
            if (runtimeerrormode > 1) break;
            return -emsg(retval);
          }
          break;
        case 2: /* received request for more bits */
          retval = send_more_esti_bits(receivebuf);
          if (retval) { /* an error occured */
            if (runtimeerrormode > 1) break;
            return -emsg(retval);
          }

          break;
        case 3: /* reveived error confirmation message */
          retval = prepare_dualpass(receivebuf);
          if (retval) { /* an error occured */
            if (runtimeerrormode > 1) break;
            return -emsg(retval);
          }
          break;
        case 4: /* reveived parity list message */
          retval = start_binarysearch(receivebuf);
          if (retval) { /* an error occured */
            if (runtimeerrormode > 1) break;
            return -emsg(retval);
          }
          break;
        case 5: /* reveive a binarysearch message */
          retval = process_binarysearch(receivebuf);
          if (retval) { /* an error occured */
            if (runtimeerrormode > 1) break;
            return -emsg(retval);
          }

          break;
        case 6: /* receive a BICONF initiating request */
          retval = generate_biconfreply(receivebuf);
          if (retval) { /* an error occured */
            if (runtimeerrormode > 1) break;
            return -emsg(retval);
          }

          break;
        case 7: /* receive a BICONF parity response */
          retval = receive_biconfreply(receivebuf);
          if (retval) { /* an error occured */
            if (runtimeerrormode > 1) break;
            return -emsg(retval);
          }

          break;
        case 8: /* receive a privacy amplification start msg */
          retval = receive_privamp_msg(receivebuf);
          if (retval) { /* an error occured */
            if (runtimeerrormode > 1) break;
            return -emsg(retval);
          }
          break;

        default: /* packet subtype not known */
          fprintf(stderr, "received subtype %d; ",
                  ((unsigned int *)receivebuf)[2]);
          return -emsg(45);
      }
      /* printf("receive packet successfully digested\n");
         fflush(stdout); */
      /* remove this packet from the queue */
      rec_packetlist = sbfp->next; /* upate packet pointer */
      free2(receivebuf);           /* free data section... */
      free2(sbfp);                 /* ...and pointer entry */
    }

  } while (noshutdown);
  
  /* close nicely */
  fclose(fhandle[handleId_commandPipe]);
  close(handle[handleId_sendPipe]);
  close(handle[handleId_receivePipe]);
  fclose(fhandle[handleId_notifyPipe]);
  fclose(fhandle[handleId_queryPipe]);
  fclose(fhandle[handleId_queryRespPipe]);
  return 0;
}

#include "ecd2_qber_estim.h"

// ERROR ESTIMATION
/* ------------------------------------------------------------------------ */
/* function to provide the number of bits needed in the initial error
   estimation; eats the local error (estimated or guessed) as a float. Uses
   the maximum for either estimating the range for k0 with the desired error,
   or a sufficient separation from the no-error-left area. IS that fair?
   Anyway, returns a number of bits. */
int testbits_needed(float e) {
  float ldi;
  int bn;
  ldi = USELESS_ERRORBOUND - e; /* difference to useless bound */
  bn = MAX((int)(e * INI_EST_SIGMA / ldi / ldi + .99),
           (int)(1. / e / DESIRED_K0_ERROR / DESIRED_K0_ERROR));
  return bn;
}

/* function to initiate the error estimation procedure. parameter is
   statrepoch, return value is 0 on success or !=0 (w error encoding) on error.
 */
int errorest_1(unsigned int epoch) {
  struct keyblock *kb;        /* points to current keyblock */
  float f_inierr, f_di;       /* for error estimation */
  int bits_needed;            /* number of bits needed to send */
  struct ERRC_ERRDET_0 *msg1; /* for header to be sent */

  if (!(kb = get_thread(epoch))) return 73; /* cannot find key block */

  /* set role in block to alice (initiating the seed) in keybloc struct */
  kb->role = 0;
  /* seed the rng, (the state has to be kept with the thread, use a lock
     system for the rng in case several ) */
  kb->RNG_usage = 0; /* use simple RNG */
  if (!(kb->RNG_state = get_r_seed())) return 39;

  /*  evaluate how many bits are needed in this round */
  f_inierr = kb->initialerror / 65536.; /* float version */

  if (ini_err_skipmode) { /* don't do error estimation */
    kb->errormode = 1;
    msg1 = fillsamplemessage(kb, 1, kb->initialerror, kb->BellValue);
  } else {
    kb->errormode = 0;
    f_di = USELESS_ERRORBOUND - f_inierr;
    if (f_di <= 0) return 41; /* no error extractable */
    bits_needed = testbits_needed(f_inierr);

    if (bits_needed >= kb->initialbits) return 42; /* not possible */
    /* fill message with sample bits */
    msg1 = fillsamplemessage(kb, bits_needed, 0, kb->BellValue);
  }
  if (!msg1) return 43; /* a malloc error occured */

  /* send this structure to the other side */
  insert_sendpacket((char *)msg1, msg1->bytelength);

  /* go dormant again.  */
  return 0;
}

/* function to process the first error estimation packet. Argument is a pointer
   to the receivebuffer with both the header and the data section. Initiates
   the error estimation, and prepares the next  package for transmission.
   Currently, it assumes only PRNG-based bit selections.

   Return value is 0 on success, or an error message useful for emsg.

*/
int process_esti_message_0(char *receivebuf) {
  struct ERRC_ERRDET_0 *in_head; /* holds header */
  struct keyblock *kb;           /* poits to thread info */
  unsigned int *in_data;         /* holds input data bits */
  /* int retval; */
  int i, seen_errors, rn_order, bipo;
  unsigned int bpm;
  struct ERRC_ERRDET_2 *h2; /* for more requests */
  struct ERRC_ERRDET_3 *h3; /* reply message */
  int replymode;            /* 0: terminate, 1: more bits, 2: continue */
  float localerror, ldi;
  int newbitsneeded = 0; /* to keep compiler happy */
  int overlapreply;

  /* get convenient pointers */
  in_head = (struct ERRC_ERRDET_0 *)receivebuf;
  in_data = (unsigned int *)(&receivebuf[sizeof(struct ERRC_ERRDET_0)]);

  /* try to find overlap with existing files */
  overlapreply = check_epochoverlap(in_head->epoch, in_head->number_of_epochs);

  if (overlapreply && in_head->seed) return 46; /* conflict */
  if ((!overlapreply) && !(in_head->seed)) return 51;

  if (overlapreply) { /* we have an update message to request more bits */
    kb = get_thread(in_head->epoch);
    if (!kb) return 48; /* cannot find thread */
    kb->leakagebits += in_head->numberofbits;
    kb->estimatedsamplesize += in_head->numberofbits;
    seen_errors = kb->estimatederror;
  } else {
    /* create a thread with the loaded files, get thead handle */
    if ((i = create_thread(in_head->epoch, in_head->number_of_epochs, 0.0,
                           0.0))) {
      fprintf(stderr, "create_thread return code: %d epoch: %08x, number:%d\n",
              i, in_head->epoch, in_head->number_of_epochs);
      return 47; /* no success */
    }

    kb = get_thread(in_head->epoch);
    if (!kb) return 48; /* should not happen */

    /* update the thread with the type status, and with the info form the
       other side */
    kb->RNG_state = in_head->seed;
    kb->RNG_usage = 0; /* use PRNG sequence */
    kb->leakagebits = in_head->numberofbits;
    kb->role = 1; /* being bob */
    seen_errors = 0;
    kb->estimatedsamplesize = in_head->numberofbits;
    kb->BellValue = in_head->BellValue;
  }

  /* do the error estimation */
  rn_order = get_order_2(kb->initialbits);

  for (i = 0; i < (in_head->numberofbits); i++) {
    do { /* generate a bit position */
      bipo = PRNG_value2(rn_order, &kb->RNG_state);
      if (bipo > kb->initialbits) continue;          /* out of range */
      bpm = bt_mask(bipo);                           /* bit mask */
      if (kb->testmarker[bipo / 32] & bpm) continue; /* already used */
      /* got finally a bit */
      kb->testmarker[bipo / 32] |= bpm; /* mark as used */
      if (((kb->mainbuf[bipo / 32] & bpm) ? 1 : 0) ^
          ((in_data[i / 32] & bt_mask(i)) ? 1 : 0)) { /* error */
        seen_errors++;
      }
      break;
    } while (1);
  }

  /* save error status */
  kb->estimatederror = seen_errors;

  if (in_head->errormode) { /* skip the error estimation */
    kb->errormode = 1;
    localerror = (float)in_head->errormode / 65536.0;
    replymode = replyMode_continue; /* skip error est part */
  } else {
    kb->errormode = 0;
    /* make decision if to ask for more bits */
    localerror = (float)seen_errors / (float)(kb->estimatedsamplesize);

    ldi = USELESS_ERRORBOUND - localerror;
    if (ldi <= 0.) { /* ignore key bits : send error number to terminate */
      replymode = replyMode_terminate;
    } else {
      newbitsneeded = testbits_needed(localerror);
      if (newbitsneeded > kb->initialbits) { /* will never work */
        replymode = replyMode_terminate;
      } else {
        if (newbitsneeded > kb->estimatedsamplesize) { /*  more bits */
          replymode = replyMode_moreBits;
        } else { /* send confirmation message */
          replymode = replyMode_continue;
        }
      }
    }
  }

  #ifdef DEBUG
  printf("process_esti_message_0: estErr: %d errMode: %d \
 lclErr: %.4f estSampleSize: %d newBitsNeeded: %d initialBits: %d\n",
    kb->estimatederror, kb->errormode, 
    localerror, kb->estimatedsamplesize, newbitsneeded, kb->initialbits);
  #endif

  /* prepare reply message */
  switch (replymode) {
    case replyMode_terminate:
    case replyMode_continue: /* send message 3 */
      h3 = (struct ERRC_ERRDET_3 *)malloc2(sizeof(struct ERRC_ERRDET_3));
      if (!h3) return 43; /* cannot malloc */
      h3->tag = ERRC_PROTO_tag;
      h3->subtype = ERRC_ERRDET_3_subtype;
      h3->bytelength = sizeof(struct ERRC_ERRDET_3);
      h3->epoch = kb->startepoch;
      h3->number_of_epochs = kb->numberofepochs;
      h3->tested_bits = kb->leakagebits;
      h3->number_of_errors = seen_errors;
      insert_sendpacket((char *)h3, h3->bytelength); /* error trap? */
      break;
    case replyMode_moreBits: /* send message 2 */
      h2 = (struct ERRC_ERRDET_2 *)malloc2(sizeof(struct ERRC_ERRDET_2));
      h2->tag = ERRC_PROTO_tag;
      h2->subtype = ERRC_ERRDET_2_subtype;
      h2->bytelength = sizeof(struct ERRC_ERRDET_2);
      h2->epoch = kb->startepoch;
      h2->number_of_epochs = kb->numberofepochs;
      /* this is the important number */
      h2->requestedbits = newbitsneeded - kb->estimatedsamplesize;
      insert_sendpacket((char *)h2, h2->bytelength);
      break;
  }

  /* update thread */
  switch (replymode) {
    case replyMode_terminate: /* kill the thread due to excessive errors */
      #ifdef DEBUG
      printf("Kill the thread due to excessive errors\n");
      fflush(stdout);
      #endif
      remove_thread(kb->startepoch);
      break;
    case replyMode_moreBits: /* wait for more bits to come */
      kb->processingstate = PRS_GETMOREEST;
      break;
    case replyMode_continue: /* error estimation is done, proceed to next step */
      kb->processingstate = PRS_KNOWMYERROR;
      kb->estimatedsamplesize = kb->leakagebits; /* is this needed? */
      /****** more to do here *************/
      /* calculate k0 and k1 for further uses */
      if (localerror < 0.01444) {
        kb->k0 = 64; /* min bitnumber */
      } else {
        kb->k0 = (int)(0.92419642 / localerror);
      }
      kb->k1 = 3 * kb->k0; /* block length second array */
      break;
  }

  return 0; /* everything went well */
}

/* function to reply to a request for more estimation bits. Argument is a
   pointer to the receive buffer containing the message. Just sends over a
   bunch of more estimaton bits. Currently, it uses only the PRNG method.

   Return value is 0 on success, or an error message otherwise. */
int send_more_esti_bits(char *receivebuf) {
  struct ERRC_ERRDET_2 *in_head; /* holds header */
  struct keyblock *kb;           /* poits to thread info */
  int bitsneeded;                /* number of bits needed to send */
  struct ERRC_ERRDET_0 *msg1;    /* for header to be sent */

  /* get pointers for header...*/
  in_head = (struct ERRC_ERRDET_2 *)receivebuf;

  /* ...and find thread: */
  kb = get_thread(in_head->epoch);
  if (!kb) {
    fprintf(stderr, "epoch %08x: ", in_head->epoch);
    return 49;
  }
  /* extract relevant information from thread */
  bitsneeded = in_head->requestedbits;

  /* prepare a response message block / fill the response with sample bits */
  msg1 = fillsamplemessage(kb, bitsneeded, 0, kb->BellValue);
  if (!msg1) return 43; /* a malloc error occured */

  /* adjust message reply to hide the seed/indicate a second reply */
  msg1->seed = 0;
  /* send this structure to outgoing mailbox */
  insert_sendpacket((char *)msg1, msg1->bytelength);

  /* everything is fine */
  return 0;
}

/* function to proceed with the error estimation reply. Estimates if the
   block deserves to be considered further, and if so, prepares the permutation
   array of the key, and determines the parity functions of the first key.
   Return value is 0 on success, or an error message otherwise. */
int prepare_dualpass(char *receivebuf) {
  struct ERRC_ERRDET_3 *in_head; /* holds header */
  struct keyblock *kb;           /* poits to thread info */
  float localerror, ldi;
  int errormark, newbitsneeded;
  unsigned int newseed; /* seed for permutation */
  int msg4datalen;
  struct ERRC_ERRDET_4 *h4;    /* header pointer */
  unsigned int *h4_d0, *h4_d1; /* pointer to data tracks  */
  int retval;

  /* get pointers for header...*/
  in_head = (struct ERRC_ERRDET_3 *)receivebuf;

  /* ...and find thread: */
  kb = get_thread(in_head->epoch);
  if (!kb) {
    fprintf(stderr, "epoch %08x: ", in_head->epoch);
    return 49;
  }
  /* extract error information out of message */
  if (in_head->tested_bits != kb->leakagebits) return 52;
  kb->estimatedsamplesize = in_head->tested_bits;
  kb->estimatederror = in_head->number_of_errors;

  /* decide if to proceed */
  if (kb->errormode) {
    localerror = (float)kb->initialerror / 65536.;
  } else {
    localerror = (float)kb->estimatederror / (float)kb->estimatedsamplesize;
    ldi = USELESS_ERRORBOUND - localerror;
    errormark = 0;
    if (ldi <= 0.) {
      errormark = 1; /* will not work */
    } else {
      newbitsneeded = testbits_needed(localerror);
      if (newbitsneeded > kb->initialbits) { /* will never work */
        errormark = 1;
      }
    }
    #ifdef DEBUG
    printf("prepare_dualpass kb. estSampleSize: %d estErr: %d errMode: %d lclErr: %.4f \
 ldi: %.4f newBitsNeeded: %d initBits: %d errMark: %d\n",
      kb->estimatedsamplesize, kb->estimatederror, kb->errormode, 
      localerror, ldi, newbitsneeded, kb->initialbits, errormark);
    fflush(stdout);
    #endif
    if (errormark) { /* not worth going */
      remove_thread(kb->startepoch);
      return 0;
    }
  }

  /* determine process variables */
  kb->processingstate = PRS_KNOWMYERROR;

  kb->estimatedsamplesize = kb->leakagebits; /* is this needed? */

  /****** more to do here *************/
  /* calculate k0 and k1 for further uses */
  if (localerror < 0.01444) {
    kb->k0 = 64; /* min bitnumber */
  } else {
    kb->k0 = (int)(0.92419642 / localerror);
  }
  kb->k1 = 3 * kb->k0; /* block length second array */

  /* install new seed */
  kb->RNG_usage = 0; /* use simple RNG */
  if (!(newseed = get_r_seed())) return 39;
  kb->RNG_state = newseed; /* get new seed for RNG */

  /* prepare permutation array */
  prepare_permutation(kb);

  /* prepare message 5 frame - this should go into prepare_permutation? */
  kb->partitions0 = (kb->workbits + kb->k0 - 1) / kb->k0;
  kb->partitions1 = (kb->workbits + kb->k1 - 1) / kb->k1;

  /* get raw buffer */
  msg4datalen = ((kb->partitions0 + 31) / 32 + (kb->partitions1 + 31) / 32) * 4;
  h4 = (struct ERRC_ERRDET_4 *)malloc2(sizeof(struct ERRC_ERRDET_4) +
                                       msg4datalen);
  if (!h4) return 43; /* cannot malloc */
  /* both data arrays */
  h4_d0 = (unsigned int *)&h4[1];
  h4_d1 = &h4_d0[(kb->partitions0 + 31) / 32];
  h4->tag = ERRC_PROTO_tag;
  h4->bytelength = sizeof(struct ERRC_ERRDET_4) + msg4datalen;
  h4->subtype = ERRC_ERRDET_4_subtype;
  h4->epoch = kb->startepoch;
  h4->number_of_epochs = kb->numberofepochs; /* length of the block */
  h4->seed = newseed;                        /* permutator seed */

  /* these are optional; should we drop them? */
  h4->k0 = kb->k0;
  h4->k1 = kb->k1;
  h4->totalbits = kb->workbits;

  /* evaluate parity in blocks */
  prepare_paritylist1(kb, h4_d0, h4_d1);

  /* update status */
  kb->processingstate = PRS_PERFORMEDPARITY1;
  kb->leakagebits += kb->partitions0 + kb->partitions1;

  /* transmit message */
  retval = insert_sendpacket((char *)h4, h4->bytelength);
  if (retval) return retval;

  return 0; /* go dormant again... */
}
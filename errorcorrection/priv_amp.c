#include "priv_amp.h"

// PRIVACY AMPLIFICATION HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */

/**
 * @brief helper: eve's error knowledge
 * 
 * @param z 
 * @return float 
 */
float phi(float z) {
  return ((1 + z) * log(1 + z) + (1 - z) * log(1 - z)) / log(2.);
}

/**
 * @brief 
 * 
 * @param q 
 * @return float 
 */
float binentrop(float q) {
  return (-q * log(q) - (1 - q) * log(1 - q)) / log(2.);
}

// PRIVACY AMPLIFICATION MAIN FUNCTIONS
/* ------------------------------------------------------------------------- */

/**
 * @brief Function to initiate the privacy amplification.
 * 
 *  Sends out a message with
   a PRNG seed (message 8), and hand over to the core routine for the PA.
 * 
 * @param kb keyblock ptr
 * @return int 0 if success, otherwise error code
 */
int initiate_privacyamplification(struct keyblock *kb) {
  unsigned int seed;
  struct ERRC_ERRDET_8 *h8; /* head for trigger message */

  /* generate local RNG seed */
  seed = get_r_seed();

  /* prepare messagehead */
  h8 = (struct ERRC_ERRDET_8 *)malloc2(sizeof(struct ERRC_ERRDET_8));
  if (!h8) return 62; /* can't malloc */
  h8->tag = ERRC_PROTO_tag;
  h8->bytelength = sizeof(struct ERRC_ERRDET_8);
  h8->subtype = ERRC_ERRDET_8_subtype;
  h8->epoch = kb->startepoch;
  h8->number_of_epochs = kb->numberofepochs;
  h8->seed = seed;                /* significant content */
  h8->lostbits = kb->leakagebits; /* this is what we use for PA */
  h8->correctedbits = kb->correctederrors;

  /* insert message in msg pool */
  insert_sendpacket((char *)h8, h8->bytelength);

  /* do actual privacy amplification */
  return do_privacy_amplification(kb, seed, kb->leakagebits);
}

/**
 * @brief Function to process a privacy amplification message.
 * 
 * Parses the message and passes
   the real work over to the do_privacyamplification part
 * 
 * @param receivebuf incoming message
 * @return int 0 if success, otherwise error code
 */
int receive_privamp_msg(char *receivebuf) {
  struct ERRC_ERRDET_8 *in_head; /* holds header */
  struct keyblock *kb;           /* poits to thread info */

  /* get pointers for header...*/
  in_head = (struct ERRC_ERRDET_8 *)receivebuf;

  /* ...and find thread: */
  kb = get_thread(in_head->epoch);
  if (!kb) {
    fprintf(stderr, "epoch %08x: ", in_head->epoch);
    return 49;
  }

  /* retreive number of corrected bits */
  kb->correctederrors = in_head->correctedbits;

  /* do some consistency checks???*/

  /* pass to the core prog */
  return do_privacy_amplification(kb, in_head->seed, in_head->lostbits);
}

/**
 * @brief Do core part of the privacy amplification.
 * 
 * Calculates the compression ratio
   based on the lost bits, saves the final key and removes the thread from the
   list. 
 * 
 * @param kb keyblock ptr
 * @param seed 
 * @param lostbits 
 * @return int 0 if success, otherwise error code
 */
int do_privacy_amplification(struct keyblock *kb, unsigned int seed,
                             int lostbits) {
  int sneakloss;
  float trueerror, safe_error;
  // float cheeky_error;
  unsigned int *finalkey;  /* pointer to final key */
  unsigned int m;          /* addition register */
  int numwords, mlen;      /* number of words in final key / message length */
  struct header_7 *outmsg; /* keeps output message */
  int i, j;                /* counting indices */
  char ffnam[FNAMELENGTH + 10]; /* to store filename */
  int written, rv;              /* counts writeout bits, return value */
  int redundantloss; /* keeps track of redundancy in error correction */
  float BellHelper;

  /* determine final key size */
  /* redundancy in parity negotiation; we transmit the last bit which could#
     be deducked from tracking the whole parity information per block. For
     each detected error, there is one bit redundant, which is overcounted
     in the leakage */
  redundantloss = kb->correctederrors;

  /* This is the error rate found in the error correction process. It should
     be a fair representation of the errors on that block of raw key bits,
     but a safety margin on the error of the error should be added, e.g.
     in terms of multiples of the standard deviation assuming a poissonian
     distribution for errors to happen (not sure why this is a careless
     assumption in the first place either. */
  trueerror = (float)kb->correctederrors / (float)kb->workbits;

  /* This 'intrisic error' thing is very dodgy, it should not be used at all
     unless you know what Eve is doing (which by definition you don't).
     Therefore, it s functionality is mostly commented out, only the
     basic query remains. The idea is based on the hope ventilated
     at some point that there is a basic error (of the kind of a detector
     dark count rate) which does lead to any information
     loss to the eavesdropper. Relies on lack of imagination how an
     eavesdropper can influence this basic error rather than on fundamental
     laws. Since this is dirty, we might as well assume that such an error
     is UNCORRELATED to errors caused by potential eavesdropping, and
     assume they add quadratically. Let's at least check that the true
     error is not smaller than the "basic" error..... */
  if (arguments.intrinsicerr < trueerror) {
    // Unused code
    // cheeky_error = sqrt(trueerror * trueerror - arguments.intrinsicerr * arguments.intrinsicerr);

    /* Dodgy intrinsic error subtraction would happen here. */

    /* We now evaluate the knowledge of an eavesdropper on the initial raw
       key for a given error rate, excluding the communication on error
       correction */
    if (!arguments.bellmode) { /* do single-photon source bound */

      if (kb->correctederrors > 0) {
        safe_error = trueerror * (1. + arguments.errormargin / sqrt(kb->correctederrors));
      } else {
        safe_error = trueerror;
      }
      sneakloss = (int)(binentrop(safe_error) * kb->workbits);

      /* old version of the loss to eve:
         sneakloss =
        (int)(phi(2*sqrt(trueerror*(1-trueerror)))/2.*kb->workbits);
      */
    } else { /* we do the device-indepenent estimation */
      BellHelper = kb->BellValue * kb->BellValue / 4. - 1.;
      if (BellHelper < 0.) { /* we have no key...*/
        sneakloss = kb->workbits;
      } else { /* there is hope... */
        sneakloss =
            (int)(kb->workbits * binentrop((1. + sqrt(BellHelper)) / 2.));
      }
    }
  } else {
    sneakloss = 0; /* Wruaghhh - how dirty... */
  }

  /* here we do the accounting of gained and lost bits */
  kb->finalkeybits =
      kb->workbits - (kb->leakagebits + sneakloss) + redundantloss;
  if (kb->finalkeybits < 0) kb->finalkeybits = 0; /* no hope. */

  /* dirtwork for testing. I need to leave this in because it is the basis
   for may of the plots we have. */
  printf("PA disable: %d\n", arguments.disable_privacyamplification);

  if (arguments.disable_privacyamplification) {
    kb->finalkeybits = kb->workbits;
  }

  printf("before privacy amp:\n corrected errors: %d\n workbits: %d\n",
         kb->correctederrors, kb->workbits);
  printf(" trueerror: %f\n sneakloss: %d\n leakagebits: %d\n", trueerror,
         sneakloss, kb->leakagebits - redundantloss);
  printf(" finakeybits: %d\n", kb->finalkeybits);
  #ifdef DEBUG
  fflush(stdout);
  #endif

  /* initiate seed */
  kb->RNG_state = seed;

  /* set last bits to zero in workbits.... */
  numwords = (kb->workbits + 31) / 32;
  if (kb->workbits & 31)
    kb->mainbuf[numwords - 1] &= (0xffffffff << (32 - (kb->workbits & 31)));

  /* prepare structure for final key */
  mlen = sizeof(struct header_7) + ((kb->finalkeybits + 31) / 32) * 4;
  outmsg = (struct header_7 *)malloc2(mlen);
  if (!outmsg) return 63;
  outmsg->tag = TYPE_7_TAG; /* final key file */
  outmsg->epoc = kb->startepoch;
  outmsg->numberofepochs = kb->numberofepochs;
  outmsg->numberofbits = kb->finalkeybits;

  finalkey = (unsigned int *)&outmsg[1]; /* here starts data area */

  /* clear target buffer */
  bzero(finalkey, (kb->finalkeybits + 31) / 32 * 4);

  /* prepare final key */
  if (arguments.disable_privacyamplification) { /* no PA fo debugging */
    for (j = 0; j < numwords; j++) finalkey[j] = kb->mainbuf[j];
  } else { /* do privacy amplification */
    /* create compression matrix on the fly while preparing key */
    for (i = 0; i < kb->finalkeybits; i++) { /* go through all targetbits */
      m = 0;                                 /* initial word */
      for (j = 0; j < numwords; j++)
        m ^= (kb->mainbuf[j] & PRNG_value2_32(&kb->RNG_state));
      if (parity(m)) finalkey[i / 32] |= bt_mask(i);
    }
  }

  /* send final key to file */
  strncpy(ffnam, arguments.fname[handleId_finalKeyDir], FNAMELENGTH);                /* fnal key directory */
  atohex(&ffnam[strlen(ffnam)], kb->startepoch);        /* add file name */
  arguments.handle[handleId_finalKeyDir] = open(ffnam, FILEOUTMODE, OUTPERMISSIONS); /* open target */
  if (-1 == arguments.handle[handleId_finalKeyDir]) return 64;
  written = 0;
  while (1) {
    rv = write(arguments.handle[handleId_finalKeyDir], &((char *)outmsg)[written], mlen - written);
    if (rv == -1) return 65; /* write error happened */
    written += rv;
    if (written >= mlen) break;
    usleep(100000); /* sleep 100 msec */
  }
  close(arguments.handle[handleId_finalKeyDir]);

  /* send notification */
  switch (arguments.verbosity_level) {
    case VERBOSITY_EPOCH: /* output raw block name */
      fprintf(arguments.fhandle[handleId_notifyPipe], "%08x\n", kb->startepoch);
      break;
    case VERBOSITY_EPOCH_FIN: /* block name and final bits */
      fprintf(arguments.fhandle[handleId_notifyPipe], "%08x %d\n", kb->startepoch, kb->finalkeybits);
      break;
    case VERBOSITY_EPOCH_INI_FIN_ERR: /* block name, ini bits, final bits, error rate */
      fprintf(arguments.fhandle[handleId_notifyPipe], "%08x %d %d %.4f\n", kb->startepoch, kb->initialbits,
              kb->finalkeybits, trueerror);
      break;
    case VERBOSITY_EPOCH_INI_FIN_ERR_PLAIN: /* same as with 2 but with text */
      fprintf(arguments.fhandle[handleId_notifyPipe],
              "startepoch: %08x initial bit number: %d final bit number: %d "
              "error rate: %.4f\n",
              kb->startepoch, kb->initialbits, kb->finalkeybits, trueerror);
      break;
    case VERBOSITY_EPOCH_INI_FIN_ERR_EXPLICIT: /* block name, ini bits, final bits, error rate, leak bits */
      fprintf(arguments.fhandle[handleId_notifyPipe], "%08x %d %d %.4f %d\n", kb->startepoch,
              kb->initialbits, kb->finalkeybits, trueerror, kb->leakagebits);
      break;
    case VERBOSITY_EPOCH_INI_FIN_ERR_EXPLICIT_WITH_COMMENTS: /* same as with 4 but with text */
      fprintf(arguments.fhandle[handleId_notifyPipe],
              "startepoch: %08x initial bit number: %d final bit number: %d "
              "error rate: %.4f leaked bits in EC: %d\n",
              kb->startepoch, kb->initialbits, kb->finalkeybits, trueerror,
              kb->leakagebits);
      break;
  }

  #ifdef DEBUG
  printf("startepoch: %08x initial bit number: %d final bit number: %d error rate: %.4f leaked bits in EC: %d\n",
              kb->startepoch, kb->initialbits, kb->finalkeybits, trueerror, kb->leakagebits);
  fflush(stdout);
  #endif

  fflush(arguments.fhandle[handleId_notifyPipe]);
  /* cleanup outmessage buf */
  free2(outmsg);

  /* destroy thread */
  printf("remove thread\n");
  fflush(stdout);
  return remove_thread(kb->startepoch);

  /* return benignly */
  return 0;
}
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
 * @param pb processblock ptr
 * @return int 0 if success, otherwise error code
 */
int initiate_privacyamplification(ProcessBlock *pb) {
  EcPktHdr_StartPrivAmp *h8; /* head for trigger message */
  int errorCode = 0;

  /* prepare messagehead */
  errorCode = comms_createEcHeader((char **)&h8, SUBTYPE_START_PRIV_AMP, 0, pb);
  if (errorCode) return 62; // hardcoded in original impl.
  h8->seed = generateRngSeed();   /* generate local RNG seed */
  h8->lostbits = pb->leakageBits; /* this is what we use for PA */
  h8->correctedbits = pb->correctedErrors;

  /* insert message in msg pool */
  comms_insertSendPacket((char *)h8, h8->base.totalLengthInBytes);

  /* do actual privacy amplification */
  return do_privacy_amplification(pb, h8->seed, pb->leakageBits);
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
int receive_privamp_msg(ProcessBlock *pb, char *receivebuf) {
  // Get header
  EcPktHdr_StartPrivAmp *in_head = (EcPktHdr_StartPrivAmp *)receivebuf;

  /* retrieve number of corrected bits */
  pb->correctedErrors = in_head->correctedbits;

  /* do some consistency checks???*/

  /* pass to the core prog */
  return do_privacy_amplification(pb, in_head->seed, in_head->lostbits);
}

/**
 * @brief Do core part of the privacy amplification.
 * 
 * Calculates the compression ratio
   based on the lost bits, saves the final key and removes the processblock from the
   list. 
 * 
 * @param pb processblock ptr
 * @param seed 
 * @param lostbits 
 * @return int 0 if success, otherwise error code
 */
int do_privacy_amplification(ProcessBlock *pb, unsigned int seed, int lostbits) {
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
  redundantloss = pb->correctedErrors;

  /* This is the error rate found in the error correction process. It should
     be a fair representation of the errors on that block of raw key bits,
     but a safety margin on the error of the error should be added, e.g.
     in terms of multiples of the standard deviation assuming a poissonian
     distribution for errors to happen (not sure why this is a careless
     assumption in the first place either. */
  trueerror = (float)pb->correctedErrors / (float)pb->workbits;

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

      safe_error = trueerror;
      if (pb->correctedErrors > 0) {
        safe_error *= (1. + arguments.errormargin / sqrt(pb->correctedErrors));
      }
      sneakloss = (int)(binentrop(safe_error) * pb->workbits);

      /* old version of the loss to eve:
         sneakloss =
        (int)(phi(2*sqrt(trueerror*(1-trueerror)))/2.*pb->workbits);
      */
    } else { /* we do the device-indepenent estimation */
      BellHelper = pb->bellValue * pb->bellValue / 4. - 1.;
      if (BellHelper < 0.) { /* we have no key...*/
        sneakloss = pb->workbits;
      } else { /* there is hope... */
        sneakloss = (int)(pb->workbits * binentrop((1. + sqrt(BellHelper)) / 2.));
      }
    }
  } else {
    sneakloss = 0; /* Wruaghhh - how dirty... */
  }

  /* here we do the accounting of gained and lost bits */
  pb->finalKeyBits =
      pb->workbits - (pb->leakageBits + sneakloss) + redundantloss;
  if (pb->finalKeyBits < 0) pb->finalKeyBits = 0; /* no hope. */

  /* dirtwork for testing. I need to leave this in because it is the basis
   for may of the plots we have. */
  printf("PA disable: %d\n", arguments.disable_privacyamplification);

  if (arguments.disable_privacyamplification) {
    pb->finalKeyBits = pb->workbits;
  }

  printf("before privacy amp:\n corrected errors: %d\n workbits: %d\n",
         pb->correctedErrors, pb->workbits);
  printf(" trueerror: %f\n sneakloss: %d\n leakageBits: %d\n", trueerror,
         sneakloss, pb->leakageBits - redundantloss);
  printf(" finakeybits: %d\n", pb->finalKeyBits);
  #ifdef DEBUG
  fflush(stdout);
  #endif

  /* initiate seed */
  pb->rngState = seed;

  /* set last bits to zero in workbits.... */
  numwords = wordIndex(pb->workbits + 31);
  if (pb->workbits & 31)
    pb->mainBufPtr[numwords - 1] &= (0xffffffff << (32 - (pb->workbits & 31)));

  /* prepare structure for final key */
  mlen = sizeof(struct header_7) + wordIndex(pb->finalKeyBits + 31) * 4;
  outmsg = (struct header_7 *)malloc2(mlen);
  if (!outmsg) return 63;
  outmsg->tag = TYPE_7_TAG; /* final key file */
  outmsg->epoc = pb->startEpoch;
  outmsg->numberOfEpochs = pb->numberOfEpochs;
  outmsg->numberofbits = pb->finalKeyBits;

  finalkey = (unsigned int *)&outmsg[1]; /* here starts data area */

  /* clear target buffer */
  bzero(finalkey, wordIndex(pb->finalKeyBits + 31) * 4);

  /* prepare final key */
  if (arguments.disable_privacyamplification) { /* no PA fo debugging */
    for (j = 0; j < numwords; j++) finalkey[j] = pb->mainBufPtr[j];
  } else { /* do privacy amplification */
    /* create compression matrix on the fly while preparing key */
    for (i = 0; i < pb->finalKeyBits; i++) { /* go through all targetbits */
      m = 0;                                 /* initial word */
      for (j = 0; j < numwords; j++)
        m ^= (pb->mainBufPtr[j] & PRNG_value2_32(&pb->rngState));
      if (parity(m)) finalkey[wordIndex(i)] |= bt_mask(i);
    }
  }

  /* send final key to file */
  strncpy(ffnam, arguments.fname[handleId_finalKeyDir], FNAMELENGTH);                /* fnal key directory */
  atohex(&ffnam[strlen(ffnam)], pb->startEpoch);        /* add file name */
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
      fprintf(arguments.fhandle[handleId_notifyPipe], "%08x\n", pb->startEpoch);
      break;
    case VERBOSITY_EPOCH_FIN: /* block name and final bits */
      fprintf(arguments.fhandle[handleId_notifyPipe], "%08x %d\n", pb->startEpoch, pb->finalKeyBits);
      break;
    case VERBOSITY_EPOCH_INI_FIN_ERR: /* block name, ini bits, final bits, error rate */
      fprintf(arguments.fhandle[handleId_notifyPipe], "%08x %d %d %.4f\n", pb->startEpoch, pb->initialBits,
              pb->finalKeyBits, trueerror);
      break;
    case VERBOSITY_EPOCH_INI_FIN_ERR_PLAIN: /* same as with 2 but with text */
      fprintf(arguments.fhandle[handleId_notifyPipe],
              "startEpoch: %08x initial bit number: %d final bit number: %d "
              "error rate: %.4f\n",
              pb->startEpoch, pb->initialBits, pb->finalKeyBits, trueerror);
      break;
    case VERBOSITY_EPOCH_INI_FIN_ERR_EXPLICIT: /* block name, ini bits, final bits, error rate, leak bits */
      fprintf(arguments.fhandle[handleId_notifyPipe], "%08x %d %d %.4f %d\n", pb->startEpoch,
              pb->initialBits, pb->finalKeyBits, trueerror, pb->leakageBits);
      break;
    case VERBOSITY_EPOCH_INI_FIN_ERR_EXPLICIT_WITH_COMMENTS: /* same as with 4 but with text */
      fprintf(arguments.fhandle[handleId_notifyPipe],
              "startEpoch: %08x initial bit number: %d final bit number: %d "
              "error rate: %.4f leaked bits in EC: %d\n",
              pb->startEpoch, pb->initialBits, pb->finalKeyBits, trueerror,
              pb->leakageBits);
      break;
  }

  #ifdef DEBUG
  printf("startEpoch: %08x initial bit number: %d final bit number: %d error rate: %.4f leaked bits in EC: %d\n",
              pb->startEpoch, pb->initialBits, pb->finalKeyBits, trueerror, pb->leakageBits);
  fflush(stdout);
  #endif

  fflush(arguments.fhandle[handleId_notifyPipe]);
  /* cleanup outmessage buf */
  free2(outmsg);

  /* destroy processblock */
  printf("remove processblock\n");
  fflush(stdout);
  return remove_processblock(pb->startEpoch);

  /* return benignly */
  return 0;
}
#include "ecd2_helper.h"

// HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */
// GENERAL HELPER FUNCTIONS

/* helper to obtain the smallest power of two to carry a number a */
int get_order(int a) {
  unsigned int order = 0xffffffff;
  while ((order & a) == a) order >>= 1;
  return (order << 1) + 1;
}
/* get the number of bits necessary to carry a number x ; result is e.g. 3 for parameter 8, 5 for parameter 17 etc. */
int get_order_2(int x) {
  int x2;
  int retval = 0;
  for (x2 = x - 1; x2; x2 >>= 1) retval++;
  return retval;
}

/* helper: count the number of set bits in a longint */
int count_set_bits(unsigned int a) {
  int c = 0;
  unsigned int i;
  for (i = 1; i; i <<= 1)
    if (i & a) c++;
  return c;
}

/* helper for name. adds a slash, hex file name and a terminal 0 */
char hexdigits[] = "0123456789abcdef";
void atohex(char *target, unsigned int v) {
  int i;
  target[0] = '/';
  for (i = 1; i < 9; i++) target[i] = hexdigits[(v >> (32 - i * 4)) & 15];
  target[9] = 0;
}

/* helper: eve's error knowledge */
float phi(float z) {
  return ((1 + z) * log(1 + z) + (1 - z) * log(1 - z)) / log(2.);
}
float binentrop(float q) {
  return (-q * log(q) - (1 - q) * log(1 - q)) / log(2.);
}

/* helper function to get a seed from the random device; returns seed or 0
   on error */
unsigned int get_r_seed(void) {
  int rndhandle; /* keep device handle for random device */
  unsigned int reply;

  rndhandle = open(RANDOMGENERATOR, O_RDONLY);
  if (-1 == rndhandle) {
    fprintf(stderr, "errno: %d", errno);
    return 39;
  }
  if (sizeof(unsigned int) != read(rndhandle, &reply, sizeof(unsigned int))) {
    return 0; /* not enough */
  }
  close(rndhandle);
  return reply;
}

/* helper function to compress key down in a sinlge sequence to eliminate the
   revealeld bits. updates workbits accordingly, and reduces number of
   revealed bits in the leakage_bits_counter  */
void cleanup_revealed_bits(struct keyblock *kb) {
  int lastbit = kb->initialbits - 1;
  unsigned int *d = kb->mainbuf;    /* data buffer */
  unsigned int *m = kb->testmarker; /* index for spent bits */
  unsigned int bm;                  /* temp storage of bitmask */
  int i;

  /* find first nonused lastbit */
  while ((lastbit > 0) && (m[lastbit / 32] & bt_mask(lastbit))) lastbit--;

  /* replace spent bits in beginning by untouched bits at end */
  for (i = 0; i <= lastbit; i++) {
    bm = bt_mask(i);
    if (m[i / 32] & bm) { /* this bit is revealed */
      d[i / 32] =
          (d[i / 32] & ~bm) |
          ((d[lastbit / 32] & bt_mask(lastbit)) ? bm : 0); /* transfer bit */
      /* get new lastbit */
      lastbit--;
      while ((lastbit > 0) && (m[lastbit / 32] & bt_mask(lastbit))) lastbit--;
    }
  }
  /* i should now contain the number of good bits */
  kb->workbits = i;

  /* fill rest of buffer with zeros for not loosing any bits */
  d[i / 32] &= ((i & 31) ? (0xffffffff << (32 - (i & 31))) : 0);
  for (i = ((kb->workbits / 32) + 1); i < (kb->initialbits + 31) / 32; i++) {
    d[i] = 0;
    /* printf("   i= %d\n",i); */
  }

  /* update number of lost bits */
  kb->leakagebits = 0;

  return;
}
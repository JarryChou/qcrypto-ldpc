#ifndef PKT_HEADER_7
#define PKT_HEADER_7

#define TYPE_7_TAG 7
#define TYPE_7_TAG_U 0x107

typedef struct header_7 { /* header for type-7 stream packet */
  int tag;
  unsigned int epoc;
  unsigned int numberOfEpochs;
  int numberofbits;
} h7;

#endif
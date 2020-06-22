#ifndef PKT_HEADER_3
#define PKT_HEADER_3

#define TYPE_3_TAG 3
#define TYPE_3_TAG_U 0x103

typedef struct header_3 { /* header for type-3 stream packet */
  int tag;
  unsigned int epoc;
  unsigned int length;
  int bitsperentry;
} h3;

#endif
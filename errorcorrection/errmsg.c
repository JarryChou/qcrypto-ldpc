#include "errmsg.h"

//  ERROR MANAGEMENT
int emsg(int code) {
  fprintf(stderr, "%s\n", errormessage[code]);
  return code;
}
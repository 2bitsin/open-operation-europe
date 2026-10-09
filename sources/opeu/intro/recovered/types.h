#ifndef OPEU_TYPES_H
#define OPEU_TYPES_H

/* DOS-compatible filename; C 6.0a has 16-bit int, 32-bit long, no stdint.h. */
#if defined(_MSC_VER) && _MSC_VER < 1000
typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long uint32_t;
typedef signed int int16_t;
typedef signed long int32_t;
#else
#include <stdint.h>
#endif

#endif

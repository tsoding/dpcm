#ifndef DPCM_H_
#define DPCM_H_

#include <stdint.h>

typedef struct {
    int32_t *items;
    size_t count;
    size_t capacity;
} Samples_S32;

#define BBG(fmt, expr) printf(#expr " = " fmt "\n", (expr))

#endif // DPCM_H_

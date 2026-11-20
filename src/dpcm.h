#ifndef DPCM_H_
#define DPCM_H_

#include <stdint.h>

typedef struct {
    int32_t *items;
    size_t count;
    size_t capacity;
} Samples_S32;

// Baba Gee macro
#define BBG(fmt, expr) printf(#expr " = " fmt "\n", (expr))

#define DPCM_MAGIC 0x4d435044
#define DPCM_VERSION 1
#define DPCM_CHANNELS 1

typedef uint32_t DPCM_Format;
enum {
    FORMAT_DPCM32,
    FORMAT_DPCM1,
};

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t sample_rate;
    DPCM_Format format;
    uint64_t sample_count;
} DPCM_File;

#endif // DPCM_H_

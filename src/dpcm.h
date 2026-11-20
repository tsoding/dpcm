#ifndef DPCM_H_
#define DPCM_H_

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int32_t *items;
    size_t count;
    size_t capacity;
} Samples_S32;

typedef struct {
    uint64_t *items;
    size_t count;
    size_t bit_count;
    size_t capacity;
} Samples_Packed_U1;

void packed_u1_append(Samples_Packed_U1 *s, uint64_t bit);

// Baba Gee macro
#define BBG(fmt, expr) printf(#expr " = " fmt "\n", (expr))

#define DPCM_MAGIC 0x4d435044
#define DPCM_VERSION 2
#define DPCM_CHANNELS 1
#define DPCM1_AMP_QUAT 100

typedef uint32_t DPCM_Format;
enum {
    FORMAT_DPCM32,
    FORMAT_DPCM1,
    __DPCM_FORMAT_COUNT,
};

static_assert(__DPCM_FORMAT_COUNT == 2, "Amount of DPCM formats have changed");
const char *dpcm_format_names[__DPCM_FORMAT_COUNT] = {
    [FORMAT_DPCM32] = "dpcm32",
    [FORMAT_DPCM1]  = "dpcm1",
};

bool get_dpcm_format_by_name(const char *name, DPCM_Format *format);

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t sample_rate;
    DPCM_Format format;
    uint64_t sample_count;
    uint16_t dpcm1_bits_per_sample;
    uint16_t dpcm1_amp_step_in_percents;
    uint64_t dpcm1_bit_count;
} DPCM_File;

typedef struct {
    int32_t *samples;
    size_t count;
    size_t cursor;
    int32_t previous;
} DPCM32_Player;

int32_t dpcm32_next_sample(void *p);

typedef struct {
    uint64_t *samples;
    size_t count;
    size_t bit_count;
    size_t bits_per_sample;
    size_t amp_step_in_percents;
    size_t bit_cursor;
    int32_t needle;
} DPCM1_Player;

int32_t dpcm1_next_sample(void *p);

#endif // DPCM_H_

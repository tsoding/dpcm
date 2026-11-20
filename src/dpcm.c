#include <string.h>
#include "nob.h"
#include "dpcm.h"

bool get_dpcm_format_by_name(const char *name, DPCM_Format *format)
{
    for (DPCM_Format i = 0; i < __DPCM_FORMAT_COUNT; ++i) {
        if (strcmp(dpcm_format_names[i], name) == 0) {
            *format = i;
            return true;
        }
    }
    return false;
}

void packed_u1_append(Samples_Packed_U1 *s, uint64_t bit)
{
    assert(s->bit_count >= s->count);
    size_t size_of_item_in_bits = sizeof(*s->items)*8;
    size_t bits_remainder = s->bit_count%size_of_item_in_bits;

    if (bits_remainder == 0) {
        da_append(s, 0);
    }

    da_last(s) |= ((bit&1)<<bits_remainder);

    s->bit_count += 1;
}

int32_t dpcm32_next_sample(void *p_)
{
    DPCM32_Player *p = p_;
    return p->cursor < p->count ? p->previous += p->samples[p->cursor++] : 0;
}

int32_t dpcm1_next_sample(void *p_)
{
    DPCM1_Player *p = p_;
    assert(p->bit_count >= p->count);
    size_t size_of_item_in_bits = sizeof(*p->samples)*8;
    int32_t step_up_quant   = INT_MAX/DPCM1_AMP_QUAT;
    int32_t step_down_quant = INT_MIN/DPCM1_AMP_QUAT;
    for (size_t j = 0; j < p->bits_per_sample && p->bit_cursor < p->bit_count; ++j, ++p->bit_cursor) {
        size_t bits_remainder = p->bit_cursor%size_of_item_in_bits;
        size_t index          = p->bit_cursor/size_of_item_in_bits;
        if ((p->samples[index]>>bits_remainder)&1) {
            p->needle += step_up_quant*p->amp_step_in_percents;
        } else {
            p->needle += step_down_quant*p->amp_step_in_percents;
        }
    }
    return p->needle;
}

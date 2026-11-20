#include <stdio.h>
#include "dpcm.h"

int main(void)
{
    Samples_Packed_U1 s = {0};
    for (size_t i = 0; i < 69; ++i) {
        packed_u1_append(&s, i&1);
    }
    return 0;
}

#include "dpcm.c"

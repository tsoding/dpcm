#define NOB_IMPLEMENTATION
#include "nob.h"

#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

#include "dpcm.h"

void usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s <input.flac> <output.dpcm32>\n", program_name);
}

int main(int argc, char **argv)
{
    const char *program_name = shift(argv, argc);

    if (argc <= 0) {
        usage(program_name);
        fprintf(stderr, "ERROR: no input is provided\n");
        return 1;
    }
    const char *input_file_path = shift(argv, argc);

    if (argc <= 0) {
        usage(program_name);
        fprintf(stderr, "ERROR: no output is provided\n");
        return 1;
    }
    const char *output_file_path = shift(argv, argc);

    drflac* pFlac = drflac_open_file(input_file_path, NULL);
    if (!pFlac) {
        fprintf(stderr, "ERROR: could not open \"%s\"\n", input_file_path);
        return 1;
    }

    size_t samples_size_in_bytes = pFlac->totalPCMFrameCount*pFlac->channels*sizeof(drflac_int32);
    drflac_int32* pSamples = malloc(samples_size_in_bytes);
    drflac_uint64 numberOfInterleavedSamplesActuallyRead = drflac_read_pcm_frames_s32(pFlac, pFlac->totalPCMFrameCount, pSamples);

    BBG("%llu", pFlac->totalPCMFrameCount);
    BBG("%d", pFlac->channels);
    BBG("%d", pFlac->bitsPerSample);
    BBG("%d", pFlac->sampleRate);
    BBG("%llu", numberOfInterleavedSamplesActuallyRead);
    BBG("%p", pSamples);

    Samples_S32 dpcm = {0};
    drflac_int32 previous = 0;
    da_append(&dpcm, previous);
    for (size_t i = 0; i < pFlac->totalPCMFrameCount; ++i) {
        drflac_int32 current = pSamples[i*pFlac->channels];
        da_append(&dpcm, current - previous);
        previous = current;
    }

    if (!write_entire_file(output_file_path, dpcm.items, sizeof(*dpcm.items)*dpcm.count)) {
        return 1;
    }
    printf("Generated \"%s\"\n", output_file_path);

    return 0;
}

#include <stdio.h>
#include <inttypes.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

#define CHANNELS 1
#define SAMPLE_RATE 44100

typedef struct {
    int32_t *samples;
    size_t count;
    size_t cursor;
    int32_t previous;
} DPCM32_Player;

int32_t dpcm32_next_sample(DPCM32_Player *p)
{
    return p->cursor < p->count ? p->previous += p->samples[p->cursor++] : 0;
}

void playback_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    UNUSED(pInput);
    DPCM32_Player *player = pDevice->pUserData;
    assert(pDevice->playback.channels == CHANNELS);
    ma_int32 *output = pOutput;
    for (ma_uint32 i = 0; i < frameCount; ++i) {
        output[i] = dpcm32_next_sample(player);
    }
}

void usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s <input.dpcm32>\n", program_name);
}

int main(int argc, char **argv)
{
    const char *program_name = shift(argv, argc);

    if (argc <= 0) {
        usage(program_name);
        printf("ERROR: no input is provided\n");
        return 1;
    }

    const char *input_file_path = shift(argv, argc);
    String_Builder sb = {0};
    if (!read_entire_file(input_file_path, &sb)) return 1;

    assert(sb.count%sizeof(int32_t) == 0);

    DPCM32_Player player = {
        .samples = (void*)sb.items,
        .count   = sb.count/sizeof(int32_t),
    };

    ma_device_config config  = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_s32; // Set to ma_format_unknown to use the device's native format.
    config.playback.channels = CHANNELS;      // Set to 0 to use the device's native channel count.
    config.sampleRate        = SAMPLE_RATE;   // Set to 0 to use the device's native sample rate.
    config.pUserData         = &player;
    config.dataCallback      = playback_callback;

    ma_device playback_device;
    if (ma_device_init(NULL, &config, &playback_device) != MA_SUCCESS) {
        fprintf(stderr, "ERROR: failed to initialize mah playback device\n");
        return -1;  // Failed to initialize the device.
    }

    ma_device_start(&playback_device);

    while (true) {
        usleep(16*1000); // sedate the event loop
    }

    printf("OK\n");

    return 0;
}

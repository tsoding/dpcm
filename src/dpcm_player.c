#include <stdio.h>
#include <inttypes.h>

#include "miniaudio.h"
#include "nob.h"

#include "dpcm.h"

typedef struct {
    void *p;
    int32_t (*next_sample)(void *p);
} Abstract_DPCM_Player;

void playback_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    UNUSED(pInput);
    Abstract_DPCM_Player *player = pDevice->pUserData;
    assert(pDevice->playback.channels == DPCM_CHANNELS);
    ma_int32 *output = pOutput;
    for (ma_uint32 i = 0; i < frameCount; ++i) {
        output[i] = player->next_sample(player->p);
    }
}

void usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s <input.dpcm>\n", program_name);
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

    DPCM_File *meta = (void*)sb.items;
    if (sb.count < sizeof(DPCM_File)) {
        fprintf(stderr, "ERROR: %s: File is too small.\n", input_file_path);
        return 1;
    }
    if (meta->magic != DPCM_MAGIC) {
        fprintf(stderr, "ERROR: %s: Invalid magic 0x%08X.\n", input_file_path, meta->magic);
        return 1;
    }
    if (meta->version != DPCM_VERSION) {
        fprintf(stderr, "ERROR: %s: Unsupported version %u. Supported only %u.\n", input_file_path, meta->version, DPCM_VERSION);
        return 1;
    }
    if (!(0 <= meta->format && meta->format < __DPCM_FORMAT_COUNT)) {
        fprintf(stderr, "ERROR: %s: Unsupported format.\n", input_file_path);
        return 1;
    }

    switch (meta->format) {
    case FORMAT_DPCM32:
        if (meta->sample_count != (sb.count - sizeof(DPCM_File))/sizeof(int32_t)) {
            fprintf(stderr, "ERROR: %s: Invalid amount of samples\n", input_file_path);
            return 1;
        }
        break;
    case FORMAT_DPCM1:
        if (meta->sample_count != (sb.count - sizeof(DPCM_File))/sizeof(uint64_t)) {
            fprintf(stderr, "ERROR: %s: Invalid amount of samples\n", input_file_path);
            return 1;
        }
        if (meta->dpcm1_bit_count < meta->sample_count) {
            fprintf(stderr, "ERROR: %s: Invalid amount of bit samples\n", input_file_path);
            return 1;
        }
        break;
    default:
        UNREACHABLE("DPCM_Format");
    }

    Abstract_DPCM_Player player = {0};
    ma_device_config config  = ma_device_config_init(ma_device_type_playback);

    switch (meta->format) {
    case FORMAT_DPCM32: {
        DPCM32_Player *p = malloc(sizeof(*p));
        memset(p, 0, sizeof(*p));

        p->samples = (void*)(sb.items + sizeof(DPCM_File));
        p->count   = meta->sample_count;

        player.p           = p;
        player.next_sample = dpcm32_next_sample;
    } break;

    case FORMAT_DPCM1: {
        DPCM1_Player *p = malloc(sizeof(*p));
        memset(p, 0, sizeof(*p));
        p->samples              = (void*)(sb.items + sizeof(DPCM_File));
        p->count                = meta->sample_count;
        p->bit_count            = meta->dpcm1_bit_count;
        p->bits_per_sample      = meta->dpcm1_bits_per_sample;
        p->amp_step_in_percents = meta->dpcm1_amp_step_in_percents;

        player.p           = p;
        player.next_sample = dpcm1_next_sample;
    } break;

    default:
        UNREACHABLE("DPCM_Format");
    }

    config.playback.format   = ma_format_s32;     // Set to ma_format_unknown to use the device's native format.
    config.playback.channels = DPCM_CHANNELS;     // Set to 0 to use the device's native channel count.
    config.sampleRate        = meta->sample_rate; // Set to 0 to use the device's native sample rate.
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

#include "dpcm.c"
#define NOB_IMPLEMENTATION
#include "nob.h"

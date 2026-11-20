#include "nob.h"
#undef temp_alloc
#include "dr_flac.h"
#include "stb_vorbis.c"
#include "flag.h"

#include "dpcm.h"

void usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s [OPTIONS] <input.flac> <output.dpcm>\n", program_name);
    fprintf(stderr, "OPTIONS:\n");
    flag_print_options(stderr);
}

int main(int argc, char **argv)
{
    char *format_name = "dpcm32";
    bool help = false;

    uint64_t dpcm1_bits_per_sample = 1;
    uint64_t dpcm1_amp_step_in_percents = 1;

    flag_str_var(&format_name, "f", format_name, "The format of the output DPCM file");
    flag_bool_var(&help, "help", false, "Print this help message");
    flag_uint64_var(&dpcm1_bits_per_sample, "b", 1, "Time step in samples for DPCM1 format");
    flag_uint64_var(&dpcm1_amp_step_in_percents, "a", 1, "Amplitude in percents for DPCM1 format");

    if (!flag_parse(argc, argv)) {
        usage(flag_program_name());
        flag_print_error(stderr);
        return 1;
    }

    if (help) {
        usage(flag_program_name());
        return 0;
    }

    argc = flag_rest_argc();
    argv = flag_rest_argv();

    if (argc <= 0) {
        usage(flag_program_name());
        fprintf(stderr, "ERROR: no input is provided\n");
        return 1;
    }
    const char *input_file_path = shift(argv, argc);

    if (argc <= 0) {
        usage(flag_program_name());
        fprintf(stderr, "ERROR: no output is provided\n");
        return 1;
    }
    const char *output_file_path = shift(argv, argc);

    DPCM_Format format = FORMAT_DPCM32;
    if (!get_dpcm_format_by_name(format_name, &format)) {
        fprintf(stderr, "Available DPCM formats:\n");
        for (DPCM_Format i = 0; i < __DPCM_FORMAT_COUNT; ++i) {
            fprintf(stderr, "    %s\n", dpcm_format_names[i]);
        }
        fprintf(stderr, "ERROR: invalid DPCM format\n");
        return 1;
    }

    int32_t *samples = NULL;
    int samples_count;
    int channels;
    int sample_rate;

    if (sv_end_with(sv_from_cstr(input_file_path), ".flac")){
        drflac* pFlac = drflac_open_file(input_file_path, NULL);
        if (!pFlac) {
            fprintf(stderr, "ERROR: could not open \"%s\"\n", input_file_path);
            return 1;
        }

        size_t samples_size_in_bytes = pFlac->totalPCMFrameCount*pFlac->channels*sizeof(drflac_int32);
        samples = malloc(samples_size_in_bytes);
        drflac_uint64 numberOfInterleavedSamplesActuallyRead = drflac_read_pcm_frames_s32(pFlac, pFlac->totalPCMFrameCount, samples);
        channels = pFlac->channels;
        sample_rate = pFlac->sampleRate;
        samples_count = numberOfInterleavedSamplesActuallyRead;
    } else if (sv_end_with(sv_from_cstr(input_file_path), ".ogg")) {
        int16_t *smaller_samples = NULL;

        samples_count = stb_vorbis_decode_filename(input_file_path, &channels, &sample_rate, &smaller_samples);
        if (samples_count < 0) {
            fprintf(stderr, "ERROR: could not open \"%s\"\n", input_file_path);
            return 1;
        }

        size_t samples_size_in_bytes = samples_count*channels*sizeof(*samples);
        samples = malloc(samples_size_in_bytes);

        for (int i = 0; i < samples_count; ++i){
            for (int j = 0; j < channels; ++j) {
                samples[i*channels + j] = (double)smaller_samples[i*channels + j]/INT16_MAX*INT32_MAX;
            }
        }
    } else {
        fprintf(stderr, "ERROR: Unsupported file format\n");
        return 1;
    }

    BBG("%d", channels);
    BBG("%d", sample_rate);
    BBG("%d", samples_count);
    BBG("%p", samples);

    switch (format) {
    case FORMAT_DPCM32: {
        Samples_S32 dpcm = {0};
        int32_t previous = 0;
        da_append(&dpcm, previous);
        for (int i = 0; i < samples_count; ++i) {
            int32_t current = samples[i*channels];
            da_append(&dpcm, current - previous);
            previous = current;
        }

        String_Builder sb = {0};
        DPCM_File meta = {
            .magic        = DPCM_MAGIC,
            .version      = DPCM_VERSION,
            .sample_rate  = sample_rate,
            .format       = FORMAT_DPCM32,
            .sample_count = dpcm.count,
        };
        sb_append_buf(&sb, &meta, sizeof(meta));
        sb_append_buf(&sb, dpcm.items, sizeof(*dpcm.items)*dpcm.count);

        if (!write_entire_file(output_file_path, sb.items, sb.count)) {
            return 1;
        }
        printf("Generated \"%s\"\n", output_file_path);
    } break;

    case FORMAT_DPCM1: {
        Samples_Packed_U1 dpcm = {0};

        int32_t needle = 0;
        int32_t step_up_quant   = INT_MAX/DPCM1_AMP_QUAT;
        int32_t step_down_quant = INT_MIN/DPCM1_AMP_QUAT;
        for (int i = 0; i < samples_count; ++i) {
            int32_t sample = samples[i*channels];
            for (size_t j = 0; j < dpcm1_bits_per_sample; ++j) {
                if (needle < sample) {
                    needle += step_up_quant*dpcm1_amp_step_in_percents;
                    packed_u1_append(&dpcm, 1);
                } else {
                    needle += step_down_quant*dpcm1_amp_step_in_percents;
                    packed_u1_append(&dpcm, 0);
                }
            }
        }

        String_Builder sb = {0};
        DPCM_File meta = {
            .magic                      = DPCM_MAGIC,
            .version                    = DPCM_VERSION,
            .sample_rate                = sample_rate,
            .format                     = FORMAT_DPCM1,
            .sample_count               = dpcm.count,
            .dpcm1_bits_per_sample      = dpcm1_bits_per_sample,
            .dpcm1_amp_step_in_percents = dpcm1_amp_step_in_percents,
            .dpcm1_bit_count            = dpcm.bit_count,
        };

        sb_append_buf(&sb, &meta, sizeof(meta));
        sb_append_buf(&sb, dpcm.items, sizeof(*dpcm.items)*dpcm.count);

        if (!write_entire_file(output_file_path, sb.items, sb.count)) {
            return 1;
        }
        printf("Generated \"%s\"\n", output_file_path);
    } break;

    default: UNREACHABLE("DPCM_Format");
    }

    return 0;
}

#include "dpcm.c"
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#define FLAG_IMPLEMENTATION
#include "flag.h"
#define NOB_IMPLEMENTATION
#include "nob.h"

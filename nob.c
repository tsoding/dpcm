#define NOB_IMPLEMENTATION
#include "thirdparty/nob.h"
#define FLAG_IMPLEMENTATION
#include "thirdparty/flag.h"

#define BUILD_FOLDER      "build/"
#define SRC_FOLDER        "src/"
#define THIRDPARTY_FOLDER "thirdparty/"
#define ASSETS_FOLDER     "assets/"

void cc(Cmd *cmd)
{
    cmd_append(cmd, "clang");
    cmd_append(cmd, "-Wall");
    cmd_append(cmd, "-Wextra");
    cmd_append(cmd, "-ggdb");
    cmd_append(cmd, "-I"THIRDPARTY_FOLDER);
}

void libs(Cmd *cmd)
{
    cmd_append(cmd, "-lm");
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!mkdir_if_not_exists(BUILD_FOLDER)) return 1;

    bool run   = false;
    bool play  = false;
    bool debug = false;
    bool help  = false;
    bool dpcm1 = false;

    flag_bool_var(&run, "run", false, "Run the encoder after the build");
    flag_bool_var(&play, "play", false, temp_sprintf("Run the player after the build if -%s is enabled", flag_name(&run)));
    flag_bool_var(&debug, "debug", false, "Run the programs in the debugger (gf2)");
    flag_bool_var(&dpcm1, "dpcm1", false, "Test dpcm1 format");
    flag_bool_var(&help, "help", false, "Print this help message");

    if (!flag_parse(argc, argv)) {
        flag_print_options(stderr);
        flag_print_error(stderr);
        return 1;
    }

    if (help) {
        flag_print_options(stdout);
        return 0;
    }

    Procs procs = {0};
    Cmd cmd = {0};

    cc(&cmd);
    cmd_append(&cmd, "-o", BUILD_FOLDER"flac2dpcm");
    cmd_append(&cmd, SRC_FOLDER"flac2dpcm.c");
    libs(&cmd);
    if (!cmd_run(&cmd, .async = &procs)) return 1;

    cc(&cmd);
    cmd_append(&cmd, "-o", BUILD_FOLDER"dpcm_player");
    cmd_append(&cmd, SRC_FOLDER"dpcm_player.c");
    libs(&cmd);
    if (!cmd_run(&cmd, .async = &procs)) return 1;

    if (!procs_flush(&procs)) return 1;

    if (run) {
        if (debug) cmd_append(&cmd, "gf2");
        cmd_append(&cmd, BUILD_FOLDER"flac2dpcm");
        if (dpcm1) cmd_append(&cmd, "-f", "dpcm1");
        // cmd_append(&cmd, ASSETS_FOLDER"just an obstacle in my way 1.flac");
        cmd_append(&cmd, ASSETS_FOLDER"checkpoint.ogg");
        cmd_append(&cmd, BUILD_FOLDER"output.dpcm");
        if (!cmd_run(&cmd)) return 1;

        if (play) {
            if (debug) cmd_append(&cmd, "gf2");
            cmd_append(&cmd, BUILD_FOLDER"dpcm_player");
            cmd_append(&cmd, BUILD_FOLDER"output.dpcm");
            if (!cmd_run(&cmd)) return 1;

            // This is useless code, but I'm keeping it in here 'cause I do not remember
            // how to use ffplay.
            if (0) {
                cmd_append(&cmd, "ffplay");
                cmd_append(&cmd, "-f", "s32le");
                cmd_append(&cmd, "-ar", "44100");
                cmd_append(&cmd, "-ac", "2");
                cmd_append(&cmd, "output.pcm");
                if (!cmd_run(&cmd)) return 1;
            }
        }
    }

    return 0;
}

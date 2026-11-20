#define NOB_IMPLEMENTATION
#include "thirdparty/nob.h"

#define BUILD_FOLDER      "build/"
#define SRC_FOLDER        "src/"
#define THIRDPARTY_FOLDER "thirdparty/"
#define ASSETS_FOLDER     "assets/"

void cc(Cmd *cmd)
{
    cmd_append(cmd, "clang");
    cmd_append(cmd, "-Wall");
    cmd_append(cmd, "-Wextra");
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

    bool run  = !false;
    bool play = !false;

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
        cmd_append(&cmd, BUILD_FOLDER"flac2dpcm");
        cmd_append(&cmd, ASSETS_FOLDER"just an obstacle in my way 1.flac");
        cmd_append(&cmd, BUILD_FOLDER"output.dpcm32");
        if (!cmd_run(&cmd)) return 1;

        if (play) {
            cmd_append(&cmd, BUILD_FOLDER"dpcm_player");
            cmd_append(&cmd, BUILD_FOLDER"output.dpcm32");
            if (!cmd_run(&cmd)) return 1;

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

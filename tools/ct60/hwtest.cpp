// HWTEST.TOS: runs the game's hardware detection (src/main/atari/hwdetect.cpp) alone,
// three times, prints the result and appends it to JOURNAUX\HWTEST.LOG (folder created if
// needed). With the argument AUTO (test bench) it ends at once; otherwise it waits for a key.
// Build: tools/ct60/build_hwtest.sh
#include <cstdio>
#include <cstring>
#include <mint/osbind.h>
#include "atari/hwdetect.hpp"

int main(int argc, char** argv)
{
    const bool automatic = argc > 1 && std::strcmp(argv[1], "AUTO") == 0;
    char line[256];
    Dcreate("JOURNAUX");
    std::printf("Cannonball CT60 - hardware detection\r\n");
    for (int pass = 1; pass <= 3; pass++)
    {
        hw_detect();
        hw_describe(line);
        std::printf("pass %d/3\r\n%s\r\n", pass, line);
        FILE* f = std::fopen("JOURNAUX\\HWTEST.LOG", "a");
        if (f) { std::fprintf(f, "pass %d/3 %s\r\n", pass, line); std::fclose(f); }
    }
    if (!automatic) { std::printf("Press a key.\r\n"); Cconin(); }
    return 0;
}

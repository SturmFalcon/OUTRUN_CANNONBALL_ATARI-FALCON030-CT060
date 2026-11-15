// HWTEST.TOS: runs the game's hardware detection (src/main/atari/hwdetect.cpp) alone,
// prints the result and appends it to HWTEST.LOG next to the program. Any key quits.
// Build: tools/ct60/build_hwtest.sh
#include <cstdio>
#include <mint/osbind.h>
#include "atari/hwdetect.hpp"

int main()
{
    char line[256];
    std::printf("Cannonball CT60 - hardware detection\r\nMeasuring (0.4 s)...\r\n");
    for (int pass = 1; pass <= 3; pass++)
    {
        hw_detect();
        hw_describe(line);
        std::printf("pass %d/3\r\n%s\r\n", pass, line);
        FILE* f = std::fopen("HWTEST.LOG", "a");
        if (f) { std::fprintf(f, "pass %d/3 %s\r\n", pass, line); std::fclose(f); }
    }
    std::printf("Press a key.\r\n");
    if (!(Kbshift(-1) & 0x10)) Cconin();   // CapsLock on = no wait (automatic runs)
    return 0;
}

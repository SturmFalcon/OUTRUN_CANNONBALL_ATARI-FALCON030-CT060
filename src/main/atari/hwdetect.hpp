#pragma once

/***************************************************************************
    Hardware detection for the Falcon port (CT60 work, see HARDWARE_DETECTION.md).

    Read once at start-up: CPU and FPU (cookies), CT60/CT63 (cookie "CT60"),
    machine, free ST-RAM and fast RAM, and the CPU clock MEASURED with a
    timed loop against the 200 Hz system counter (no declared value is
    trusted). The clock is derived from the loop count with the cycle cost
    of the loop given by the Motorola manuals for the detected CPU: until it
    has been checked on a real CT60 it is an estimate, and the raw count is
    kept so that it can be re-checked.

    Nothing is changed in the machine: no clock, cache or CT60 setting.
***************************************************************************/

#include <stdint.h>

enum HwProfile
{
    HWP_030      = 0,   // 68030 (stock Falcon 16 MHz, or any 030)
    HWP_040      = 1,   // 68040
    HWP_060_LOW  = 2,   // 68060 below 45 MHz
    HWP_060_60   = 3,   // 68060 45..75 MHz: main target (CT60 at 60 MHz)
    HWP_060_FAST = 4    // 68060 above 75 MHz (e.g. 95 MHz)
};

struct HwInfo
{
    long     cpu;            // _CPU cookie: 30, 40, 60 (0 if missing)
    long     fpu;            // _FPU cookie value (0 if missing)
    long     mch;            // _MCH cookie
    bool     ct60;           // "CT60" cookie present
    long     ct60_value;     // its value
    bool     mint;           // FreeMiNT running
    long     st_free;        // largest free ST-RAM block, bytes
    long     fast_free;      // largest free fast (TT/alternate) RAM block, bytes
    uint32_t spin_iters;     // loop iterations counted during the measurement
    uint32_t spin_ticks;     // 200 Hz ticks the measurement lasted
    uint32_t cycles_per_iter;// cost assumed for the detected CPU (16 for a 68060)
    uint32_t mhz_x10;        // measured clock, MHz x 10 (0 if not measured)
    HwProfile profile;
};

extern HwInfo g_hw;

// Fills g_hw. Takes about 0.4 s (two 160 ms measurements). Call once, before the sound starts.
void hw_detect();

// One line of text describing g_hw (for logs). buf must hold at least 200 chars.
void hw_describe(char* buf);
#ifdef HWDIAG
void hw_sound_snap(int slot);   // 0: before the game sets its sound up, 1: end of the bench run
#endif

const char* hw_profile_name(HwProfile p);

// First picture level for the adaptive rate (VBLs per picture: 1 = 60, 2 = 30, 3 = 20 at 60 Hz).
// [HYPOTHESIS] Every profile starts at 2 (30 pictures/s): the adaptive manager then moves from
// what each picture really costs, so the start only decides the first ~3 s. No MHz table is
// trusted until a real CT60 run has measured the cost per picture (PERFORMANCE_PROFILES.md).
int hw_start_level();

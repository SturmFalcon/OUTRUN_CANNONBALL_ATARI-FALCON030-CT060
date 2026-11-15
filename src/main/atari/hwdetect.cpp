/***************************************************************************
    Hardware detection (see hwdetect.hpp and HARDWARE_DETECTION.md).
***************************************************************************/

#include <cstdio>
#include <mint/osbind.h>
#include <mint/cookie.h>
#include "atari/hwdetect.hpp"

extern "C" void atari_hw_spin(uint32_t iterations);

HwInfo g_hw;

#define HZ_200 (*(volatile uint32_t*)0x4baL)

static const uint32_t SPIN_CHUNK = 256;   // iterations between two looks at the counter
static const uint32_t SPIN_TICKS = 32;    // 160 ms per measurement
static volatile uint32_t m_iters, m_ticks;

// Supervisor mode ($4BA is only readable there). Starts on a tick edge, then counts
// chunks of the loop until SPIN_TICKS ticks have gone by. Interrupts stay on: the
// 200 Hz and VBL handlers' time is counted as loop time, which can only lower the
// result by a few tenths of a percent on a 68060.
static long measure_super()
{
    uint32_t t = HZ_200;
    while (HZ_200 == t) { }
    const uint32_t start = HZ_200;
    uint32_t chunks = 0;
    while (HZ_200 - start < SPIN_TICKS)
    {
        atari_hw_spin(SPIN_CHUNK);
        chunks++;
    }
    m_ticks = HZ_200 - start;
    m_iters = chunks * SPIN_CHUNK;
    return 0;
}

static uint32_t cycles_for(long cpu)
{
    if (cpu >= 60) return 16;
    if (cpu >= 40) return 18;
    return 40;
}

static HwProfile profile_for(long cpu, uint32_t mhz_x10)
{
    if (cpu < 40)  return HWP_030;
    if (cpu < 60)  return HWP_040;
    if (mhz_x10 < 450) return HWP_060_LOW;
    if (mhz_x10 <= 750) return HWP_060_60;
    return HWP_060_FAST;
}

const char* hw_profile_name(HwProfile p)
{
    switch (p)
    {
        case HWP_030:      return "030";
        case HWP_040:      return "040";
        case HWP_060_LOW:  return "060-LENT";
        case HWP_060_60:   return "060-60MHZ";
        case HWP_060_FAST: return "060-RAPIDE";
    }
    return "?";
}

void hw_detect()
{
    HwInfo& h = g_hw;
    h.cpu = h.fpu = h.mch = h.ct60_value = 0;
    Getcookie(C__CPU, &h.cpu);
    Getcookie(C__FPU, &h.fpu);
    Getcookie(C__MCH, &h.mch);
    h.ct60 = Getcookie(0x43543630L /* 'CT60' */, &h.ct60_value) == C_FOUND;
    long dummy;
    h.mint = Getcookie(C_MiNT, &dummy) == C_FOUND;
    h.st_free   = Mxalloc(-1L, 0);
    h.fast_free = Mxalloc(-1L, 1);
    if (h.fast_free < 0) h.fast_free = 0;

    // Two measurements, the faster kept (the other may have been slowed by a disk or key interrupt).
    uint32_t best_iters = 0, best_ticks = 1;
    for (int i = 0; i < 2; i++)
    {
        Supexec(measure_super);
        if (m_iters * best_ticks > best_iters * m_ticks)   // < 2^32: about 1e6 iterations x 33 ticks
        {
            best_iters = m_iters;
            best_ticks = m_ticks;
        }
    }
    h.spin_iters = best_iters;
    h.spin_ticks = best_ticks;
    h.cycles_per_iter = cycles_for(h.cpu);
    // cycles per second = iters * cycles_per_iter * 200 / ticks; MHz x 10 = that / 100000
    // = iters * cycles_per_iter / (ticks * 500). 32-bit only (no 64-bit libgcc helpers, which
    // are 68020+ code in some toolchains and trap on a 68060): iters * 40 stays below 2^32 up to
    // 100 million iterations.
    h.mhz_x10 = best_ticks ? (best_iters * h.cycles_per_iter) / (best_ticks * 500) : 0;
    h.profile = profile_for(h.cpu, h.mhz_x10);
}

void hw_describe(char* buf)
{
    const HwInfo& h = g_hw;
    std::sprintf(buf, "HW cpu=%ld fpu=%lx mch=%lx ct60=%d(%lx) mint=%d st_free=%ld fast_free=%ld "
                 "spin=%lu/%luticks cyc=%lu clock=%lu.%luMHz profile=%s",
                 h.cpu, (unsigned long)h.fpu, (unsigned long)h.mch, (int)h.ct60, (unsigned long)h.ct60_value,
                 (int)h.mint, h.st_free, h.fast_free,
                 (unsigned long)h.spin_iters, (unsigned long)h.spin_ticks, (unsigned long)h.cycles_per_iter,
                 (unsigned long)(h.mhz_x10 / 10), (unsigned long)(h.mhz_x10 % 10), hw_profile_name(h.profile));
}

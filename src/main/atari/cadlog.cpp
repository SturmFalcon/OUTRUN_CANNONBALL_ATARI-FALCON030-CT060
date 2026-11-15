/***************************************************************************
    Display cadence log (see cadlog.hpp).
***************************************************************************/
#include <cstring>
#include <mint/osbind.h>
#include "atari/cadlog.hpp"

static volatile CadLog g_log;
static volatile uint32_t g_last_base = 0, g_run = 0;
static volatile bool g_counting = false;
static void (**g_slot)() = 0;

static inline uint32_t videl_base()
{
    return ((uint32_t)*(volatile uint8_t*)0xFFFF8201L << 16) |
           ((uint32_t)*(volatile uint8_t*)0xFFFF8203L << 8) |
            (uint32_t)*(volatile uint8_t*)0xFFFF820DL;
}

// Called by TOS at every VBL, registers already saved.
static void cad_vbl()
{
    const uint32_t b = videl_base();
    if (!g_counting) { g_last_base = b; g_run = 0; return; }
    g_log.vbls++;
    g_run++;
    if (b != g_last_base)
    {
        g_log.hold[g_run < CADLOG_MAX ? g_run : CADLOG_MAX]++;
        g_log.shown++;
        g_last_base = b;
        g_run = 0;
    }
}

static long install_super()
{
    const int n = *(volatile int16_t*)0x454;
    void (**q)() = *(void (***)())0x456;
    for (int i = 1; i < n; i++)          // slot 0 belongs to the VDI cursor
        if (q[i] == 0) { q[i] = cad_vbl; g_slot = &q[i]; return 1; }
    return 0;
}

static long remove_super()
{
    if (g_slot && *g_slot == cad_vbl) *g_slot = 0;
    g_slot = 0;
    return 0;
}

void cadlog_remove() { if (g_slot) Supexec(remove_super); }

bool cadlog_install()
{
    if (g_slot) return true;
    return Supexec(install_super) != 0;
}

void cadlog_reset()
{
    g_counting = false;
    std::memset((void*)&g_log, 0, sizeof(g_log));
    g_run = 0;
    g_counting = true;
}

void cadlog_get(CadLog* out)
{
    g_counting = false;
    std::memcpy(out, (const void*)&g_log, sizeof(CadLog));
    g_counting = true;
}

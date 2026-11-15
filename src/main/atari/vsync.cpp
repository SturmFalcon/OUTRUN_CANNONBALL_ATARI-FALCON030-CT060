/***************************************************************************
    Picture pacing locked on the vertical blank (see vsync.hpp).
***************************************************************************/
#include <mint/osbind.h>
#include "atari/vsync.hpp"

volatile uint32_t g_vbl = 0;
bool g_vsync_on = false;
uint32_t g_present_vbl = 0;
void (*g_vsync_idle)() = 0;
static volatile uint32_t pend_base = 0, pend_target = 0;
static volatile bool pend = false;
static void (**g_slot)() = 0;

// Called by TOS at every VBL (registers saved by TOS). The new base is written to the
// Videl and to screenpt ($45E), so TOS's own VBL code keeps the same address.
static void vsync_vbl()
{
    const uint32_t v = ++g_vbl;
    if (pend && (int32_t)(v - pend_target) >= 0)
    {
        const uint32_t b = pend_base;
        *(volatile uint8_t*)0xFFFF8201L = (uint8_t)(b >> 16);
        *(volatile uint8_t*)0xFFFF8203L = (uint8_t)(b >> 8);
        *(volatile uint8_t*)0xFFFF820DL = (uint8_t)b;
        *(volatile uint32_t*)0x45EL = b;
        pend = false;
    }
}

static long install_super()
{
    const int n = *(volatile int16_t*)0x454;
    void (**q)() = *(void (***)())0x456;
    for (int i = 1; i < n; i++)          // slot 0 belongs to the VDI cursor
        if (q[i] == 0) { q[i] = vsync_vbl; g_slot = &q[i]; return 1; }
    return 0;
}

static long remove_super()
{
    if (g_slot && *g_slot == vsync_vbl) *g_slot = 0;
    g_slot = 0;
    pend = false;
    return 0;
}

bool vsync_install()
{
    if (g_slot) return true;
    return Supexec(install_super) != 0;
}

void vsync_remove() { if (g_slot) Supexec(remove_super); }
bool vsync_installed() { return g_slot != 0; }

static volatile uint32_t m_vbls, m_ticks;
static long measure_super()
{
    volatile uint32_t* hz = (volatile uint32_t*)0x4BA;
    uint32_t t = *hz;
    while (*hz == t) { }
    const uint32_t t0 = *hz, v0 = g_vbl;
    while (*hz - t0 < 100) { }
    m_vbls = g_vbl - v0;
    m_ticks = *hz - t0;
    return 0;
}

uint32_t vsync_measure_hz_x100()
{
    if (!g_slot) return 0;
    Supexec(measure_super);
    return m_ticks ? m_vbls * 20000 / m_ticks : 0;   // vbls / (ticks / 200) * 100
}

void vsync_queue(const void* base, uint32_t target, void (*idle)())
{
    while (pend) { if (idle) idle(); }
    pend_base = (uint32_t)base;
    pend_target = target;
    pend = true;
}

bool vsync_waiting() { return pend; }

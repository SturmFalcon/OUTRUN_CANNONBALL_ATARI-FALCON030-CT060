#pragma once
/***************************************************************************
    ST-RAM write pump (CT60 work, see docs/ct60/ST_WRITE_OVERLAP.md).

    On a 68060 a long word written to ST-RAM goes through the Falcon's 16 MHz bus,
    while the CPU goes on with the next instructions: measured on the real CT60
    (Fallout MKBENCH, 2026-11-05), about 64 instructions are free per long written.
    A picture's conversion writes ~18 000 longs to the screen in one burst, so the
    CPU mostly waits for the bus. With the pump, the conversion only lists what is to
    be written (screen address + RGB565 pair, in fast RAM); the list is then written
    a few longs at a time from inside the NEXT picture's drawing loops (road lines,
    tiles, sprite rows), where there is computation to hide the bus behind, and what
    is left is written before that picture is shown.

    Hatari does not model the overlap: under Hatari the pump can only cost a little.
    The gain, if any, exists only on real hardware. outrun.ini stpump (0 = off).
***************************************************************************/
#include <stdint.h>

extern "C"
{
    extern uint32_t* g_pump_rd;    // next (address, value) pair to write
    extern uint32_t* g_pump_end;   // end of the pairs listed
    extern uint32_t  g_pump_k;     // longs written per call - 1 (dbra count)
    void atari_st_pump(void);      // writes up to g_pump_k + 1 longs; preserves every register
    void atari_st_flush(void);     // writes all that is left
    // Like atari_truecolor_diff, but lists the changed pairs at *wr instead of writing dst.
    uint32_t* atari_truecolor_stage(const uint16_t* px, uint16_t* shadow, const uint16_t* pal,
                                    uint16_t* dst, uint32_t longs, uint32_t* wr);
}

static inline void st_pump()
{
#ifndef PUMP_NOCALLS   // test aid: everything written by the flush before the picture is shown
    if (g_pump_rd < g_pump_end) atari_st_pump();
#endif
}

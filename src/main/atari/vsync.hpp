#pragma once
/***************************************************************************
    Picture pacing locked on the vertical blank (CT60 work, see
    CT60_60MHZ_OPTIMIZATION.md, "Cadence").

    A routine in the TOS VBL queue counts the VBLs (g_vbl) and shows a
    queued picture at the exact VBL it is meant for. With outrun.ini
    vsync = 1 (or 2 on a ~60 Hz display), the game steps are timed on this
    counter, two VBLs per step (29.97 steps/s on a 59.94 Hz VGA monitor),
    and the picture made after a step is shown a fixed number of VBLs later:
    every picture then stays on screen the same number of VBLs (2 at 30
    pictures/s) instead of 1, 2 or 3 depending on when it was finished.
***************************************************************************/
#include <stdint.h>

extern volatile uint32_t g_vbl;     // VBLs since vsync_install()

bool vsync_install();               // false if the VBL queue is full
void vsync_remove();                // before the program ends (quit_cleanup)
bool vsync_installed();

// VBL rate in Hz x 100, measured against the 200 Hz counter over 0.5 s (needs vsync_install()).
uint32_t vsync_measure_hz_x100();

// Shows `base` (screen memory) at VBL number `target` (or at the next VBL if that one has
// passed). Only one picture can wait: if one is still waiting, this first waits for it to be
// shown, calling idle() meanwhile (may be 0).
void vsync_queue(const void* base, uint32_t target, void (*idle)());

bool vsync_waiting();               // a queued picture is not on screen yet

// Set by main_atari.cpp's loop: true when the pictures are paced on the VBL, and the VBL at
// which the picture being made must appear. Render::flip_screen_base() (video_falcon.cpp)
// queues the picture for that VBL instead of calling VsetScreen. g_vsync_idle is called
// while a picture waits for a free slot (the sound service).
extern bool g_vsync_on;
extern uint32_t g_present_vbl;
extern void (*g_vsync_idle)();

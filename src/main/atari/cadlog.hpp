#pragma once
/***************************************************************************
    Display cadence log (test aid, used by -DBENCH_N builds).

    A routine in the TOS VBL queue reads the Videl screen base at every
    vertical blank and counts, for each picture shown, how many VBLs it stayed
    on screen. At 60 Hz, a steady 30 pictures/s shows every picture for
    exactly 2 VBLs; a 1 or a 3 is a visible judder. This measures what the
    eye sees, not what the game loop thinks it did.
***************************************************************************/
#include <stdint.h>

#define CADLOG_MAX 8
struct CadLog
{
    uint32_t vbls;                  // VBLs counted since cadlog_reset()
    uint32_t shown;                 // pictures that reached the screen
    uint32_t hold[CADLOG_MAX + 1];  // hold[n]: pictures shown n VBLs (n = CADLOG_MAX: n or more)
};

bool cadlog_install();              // false if the VBL queue is full
void cadlog_remove();               // must be called before the program ends (quit_cleanup)
void cadlog_reset();
void cadlog_get(CadLog* out);

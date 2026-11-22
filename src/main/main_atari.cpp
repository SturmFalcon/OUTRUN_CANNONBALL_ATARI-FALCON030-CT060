/***************************************************************************
    Cannonball Atari Mega STE Entry Point.

    Replaces main.cpp for this target. main.cpp is SDL2-specific (SDL_Init,
    SDL event loop, SDL_Delay-based pacing, game controller mapping file,
    force-feedback init) and is not compiled for PLATFORM_ATARI - this file
    reimplements only the platform glue, calling into exactly the same
    engine entry points (Roms::load_revb_roms, Outrun::init/tick,
    OSoundInt::tick, Video::init/prepare_frame/render_frame) that main.cpp
    does. No engine/hwvideo/hwaudio file is touched to make this work.

    v1 scope cuts, deliberately made to keep this a "transpose" rather than
    a rewrite of the frontend layer (see frontend/config.cpp's PLATFORM_ATARI
    branch for the matching config-side cuts):
      - No frontend menu (Menu class / menu.cpp) is linked in. The game
        boots straight to STATE_INIT_GAME every time.
      - No command-line parsing, no config.xml, no hiscore persistence.
      - No widescreen/hi-res video modes (atari/video.hpp only supports the
        native 320x224->320x200 cropped path).

    Builds for the Mega STE and the Falcon 030/060 (see README_ATARI.md).

    Copyright (c) port authors. See license.txt for more details.
***************************************************************************/

#include <cstdlib>
#include <cstdio>
#include <new>
#include <cstring>
#include <mint/osbind.h>
#include <mint/basepage.h>
#include <mint/cookie.h>
#include "atari/hwdetect.hpp"
#include "atari/cadlog.hpp"
#include "atari/vsync.hpp"
#include "atari/stpump.hpp"

// Stack size requested from the MiNTLib startup code (__stksize).
extern "C" long __stksize = 262144L;
#include "video.hpp"
#include "romloader.hpp"
#include "trackloader.hpp"
#include "stdint.hpp"
#include "main.hpp"
#include "engine/outrun.hpp"
#include "engine/omusic.hpp"
#include "atari/optmenu.hpp"
#include "engine/oroad.hpp"
#include "engine/oinitengine.hpp"
#include "frontend/config.hpp"

#include "engine/oinputs.hpp"
#include "engine/ooutputs.hpp"

#include "atari/timer.hpp"
#include "atari/input.hpp"
#include "atari/options.hpp"
#include "atari/dsp_replay.hpp"
#include "atari/dspmod.hpp"
#include "atari/fmdsp.hpp"

void atari_redraw_desktop();   // atari/gemredraw.cpp

using namespace cannonball;

// On a PC the OS hands out zeroed pages, so engine code that relies on
// freshly `new`ed memory being zero happens to work. TOS's Malloc does not
// zero anything, so replicate that behaviour explicitly.
void* operator new(std::size_t n)
{
    void* p = malloc(n ? n : 1);
    if (!p) throw std::bad_alloc();
    memset(p, 0, n);
    return p;
}
void* operator new[](std::size_t n) { return operator new(n); }
void operator delete(void* p) throw()   { free(p); }
void operator delete[](void* p) throw() { free(p); }

int    cannonball::state       = STATE_BOOT;
double cannonball::frame_ms    = 0;
int    cannonball::frame       = 0;
bool   cannonball::tick_frame  = true;
int    cannonball::fps_counter = 0;

Audio cannonball::audio;
bool pause_engine;
bool g_take_screenshot;
bool g_quit_confirm;

// The original (user-mode) stack pointer Super(0L) hands back when switching to supervisor mode
// at the very start of main() - saved so quit_func() can switch back to user mode before exit().
static long g_user_ssp = 0;

// Everything the game needs to give back before it goes away, but NOT the actual process
// termination call - see quit_func() (still used for the early ROM/video-init failure paths)
// and the STATE_QUIT handling in main_loop()/main() (the normal, in-game Escape/F10 path) for
// the two different ways this program can end.
static void quit_cleanup()
{
    cadlog_remove();        // the cadence log's VBL routine, if a bench build installed it
    g_vsync_on = false;
    vsync_remove();         // the VBL pacing routine (atari/vsync.hpp), before the screen is put back
    audio.stop_audio();
    dsp_replay.stop();      // hands Timer A, the sound matrix and the DSP back (no-op unless mod_dsp started it)
    dspmod.shutdown();      // same for DSPMOD (mod_dsp = 2)
    fmdsp.stop();           // and the FM program (fm_dsp = 1)
    input.shutdown();   // give the keyboard interrupt vector back (TOS does not reclaim vectors)
    // Put the display back the way it was before the game took over (see atari/video_falcon.cpp's
    // Render::disable()), so EmuTOS's own desktop is actually visible again afterwards - called
    // while still in supervisor mode, same as every other VsetScreen call in this codebase.
    video.disable();
}

// Ends the program from a point where returning from main() is not possible (start-up
// failures): cleans up, leaves supervisor mode and terminates.
static void quit_func(int code)
{
    quit_cleanup();
    // Back to user mode before terminating: this program spends its whole life in supervisor mode
    // (Super(0L) in main(), for direct hardware access). SuperToUser(), not a plain Super(ptr):
    // osbind.h documents a real EmuTOS bug where a plain Super(oldssp) gives the wrong user stack
    // pointer if the supervisor stack moved since the original Super(0L) - which it certainly has
    // by the time this runs - SuperToUser() backs up/restores around the trap to avoid it.
    SuperToUser((void*)g_user_ssp);
    // This whole quit_func()/Pterm() path was extensively tested this session (every combination
    // of exit()/Pterm(), with/without SuperToUser(), with/without any cleanup at all) and never
    // once returned to a usable desktop when triggered from inside the running game - it always
    // freezes the machine instead, GEMDOS's own doing, not this program's, since even a Pterm()
    // called with literally nothing else run first behaves identically. Kept only for the ROM
    // load / video init failure paths below (main(), before the game ever starts running - those
    // still need an immediate, unconditional way out). The Escape/F10 in-game quit path no longer
    // uses this function at all: it sets state = STATE_QUIT and lets main_loop()/main() unwind
    // and return from main() normally instead, which is what a well-behaved program launched from
    // the AUTO folder (as this one always is here, launched before the desktop/AES exists yet -
    // see README_ATARI.md) is expected to do, rather than force-terminating via a GEMDOS trap.
    Pterm(code);
}

// Step split in two halves for the VBL-paced display (see Outrun::tick_part()): set by
#ifdef STATE_CRC
// Test aid (-DSTATE_CRC, with -DAUTOPLAY): checksum of the game state after every complete
// step, written at steps 300, 600, 900 and 1200 to JOURNAUX\\STATECRC.LOG, then the program
// quits. Two builds or loops that step the game the same way give the same lines, whatever
// their picture rate: proof that the display pacing does not change the game.
#include "engine/ostats.hpp"
static void state_crc_step()
{
    static uint32_t crc = 0x12345678u, steps = 0;
    const uint32_t v[6] = { oroad.road_pos, (uint32_t)oinitengine.car_increment, (uint32_t)(uint16_t)oinitengine.car_x_pos,
                            ostats.score, (uint32_t)(uint16_t)ostats.time_counter, (uint32_t)outrun.game_state };
    for (int i = 0; i < 6; i++) { crc ^= v[i]; crc = (crc << 5) | (crc >> 27); crc *= 0x9E3779B1u; }
    steps++;
    if (steps % 300 == 0)
    {
        Dcreate("JOURNAUX");
        FILE* f = fopen("JOURNAUX\\STATECRC.LOG", "a");
        if (f) { fprintf(f, "STATECRC step=%lu crc=%08lx road_pos=%08lx score=%lu state=%d\r\n", (unsigned long)steps, (unsigned long)crc,
                         (unsigned long)oroad.road_pos, (unsigned long)ostats.score, (int)outrun.game_state); fclose(f); }
        printf("STATECRC step=%lu crc=%08lx\r\n", (unsigned long)steps, (unsigned long)crc);
        if (steps >= 1200) { fflush(stdout); state = STATE_QUIT; }
    }
}
#define STATE_CRC_STEP() state_crc_step()
#else
#define STATE_CRC_STEP() ((void)0)
#endif

// main_loop_vsync(); tick() then runs only the first half of a game step and tick_half2()
// the rest, so that a picture can be shown after each of the arcade's two vertical interrupts.
static bool g_split = false;
static bool g_half_pending = false;

static void tick_half2()
{
    if (!g_half_pending) return;
    g_half_pending = false;
    outrun.tick_part(tick_frame, 1);
    if (tick_frame) input.frame_done();
    STATE_CRC_STEP();
#ifndef NO_SOUND
    if (!audio.irq_mode()) { audio.fm_before_driver(); osoundint.tick(); }
#endif
}

// No event queue on this target: input.frame_done() polls the keyboard
// state maintained by the IKBD vector directly (see atari/input.cpp)
// instead of draining an SDL event queue.
static void tick()
{
    // (Escape goes back to the menu, whose EXIT quits: see STATE_GAME below. The "QUIT"
    // confirmation overlay, g_quit_confirm, is no longer raised.)

    frame++;

    if (config.fps == 60)
        tick_frame = frame & 1;
    else if (config.fps == 120)
        tick_frame = (frame & 3) == 1;

    if (tick_frame)
    {
        oinputs.tick();
        oinputs.do_gear();
    }

    switch (state)
    {
        case STATE_GAME:
        {
            if (tick_frame)
            {
                if (input.has_pressed(Input::TIMER)) outrun.freeze_timer = !outrun.freeze_timer;
                // The "PAUSE" caption itself is drawn straight into the RGB565 output in
                // atari/video_falcon.cpp's Render::draw_frame() (see cannonball::pause_engine
                // there) rather than through the engine's own HUD tile layer: the HUD's debug
                // font (engine/ohud.cpp's blit_text_new(), used by the FPS counter/debug overlay)
                // turned out not to render anything visible with this ROM set's tile data when
                // tried here - not worth the ROM-reverse-engineering to fix, when a handful of
                // hardcoded glyph bitmaps blitted after palette conversion sidesteps the whole
                // ROM-tile-encoding question and is guaranteed the same regardless of game state.
                if (input.has_pressed(Input::PAUSE)) pause_engine = !pause_engine;
#ifdef CHECKPOINT_LOG
                outrun.freeze_timer = true;   // test aid: the autoplay always gets to the checkpoints
#endif
                // Escape (or F1): back to the menu (atari/optmenu.hpp), whose EXIT quits
                if ((input.escape_down() || input.f1_down()) && !g_quit_confirm) { pause_engine = false; state = STATE_INIT_MENU; }
#ifdef MENU_SCRIPT
                { static int n = 0; if (++n == 600) { pause_engine = false; state = STATE_INIT_MENU; } }   // test: F1 after 20 s of game
#endif
                // F9: screenshot (not C - suspected host-level interception, see input.hpp's
                // SC_F9 comment). The actual capture happens in atari/video_falcon.cpp's
                // Render::draw_frame() (see g_take_screenshot there), which is where the final,
                // already-composited RGB565 picture is available; this just raises the request.
                if (input.has_pressed(Input::SCREENSHOT)) g_take_screenshot = true;
                // Escape/F10 are read directly at the top of tick(); this resolves the "QUIT"
                // overlay they raise (see g_quit_confirm there and in video_falcon.cpp). UP/DOWN
                // confirm/cancel it - not Y/N: both the has_pressed(QUIT_YES/QUIT_NO) path and a
                // direct scan_state read of Y/N (0x15/0x31) produced a system beep with no other
                // effect, for reasons not understood, while UP/DOWN are proven reliable all
                // session (steering) - has_pressed() itself is not the problem, only those two
                // particular scancodes are, so this stays on the ordinary has_pressed() path.
                if (g_quit_confirm)
                {
                    // state = STATE_QUIT, not quit_func(0): see quit_func()'s comment - letting
                    // main_loop() notice state==STATE_QUIT, do its own quit_cleanup() and return
                    // (main() then returns normally too, see there) is what actually gets back to
                    // a working desktop; explicitly terminating via Pterm() never did.
                    if (input.has_pressed(Input::UP)) state = STATE_QUIT;
                    if (input.has_pressed(Input::DOWN)) g_quit_confirm = false;
                }
            }

            if ((!pause_engine && !g_quit_confirm) || input.has_pressed(Input::STEP))
            {
#ifdef PERF_PRINT
                uint32_t q0 = PERF_NOW();
#endif
                if (g_split)
                {
                    // VBL-paced display (main_loop_vsync): the second half of the step, and what
                    // follows it here, runs one VBL later in tick_half2(), after a picture.
                    outrun.tick_part(tick_frame, 0);
                    g_half_pending = true;
                    break;
                }
                outrun.tick(tick_frame);
#ifdef PERF_PRINT
                uint32_t q1 = PERF_NOW();
#endif
                if (tick_frame) input.frame_done();
                STATE_CRC_STEP();
#ifndef NO_SOUND   // -DNO_SOUND also skips the engine's own sound command processing
                if (!audio.irq_mode()) { audio.fm_before_driver(); osoundint.tick(); }   // else the sound interrupt runs it (atari/audio.cpp)
#ifdef AUTOPLAY_MUSICSWAP
                // Test aid: at step 1500, the tune changes to track 1 (FM if Music\TRACK1.MOD is missing).
                { static uint32_t st = 0; if (++st == 1500) { extern OMusic omusic; omusic.play_music(0); } }
#endif
#endif
#ifdef PERF_PRINT
                {
                    static uint32_t a_out = 0, a_snd = 0, a_n = 0;
                    a_out += q1 - q0;
                    a_snd += PERF_NOW() - q1;
                    if (++a_n == 50)
                    {
                        PERF_PRINTF("STEP50 outrun=%lu rest=%lu (5ms)\r\n", (unsigned long)a_out, (unsigned long)a_snd);
                        a_out = a_snd = a_n = 0;
                    }
                }
#endif
            }
            else if (tick_frame)
            {
                input.frame_done();
            }
        }
        break;

        case STATE_INIT_GAME:
            if (config.engine.jap && !roms.load_japanese_roms())
            {
                state = STATE_QUIT;
            }
            else
            {
                tick_frame = true;
                pause_engine = false;
                outrun.init();
                state = STATE_GAME;
            }
            break;

        // The options menu (atari/optmenu.hpp), at start-up and with F1.
        case STATE_INIT_MENU:
            optmenu.init();
            state = STATE_MENU;
            break;

        case STATE_MENU:
            if (g_quit_confirm)
            {
                // the "QUIT" overlay (see STATE_GAME): up quits, down goes back
                if (input.has_pressed(Input::UP)) state = STATE_QUIT;
                if (input.has_pressed(Input::DOWN)) g_quit_confirm = false;
            }
            else
            {
                const int action = optmenu.tick();
                if (action == OptMenu::MENU_PLAY) state = STATE_INIT_GAME;
                if (action == OptMenu::MENU_EXIT) state = STATE_QUIT;   // as F10 then up
            }
            input.frame_done();
#ifndef NO_SOUND
            if (!audio.irq_mode()) { audio.fm_before_driver(); osoundint.tick(); }
#endif
            break;

        default:
            break;
    }

    outrun.outputs->writeDigitalToConsole();
}

#ifdef PLATFORM_FALCON
// Fixed-timestep loop (Falcon).  Game logic is decoupled from drawing: it always
// advances at the original 30 steps per second of real time, however slowly the
// frames can be drawn.  When drawing is slower than one step, several steps run
// back to back before the next picture (the picture is just the latest state);
// when the machine is faster, it waits for the next step.
#ifdef BENCH_N
#ifndef BENCH_START
#define BENCH_START 100      // pictures skipped (ROM loading, menus) before timing starts
#endif
#endif
#ifdef OLD_PACING
static void main_loop()
{
    volatile uint32_t* hz200 = (volatile uint32_t*)0x4BA;
    const int32_t TICK_UNITS = 20;          // 1/30 s, in units of 1/600 s (hz200 * 3)
    const int     MAX_TICKS  = 60;          // safety cap on steps run between two pictures
    const int32_t MAX_LOOP_UNITS = 600;     // never keep a picture waiting more than ~1 s catching up
    uint32_t next_tick  = *hz200 * 3;
    int      last_ticks = 1;
    int      renders    = 0;

    while (state != STATE_QUIT)
    {
        int ticks = 0;
        bool behind = false;
        const uint32_t loop_start = *hz200 * 3;
        while (ticks < MAX_TICKS)
        {
            uint32_t now = *hz200 * 3;
            if ((int32_t)(now - next_tick) < 0)
            {
                if (ticks) break;           // caught up: go and draw
                audio.service();            // ahead of real time: wait for the next step, keeping the sound DMA fed
                continue;
            }
            if (ticks && (int32_t)(now - loop_start) > MAX_LOOP_UNITS)
            {
                behind = true;              // catching up is taking too long: draw now
                break;
            }
            tick();
            // Real-time sound only when one step per picture is enough; otherwise
            // just keep the chips' timers running so the game's sound logic carries on.
#ifdef FORCE_SOUND
            audio.tick();                   // test option: always synthesise, however slow
#else
            if (ticks == 0 && last_ticks <= 1) audio.tick(); else audio.tick_muted();
#endif
            next_tick += TICK_UNITS;
            ticks++;
        }
        if (ticks == MAX_TICKS || behind)
            next_tick = *hz200 * 3;         // hopelessly behind: forget the backlog
        last_ticks = ticks;

        video.prepare_frame();
        video.render_frame();

        renders++;
#ifdef BENCH_N
        // Test aid (-DBENCH_N=400): time N pictures without printing anything in between
        // (console output scrolls the screen and would distort the measurement).
        {
            static uint32_t b_hz = 0, last_p = 0;
            static int b_frame = 0, cnt = 0;
            static double sum = 0, sumsq = 0, ssum = 0, ssumsq = 0; static int maxms = 0;
            static int last_f = 0;
            const uint32_t p1 = *hz200 * 3;
            if (renders == BENCH_START) { b_hz = *hz200; b_frame = cannonball::frame; last_p = p1; }
            else if (renders > BENCH_START && renders <= BENCH_START + BENCH_N)
            {
                double d = (double)(p1 - last_p) * 1000.0 / 600.0;
                last_p = p1; sum += d; sumsq += d * d; cnt++;
                if ((int)d > maxms) maxms = (int)d;
                { double sp = (double)(cannonball::frame - last_f); ssum += sp; ssumsq += sp * sp; }
            }
            if (renders >= BENCH_START && renders <= BENCH_START + BENCH_N) last_f = cannonball::frame;
            if (0)
            {
            }
            if (renders == BENCH_START + BENCH_N)
            {
                uint32_t dt = *hz200 - b_hz;
                double mean = sum / cnt, var = sumsq / cnt - mean * mean;
                double smean = ssum / cnt, svar = ssumsq / cnt - smean * smean;
                printf("BENCH pictures=%d hz200=%lu logic=%d K=0 interval_ms=%d max_ms=%d std_ms=%d steps_per_pic_x100=%d std_x100=%d%c%c", BENCH_N, (unsigned long)dt,
                       cannonball::frame - b_frame, (int)mean, maxms, (int)(var > 0 ? __builtin_sqrt(var) : 0), (int)(smean * 100), (int)(100 * (svar > 0 ? __builtin_sqrt(svar) : 0)), 13, 10);
            }
        }
#endif
        if (renders == 10 || renders == 40 || (renders % 25) == 0)
            PERF_PRINTF("MARK frame=%d hz200=%lu state=%d pos=%ld speed=%ld logic=%d\r\n", renders, (unsigned long)*hz200, (int)outrun.game_state, (long)(oroad.road_pos >> 16), (long)(oinitengine.car_increment >> 16), cannonball::frame);
    }

    quit_func(0);
}
#else
uint32_t g_vsync_hz_x100 = 0;              // measured VBL rate (0 = vsync not used)
static void vsync_idle() { audio.service(); }

// Fixed-cadence loop (Falcon).  Steps run at their scheduled times, 30 per second; a picture is
// drawn after every K-th step, K being the smallest number that lets a picture fit in the time
// of K steps (1 = 30 pictures/s, 2 = 15, ...), so the picture rate is steady and every picture
// advances the game by the same number of steps.  Sound is synthesised only when the machine
// was seen to be idle (waiting for the next step) for long enough.
static void main_loop()
{
    volatile uint32_t* hz200 = (volatile uint32_t*)0x4BA;
    const int32_t TICK_UNITS = 20;          // 1/30 s, in units of 1/600 s (hz200 * 3)
    const int     K_MAX      = 4;
    const uint32_t SOUND_UNITS = 10;        // about what one step of synthesis costs on a fast 060 (1/600 s)
    // Time in 1/600 s units: the 200 Hz counter x 3, or, with the pictures paced on the VBL
    // (atari/vsync.hpp), the VBL count x 10 - at ~60 Hz one VBL is ~10 units, so a step
    // (TICK_UNITS = 20) is exactly two VBLs.
    #define SCHED_NOW() (g_vsync_on ? g_vbl * 10 : *hz200 * 3)
    uint32_t next_tick = SCHED_NOW();
    int      K = atari_opt.cadence ? atari_opt.cadence : 2, steps = 0, votes = 0;
    int      pic_acc = 0;                   // pictures owed, in 1/60 units (fps and automatic mode)
    // Automatic mode (cadence = 0, fps = 0): pictures per second in 1/2 units, from 30 down
    // to 7.5. It goes down one level as soon as the current one has not fitted for a few
    // pictures, and up one level only after ~3 s in which the slowest recent pictures would
    // have fitted the faster level with 20 % to spare: no back and forth at a threshold.
    static const int LEVEL2[] = { 60, 50, 40, 30, 20, 15 };
    const int NLEVELS = 6;
    int      lvl = 3, up_votes = 0, down_votes = 0;
    uint32_t P_peak = 0;                    // slowest recent picture, decaying (1/600 s)
    uint32_t P = 0, L = 0;                  // smoothed picture / logic-step time, 1/600 s
    uint32_t idle = 0, wait_start = 0;
    bool     waiting = false, audio_on = false;
    int      renders = 0;

    while (state != STATE_QUIT)
    {
        uint32_t now = SCHED_NOW();
        if ((int32_t)(now - next_tick) < 0)
        {
            if (!waiting) { waiting = true; wait_start = *hz200 * 3; }
            audio.service();                // ahead of schedule: keep the sound DMA fed
            continue;
        }
        const uint32_t now_h = *hz200 * 3;    // the same moment on the 200 Hz clock (durations)
        if (waiting) { idle += now_h - wait_start; waiting = false; }
#ifndef CATCHUP_STEPS
#define CATCHUP_STEPS 6
#endif
        if ((int32_t)(now - next_tick) > CATCHUP_STEPS * TICK_UNITS)
            next_tick = now;                // hopelessly behind: forget the backlog

        tick();
#ifdef AUDIO_TIMING
        { extern uint32_t g_logic; g_logic++; }
#endif
        const uint32_t t1 = *hz200 * 3;
        L = (3 * L + (t1 - now_h)) / 4;
#ifdef FORCE_SOUND
        audio.tick();
#else
        if (audio_on) audio.tick(); else audio.tick_muted();
#endif
        next_tick += TICK_UNITS;

        if (atari_opt.cadence == 0)
        {
            // fps = N (or the automatic level): a picture after the steps that bring N/30 of a
            // picture each, e.g. 25 = five pictures out of six steps
            pic_acc += atari_opt.fps ? 2 * atari_opt.fps : LEVEL2[lvl];
            if (pic_acc < 60)
                continue;
            pic_acc -= 60;
        }
        else if (++steps < K)
            continue;
        steps = 0;

        const uint32_t p0 = *hz200 * 3;
        // With VBL pacing: this picture shows the game as of the step just made; it appears
        // vsync_lat VBLs after that step's period ends (next_tick is the start of the next step).
        g_present_vbl = next_tick / 10 + atari_opt.vsync_lat;
#ifdef AUDIO_TIMING
        extern uint32_t g_t_prep, g_t_draw; extern uint32_t atari_fine_time();
        const uint32_t f0 = atari_fine_time();
        video.prepare_frame();
        const uint32_t f1 = atari_fine_time();
        video.render_frame();
        g_t_prep += f1 - f0; g_t_draw += atari_fine_time() - f1;
#else
        video.prepare_frame();
        video.render_frame();
#endif
#ifdef AUDIO_TIMING
        { extern uint32_t g_pictures; g_pictures++; }
#endif
        const uint32_t p1 = *hz200 * 3;
        P = (3 * P + (p1 - p0)) / 4;
        P_peak -= P_peak / 16;
        if (p1 - p0 > P_peak) P_peak = p1 - p0;
        renders++;
#ifdef AUTOSHOT
        // Test aid (-DAUTOSHOT, with -DAUTOPLAY): saves two pictures of the drive as SHOTnnnn.PNG
        // without anyone at the keyboard, so the display can be looked at outside the emulator.
#ifdef AUTOPLAY_GRASS
        if (renders >= 300 && renders % 20 == 0 && renders <= 700) g_take_screenshot = true;
#else
        if (renders == 170 || renders == 260) g_take_screenshot = true;
#endif
#endif

        // K for the next pictures: P + K*L must fit in K*TICK_UNITS (10 % margin).
        {
            const int32_t room = TICK_UNITS - (int32_t)L;
            int target = K_MAX;
            if (room > 0)
            {
                target = (int)(((P * 11 / 10) + room - 1) / room);
                if (target < 1) target = 1;
                if (target > K_MAX) target = K_MAX;
            }
            (void)target; (void)votes;
            if (atari_opt.cadence)
                K = atari_opt.cadence;      // fixed by the option
            else
            {
                // a level fits when a picture plus the steps it covers (60 / LEVEL2) fit their time
                #define FITS(l, pic, margin) (room > 0 && (int32_t)(pic) * (100 + (margin)) * LEVEL2[l] <= 60 * 100 * room)
                if (!atari_opt.fps)
                {
#ifdef AUDIO_TIMING
                    extern uint32_t g_rate_changes, g_rate; const int lvl_before = lvl;
#endif
                    if (!FITS(lvl, P, 0)) { if (++down_votes >= 6 && lvl < NLEVELS - 1) { lvl++; down_votes = up_votes = 0; } }
                    else down_votes = 0;
                    if (lvl > 0 && FITS(lvl - 1, P_peak, 20))
                    {
                        if (++up_votes >= 3 * LEVEL2[lvl] / 2) { lvl--; up_votes = down_votes = 0; }
                    }
                    else up_votes = 0;
#ifdef AUDIO_TIMING
                    if (lvl != lvl_before) g_rate_changes++;
                    g_rate = LEVEL2[lvl];
#endif
                }
                #undef FITS
                // steps per picture, for the sound's idle-time rule below
                const int r2 = atari_opt.fps ? 2 * atari_opt.fps : LEVEL2[lvl];
                K = (60 + r2 - 1) / r2;
            }
        }
        // Sound on when the last group left enough idle time for K steps of synthesis
        // (switch on with a margin, off only when it has clearly run out).
        if (atari_opt.sound == 0)      audio_on = false;
        else if (atari_opt.sound == 1) audio_on = true;
        else if (audio_on)             audio_on = idle >= (uint32_t)K * 2;
        else                           audio_on = idle >= (uint32_t)K * SOUND_UNITS;
        idle = 0;

#ifdef BENCH_N
        // Test aid (-DBENCH_N=400): time N pictures and report their rhythm, printing nothing in between.
        {
            static uint32_t b_hz = 0, last_p = 0;
            static int b_frame = 0, cnt = 0;
            static double sum = 0, sumsq = 0, ssum = 0, ssumsq = 0; static int maxms = 0;
            static int last_f = 0;
            if (renders == BENCH_START) { b_hz = *hz200; b_frame = cannonball::frame; last_p = p1; if (cadlog_install()) cadlog_reset();
#ifdef STWRITE_COUNT
                { extern uint32_t g_st_longs, g_st_rows, g_st_words; g_st_longs = g_st_rows = g_st_words = 0; }
#endif
            }
            else if (renders > BENCH_START && renders <= BENCH_START + BENCH_N)
            {
                double d = (double)(p1 - last_p) * 1000.0 / 600.0;      // ms between two pictures
                last_p = p1; sum += d; sumsq += d * d; cnt++;
                if ((int)d > maxms) maxms = (int)d;
                { double sp = (double)(cannonball::frame - last_f); ssum += sp; ssumsq += sp * sp; }
            }
            if (renders >= BENCH_START && renders <= BENCH_START + BENCH_N) last_f = cannonball::frame;
            if (0)
            {
            }
            if (renders == BENCH_START + BENCH_N)
            {
                uint32_t dt = *hz200 - b_hz;
                double mean = sum / cnt, var = sumsq / cnt - mean * mean;
                double smean = ssum / cnt, svar = ssumsq / cnt - smean * smean;
                printf("BENCH pictures=%d hz200=%lu logic=%d K=%d interval_ms=%d max_ms=%d std_ms=%d steps_per_pic_x100=%d std_x100=%d audio=%d%c%c", BENCH_N, (unsigned long)dt,
                       cannonball::frame - b_frame, K, (int)mean, maxms, (int)(var > 0 ? __builtin_sqrt(var) : 0), (int)(smean * 100), (int)(100 * (svar > 0 ? __builtin_sqrt(svar) : 0)), (int)audio_on, 13, 10);
                {
                    CadLog c; cadlog_get(&c);
                    printf("CADENCE vbls=%lu shown=%lu drawn=%d hold1=%lu hold2=%lu hold3=%lu hold4=%lu hold5=%lu hold6=%lu hold7=%lu hold8+=%lu%c%c",
                           (unsigned long)c.vbls, (unsigned long)c.shown, BENCH_N, (unsigned long)c.hold[1], (unsigned long)c.hold[2], (unsigned long)c.hold[3],
                           (unsigned long)c.hold[4], (unsigned long)c.hold[5], (unsigned long)c.hold[6], (unsigned long)c.hold[7], (unsigned long)c.hold[8], 13, 10);
                    char hw[512]; hw_describe(hw); printf("%s%c%c", hw, 13, 10);
                    printf("VSYNC on=%d hz_x100=%lu lat=%d%c%c", (int)g_vsync_on, (unsigned long)g_vsync_hz_x100, atari_opt.vsync_lat, 13, 10);
#ifdef STWRITE_COUNT
                    extern uint32_t g_st_longs, g_st_rows, g_st_words;
                    printf("STWRITE bytes_per_pic=%lu changed_word_bytes_per_pic=%lu rows_per_pic=%lu%c%c", (unsigned long)(g_st_longs * 4 / BENCH_N), (unsigned long)(g_st_words * 2 / BENCH_N), (unsigned long)(g_st_rows / BENCH_N), 13, 10);
#endif
                    // The same three lines in JOURNAUX\CBBENCH.LOG (test bench convention: relative path,
                    // one file, appended), so the result survives without a console.
                    Dcreate("JOURNAUX");
                    FILE* bl = fopen("JOURNAUX\\CBBENCH.LOG", "a");
                    if (bl)
                    {
                        fprintf(bl, "BENCH pictures=%d hz200=%lu logic=%d K=%d interval_ms=%d max_ms=%d std_ms=%d steps_per_pic_x100=%d std_x100=%d audio=%d\r\n", BENCH_N, (unsigned long)dt,
                                cannonball::frame - b_frame, K, (int)mean, maxms, (int)(var > 0 ? __builtin_sqrt(var) : 0), (int)(smean * 100), (int)(100 * (svar > 0 ? __builtin_sqrt(svar) : 0)), (int)audio_on);
                        fprintf(bl, "CADENCE vbls=%lu shown=%lu drawn=%d hold1=%lu hold2=%lu hold3=%lu hold4=%lu hold5=%lu hold6=%lu hold7=%lu hold8+=%lu\r\n",
                                (unsigned long)c.vbls, (unsigned long)c.shown, BENCH_N, (unsigned long)c.hold[1], (unsigned long)c.hold[2], (unsigned long)c.hold[3],
                                (unsigned long)c.hold[4], (unsigned long)c.hold[5], (unsigned long)c.hold[6], (unsigned long)c.hold[7], (unsigned long)c.hold[8]);
                        fprintf(bl, "%s\r\n", hw);
                        fprintf(bl, "VSYNC on=%d hz_x100=%lu lat=%d\r\n", (int)g_vsync_on, (unsigned long)g_vsync_hz_x100, atari_opt.vsync_lat);
                        fclose(bl);
                    }
                }
                // Exit cleanly right after printing: stdout to a redirected file is fully buffered
                // (not line-buffered) while the guest process is alive, so without an explicit exit
                // here the BENCH line just sits in the C library's buffer and is never seen - only
                // process termination (normal exit(), which flushes stdio) writes it out.
                fflush(stdout);
                quit_func(0);
            }
        }
#endif
        if (renders == 10 || renders == 40 || (renders % 25) == 0)
        {
            PERF_PRINTF("MARK frame=%d hz200=%lu K=%d P=%lu L=%lu audio=%d logic=%d\r\n", renders, (unsigned long)*hz200, K, (unsigned long)P, (unsigned long)L, (int)audio_on, cannonball::frame);
#ifdef MARK_FILE
            // Test aid: overwrites (not appends) a single-line file with the latest MARK snapshot
            // plus `state`, at the same (already infrequent) cadence as the line above - so whatever
            // is on disk when the process is killed is the LAST picture that actually completed,
            // letting a freeze be located precisely (e.g. state already STATE_QUIT but stuck before
            // quit_func, vs stuck earlier still in STATE_GAME).
            {
                FILE* lf = fopen("MARK.TXT", "w");
                if (lf) { fprintf(lf, "renders=%d frame=%d hz200=%lu K=%d state=%d game_state=%d steps=%d\r\n",
                                   renders, cannonball::frame, (unsigned long)*hz200, K, (int)state, (int)outrun.game_state, steps); fclose(lf); }
            }
#endif
        }
    }

    quit_cleanup();
}

// ------------------------------------------------------------------------------------------
// VBL-paced loop (atari/vsync.hpp, outrun.ini vsync; CT60 work, see ADAPTIVE_FPS.md).
//
// The arcade runs its logic at 30 Hz but takes a vertical interrupt at 60 Hz: each game step
// is two halves, Outrun::tick_part(.., 0) (logic, road CPU, first interrupt) and
// tick_part(.., 1) (second interrupt). Here one half is due per VBL, so the game keeps its
// speed (two VBLs per step), and a picture is drawn every n halves: n = 1, 2, 3, 4 gives 60,
// 30, 20, 15 pictures per second, each one on screen exactly n VBLs (vsync_queue shows it at a
// fixed VBL). Rates that do not divide the VBL rate (40, 50...) would show pictures for 1 then
// 2 VBLs and are not offered.
//
// n is chosen from what the pictures and the halves really cost (Timer C fine clock, 26 us):
// one level down when the cost of a group of n halves plus its picture no longer fits n VBLs
// (6 pictures in a row) or two pictures missed their VBL out of the last 16; one level up only
// after about 3 s in which the average cost would fit the faster level with 10 % to spare and
// the recent peak would still be shown on time (n - 1 + vsync_lat VBLs). outrun.ini vrate fixes
// n instead (60, 30, 20, 15).
// ------------------------------------------------------------------------------------------

static uint32_t fine_now();
#ifdef STAGEPROF
extern "C" unsigned long stage_now() { return fine_now(); }
#ifdef LOOP_SILENCE
extern uint32_t g_sil_frames;
static unsigned long g_sil_frames_get() { return g_sil_frames; }
#else
static unsigned long g_sil_frames_get() { return 0xffffffffUL; }   // option not built in
#endif
extern int g_fmdsp_active();
void stage_floors(const uint16_t* pixels, unsigned long out[4]);
#endif
// 26.04 us units (Timer C runs from 192 down to 1 between two 200 Hz ticks). Supervisor only.
static uint32_t fine_now()
{
    volatile uint32_t* hz200 = (volatile uint32_t*)0x4BA;
    volatile uint8_t* tcdr = (volatile uint8_t*)0xFFFFFA23L;
    uint32_t h1, h2, c;
    do { h1 = *hz200; c = *tcdr; h2 = *hz200; } while (h1 != h2);
    if (c == 0 || c > 192) c = 192;
    return h1 * 192 + (192 - c);
}

int g_vrate_level = 2;                      // VBLs per picture now in use (for the menu / logs)
uint32_t g_vsync_missed = 0;                // pictures skipped because the game was a group behind

#ifdef SHOTHALVES
static uint32_t g_halves = 0;
uint32_t g_shot_tag = 0;
#endif
extern bool g_stpump_on;
void stpump_present_pending();

static void main_loop_vsync()
{
    static const int N_MAX = 6;             // 10 pictures/s at 60 Hz
    const uint32_t vbl_fine = g_vsync_hz_x100 ? 38400u * 100u / g_vsync_hz_x100 : 640u;
    bool fixed = atari_opt.vrate != 0;
    int n = fixed ? (atari_opt.vrate >= 60 ? 1 : atari_opt.vrate >= 30 ? 2 : atari_opt.vrate >= 20 ? 3 : 4)
                  : hw_start_level();
    int n_min = 1;
    uint32_t S = g_vbl;                     // VBL slot of the next group's first half
    uint32_t R = 0, Lh = 0, R_peak = 0, Lh_peak = 0;   // fine units: picture, one half (smoothed / peak)
    int down_votes = 0, up_count = 0, grace = 0;
    uint32_t miss_hist = 0;                 // last 32 pictures, bit = 1 if it missed its VBL
    uint32_t idle = 0;
    bool audio_on = false;
    int phase = 0, renders = 0;
    g_split = true;
    g_half_pending = false;
    if (atari_opt.stpump > 0 && g_hw.cpu >= 40)
    {
        g_pump_k = (uint32_t)(atari_opt.stpump - 1);
        g_stpump_on = true;
    }
    g_vsync_missed = 0;
#ifdef BENCH_N
    uint32_t b_hz = 0, b_steps = 0, b_lvl[N_MAX + 1] = { 0 }, b_missed = 0, b_rmax = 0, b_late = 0;
    uint32_t b_hist[256] = { 0 };           // picture cost (render + its halves), 1 ms bins
    double b_rsum = 0, b_r = 0, b_h = 0; int b_cnt = 0;
    volatile uint32_t* hz200 = (volatile uint32_t*)0x4BA;
#endif

    int vrate_seen = atari_opt.vrate;
    g_vrate_level = n;
    while (state != STATE_QUIT)
    {
        // The options menu changed the rate: a fixed one at once, AUTO from the current level.
        if (atari_opt.vrate != vrate_seen)
        {
            vrate_seen = atari_opt.vrate;
            fixed = vrate_seen != 0;
            if (fixed) n = vrate_seen >= 60 ? 1 : vrate_seen >= 30 ? 2 : vrate_seen >= 20 ? 3 : 4;
            down_votes = up_count = 0;
            miss_hist = 0;
            g_vrate_level = n;
        }
        // Wait for the group's first VBL (the sound DMA is fed meanwhile).
        uint32_t w0 = fine_now();
        while ((int32_t)(g_vbl - S) < 0) audio.service();
        idle += fine_now() - w0;
        if ((int32_t)(g_vbl - S) > 6 * N_MAX)
            S = g_vbl;                      // hopelessly behind: forget the backlog (the game slows)

        // n halves back to back (they only have to be done before the picture).
        const uint32_t g0 = fine_now();
        for (int i = 0; i < n && state != STATE_QUIT; i++)
        {
            if (phase == 0)
            {
                tick();
                if (atari_opt.sound == 0)      audio_on = false;
                else if (atari_opt.sound == 1) audio_on = true;
#ifdef FORCE_SOUND
                audio.tick();
#else
                if (audio_on) audio.tick(); else audio.tick_muted();
#endif
            }
            else
                tick_half2();
            phase ^= 1;
#ifdef SHOTHALVES
            g_halves++;
#endif
        }
        const uint32_t g1 = fine_now();
        const uint32_t lh = (g1 - g0) / n;
        Lh = (3 * Lh + lh) / 4;
        Lh_peak -= Lh_peak / 16; if (lh > Lh_peak) Lh_peak = lh;
        S += n;

        // Sound: on when the last group left enough idle time (as in main_loop()).
        if (atari_opt.sound == 2)
        {
            if (audio_on) audio_on = idle >= 128u * n;
            else          audio_on = idle >= 640u * n;
        }
        idle = 0;

        // A whole group late: no picture this time, the game keeps its pace.
        if ((int32_t)(g_vbl - S) >= n)
        {
            if (g_stpump_on) { g_present_vbl = S + atari_opt.vsync_lat; stpump_present_pending(); }
            g_vsync_missed++;
            miss_hist = (miss_hist << 1) | 1;
#ifdef BENCH_N
            if (renders >= BENCH_START) b_missed++;
#endif
        }
        else
        {
            g_present_vbl = S + atari_opt.vsync_lat;
#ifdef SHOTHALVES
            // Test aid (-DSHOTHALVES, with -DAUTOPLAY -DDUMP_INDEX): the engine's picture after a
            // given number of half steps, IXhhhhh.BIN, whatever the picture rate: two builds can then
            // be compared picture for picture (renderer changes must give identical indices).
            if (g_halves % 50 == 0 && g_halves >= 400 && g_halves <= 2400) { g_shot_tag = g_halves; g_take_screenshot = true; }
            if (g_halves > 2400) state = STATE_QUIT;
#endif
            video.prepare_frame();
            if (g_stpump_on) stpump_present_pending();   // the picture before: rest of its list, then shown
            video.render_frame();
            // queued at or after its VBL: shown one VBL late (or more)
            const bool late = (int32_t)(g_vbl - g_present_vbl) >= 0;
            miss_hist = (miss_hist << 1) | (late ? 1u : 0u);
#ifdef BENCH_N
            if (late && renders >= BENCH_START) b_late++;
#endif
            const uint32_t r = fine_now() - g1;
            R = (3 * R + r) / 4;
            R_peak -= R_peak / 16; if (r > R_peak) R_peak = r;
            renders++;
#ifdef AUTOSHOT
            if (renders == 170 || renders == 260) g_take_screenshot = true;
#endif
#ifdef BENCH_N
            if (renders == BENCH_START)
            {
                b_hz = *hz200; b_steps = cannonball::frame;
                if (cadlog_install()) cadlog_reset();
#ifdef STAGEPROF
                for (int k = 0; k < 10; k++) g_stage[k] = 0;
                { extern uint32_t g_isr_calls, g_isr_steps, g_isr_time, g_isr_max; g_isr_calls = g_isr_steps = g_isr_time = g_isr_max = 0; }
#endif
            }
            else if (renders > BENCH_START)
            {
                const uint32_t cost = r + (g1 - g0);
                const uint32_t ms = cost * 1000u / 38400u;
                b_hist[ms < 255 ? ms : 255]++;
                if (ms > b_rmax) b_rmax = ms;
                b_rsum += cost * 1000.0 / 38400.0; b_cnt++;
                b_r += r * 1000.0 / 38400.0; b_h += (g1 - g0) * 1000.0 / 38400.0 / n;
                b_lvl[n]++;
            }
            if (renders == BENCH_START + BENCH_N)
            {
                const uint32_t dt = *hz200 - b_hz;
                uint32_t acc = 0, p95 = 0;
                for (int k = 0; k < 256; k++) { acc += b_hist[k]; if (acc * 100 >= 95u * (uint32_t)b_cnt) { p95 = k; break; } }
                CadLog c; cadlog_get(&c);
                char hw[512]; hw_describe(hw);
                char l1[256], l2[256], l3[256];
                std::sprintf(l1, "VBENCH pictures=%d hz200=%lu logic=%lu pics_per_s_x10=%lu cost_ms_avg_x10=%d cost_ms_p95=%lu cost_ms_max=%lu missed=%lu late=%lu render_ms_x10=%d half_ms_x10=%d",
                             BENCH_N, (unsigned long)dt, (unsigned long)(cannonball::frame - b_steps), (unsigned long)(BENCH_N * 2000UL / (dt ? dt : 1)),
                             (int)(b_rsum * 10 / (b_cnt ? b_cnt : 1)), (unsigned long)p95, (unsigned long)b_rmax, (unsigned long)b_missed, (unsigned long)b_late,
                             (int)(b_r * 10 / (b_cnt ? b_cnt : 1)), (int)(b_h * 10 / (b_cnt ? b_cnt : 1)));
                std::sprintf(l2, "VLEVELS 60=%lu 30=%lu 20=%lu 15=%lu 12=%lu 10=%lu fixed=%d lat=%d hz_x100=%lu",
                             (unsigned long)b_lvl[1], (unsigned long)b_lvl[2], (unsigned long)b_lvl[3], (unsigned long)b_lvl[4], (unsigned long)b_lvl[5], (unsigned long)b_lvl[6],
                             (int)fixed, atari_opt.vsync_lat, (unsigned long)g_vsync_hz_x100);
                std::sprintf(l3, "CADENCE vbls=%lu shown=%lu drawn=%d hold1=%lu hold2=%lu hold3=%lu hold4=%lu hold5=%lu hold6=%lu hold7=%lu hold8+=%lu",
                             (unsigned long)c.vbls, (unsigned long)c.shown, BENCH_N, (unsigned long)c.hold[1], (unsigned long)c.hold[2], (unsigned long)c.hold[3],
                             (unsigned long)c.hold[4], (unsigned long)c.hold[5], (unsigned long)c.hold[6], (unsigned long)c.hold[7], (unsigned long)c.hold[8]);
                printf("%s\r\n%s\r\n%s\r\n%s\r\n", l1, l2, l3, hw);
#ifdef STWRITE_COUNT
                { extern uint32_t g_st_prev, g_st_blk16, g_st_runs, g_st_uni, g_st_tiles;
                  char lz[256]; std::sprintf(lz, "STUDY prev_pic_changed_bytes=%lu blk16_bytes=%lu runs=%lu uniform_rows=%lu tiles8x8_changed=%lu_of_1120 (per picture)",
                      (unsigned long)(g_st_prev * 4 / renders), (unsigned long)(g_st_blk16 * 16 / renders), (unsigned long)(g_st_runs / renders), (unsigned long)(g_st_uni / renders), (unsigned long)(g_st_tiles / renders));
                  printf("%s\r\n", lz); }
                extern uint32_t g_st_longs, g_st_rows, g_st_words;
                printf("STWRITE bytes_per_pic=%lu changed_word_bytes_per_pic=%lu rows_per_pic=%lu\r\n", (unsigned long)(g_st_longs * 4 / renders), (unsigned long)(g_st_words * 2 / renders), (unsigned long)(g_st_rows / renders));
#endif
                Dcreate("JOURNAUX");
                FILE* bl = fopen("JOURNAUX\\CBBENCH.LOG", "a");
                if (bl) { fprintf(bl, "%s\r\n%s\r\n%s\r\n%s\r\n", l1, l2, l3, hw); fclose(bl); }
#ifdef STAGEPROF
                {
                    // us per picture = sum * 26.04 / pictures; printed x10 (0.1 ms units = sum * 260 / n / 1000)
                    char ls[320], lf[200]; unsigned long fl[4];
                    const unsigned long n = b_cnt ? b_cnt : 1;
                    #define SMS(k) (unsigned long)((uint64_t)g_stage[k] * 2604 / 10000 / n)
                    std::sprintf(ls, "STAGES ms_x10 per picture: cover=%lu tilevals=%lu sky=%lu tiles=%lu road=%lu sprites=%lu text=%lu | convert=%lu draw_frame=%lu (n=%lu)",
                                 SMS(0), SMS(1), SMS(2), SMS(3), SMS(4), SMS(5), SMS(6), SMS(7), SMS(8), n);
                    #undef SMS
                    stage_floors(video.pixels, fl);
                    // 10 pictures each: ms_x10 per picture = units * 26.04 / 10 / 100
                    std::sprintf(lf, "FLOORS ms_x10 per full picture: write_ST=%lu write_fast=%lu convert_ST=%lu convert_fast=%lu",
                                 (unsigned long)((uint64_t)fl[0] * 2604 / 10000 / 10), (unsigned long)((uint64_t)fl[1] * 2604 / 10000 / 10),
                                 (unsigned long)((uint64_t)fl[2] * 2604 / 10000 / 10), (unsigned long)((uint64_t)fl[3] * 2604 / 10000 / 10));
                    extern uint32_t g_isr_calls, g_isr_steps, g_isr_time, g_isr_max;
                    char li[200];
                    std::sprintf(li, "SOUNDISR calls=%lu steps=%lu time_ms=%lu max_ms_x10=%lu over_s=%lu silent_frames=%lu fm_on_dsp_at_end=%d (expected ~60 calls/s, 30 steps/s)",
                                 (unsigned long)g_isr_calls, (unsigned long)g_isr_steps, (unsigned long)((uint64_t)g_isr_time * 2604 / 100000),
                                 (unsigned long)((uint64_t)g_isr_max * 2604 / 10000), (unsigned long)(dt / 200), (unsigned long)g_sil_frames_get(), g_fmdsp_active());
                    printf("%s\r\n%s\r\n%s\r\n", ls, lf, li);
                    FILE* sf = fopen("JOURNAUX\\CBBENCH.LOG", "a");
                    if (sf) { fprintf(sf, "%s\r\n%s\r\n%s\r\n", ls, lf, li); fclose(sf); }
                }
#endif
                fflush(stdout);
                quit_func(0);
            }
#endif
        }

        if (fixed) continue;
        // Level choice. cost(k) = picture + k halves. Sustained, it must fit k VBLs (otherwise the
        // game falls behind); a single picture may take up to k + vsync_lat VBLs (it is then
        // still shown on time). After a change, the backlog of the old level is not held against
        // the new one for 8 pictures (grace).
        if (grace > 0) { grace--; miss_hist = 0; }
        const int missed16 = __builtin_popcount(miss_hist & 0xFFFF);
        if (R + (uint32_t)n * Lh > (uint32_t)n * vbl_fine || missed16 >= 2)
        {
            if (++down_votes >= 6 || missed16 >= 2)
            {
                if (n < N_MAX) n++;
                down_votes = up_count = 0;
                miss_hist = 0;
                grace = 8;
            }
        }
        else down_votes = 0;
        const uint32_t k = (uint32_t)(n - 1);
        if (n > n_min
            && (R + k * Lh) * 100 <= 90u * k * vbl_fine                                   // average: 10 % to spare
            && R_peak + k * Lh_peak <= (k + (uint32_t)atari_opt.vsync_lat) * vbl_fine)    // peaks still on time
        {
            up_count += n;                  // VBLs spent with room for the faster level
            if (up_count >= 180) { n--; up_count = down_votes = 0; miss_hist = 0; grace = 8; }
        }
        else up_count = 0;
        g_vrate_level = n;
    }

    if (g_stpump_on) { stpump_present_pending(); g_stpump_on = false; }
    g_split = false;
    quit_cleanup();
}
#endif
#else
static void main_loop()
{
    Timer frame_time;
    volatile uint32_t* hz200 = (volatile uint32_t*)0x4BA;
    uint32_t t_start = *hz200, t_tick = 0, t_video = 0, t_audio = 0;
    int nframes = 0;

    while (state != STATE_QUIT)
    {
        frame_time.start();

        uint32_t a = *hz200;
        tick();
        uint32_t b = *hz200;

        video.prepare_frame();
        video.render_frame();
        uint32_t c = *hz200;

        audio.tick();
        uint32_t d = *hz200;
        static int frames_total = 0;
        frames_total++;
        if (frames_total == 10 || frames_total == 40 || (frames_total % 25) == 0)
            PERF_PRINTF("MARK frame=%d hz200=%lu state=%d pos=%ld speed=%ld\r\n", frames_total, (unsigned long)d, (int)outrun.game_state, (long)(oroad.road_pos >> 16), (long)(oinitengine.car_increment >> 16));
        t_tick += b - a; t_video += c - b; t_audio += d - c;
        if (++nframes == 5)
        {
            PERF_PRINTF("PERF 5 frames: total=%lu tick=%lu video=%lu audio=%lu (units 5ms)\r\n",
                   (unsigned long)(d - t_start), (unsigned long)t_tick,
                   (unsigned long)t_video, (unsigned long)t_audio);
            nframes = 0; t_start = *hz200; t_tick = t_video = t_audio = 0;
        }

        // Frame pacing: wait for the next VBL rather than an SDL_Delay()
        // ms estimate. On a PAL Mega STE this is ~50Hz; the engine's own
        // config.set_fps()/tick_frame halving logic (above) already
        // divides that down to the original arcade's 30/60fps split, the
        // same way the SDL build divides its own display's refresh rate.
        // NOTE: this ties frame rate to the display refresh rate with no
        // slack for a slow frame - if hwroad/hwtiles/hwsprites + the
        // atari/video.cpp reduction pipeline take longer than one VBL,
        // this will simply run at a fraction of 50Hz rather than dropping
        // frames gracefully. See top-level report's performance risk note.
#ifdef PLATFORM_FALCON
        frame_pace();
#else
        vbl_wait();
#endif
    }

    quit_func(0);
}
#endif // PLATFORM_FALCON

int main(int argc, char* argv[])
{
    (void)argc; (void)argv; // no command-line parsing

    // This "mint" target binary starts in user mode; direct hardware register access
    // (video shifter, DMA sound, ...) needs supervisor mode. The returned value is the user-mode
    // stack pointer, saved so quit_func() can switch back before calling exit() - see there.
    g_user_ssp = Super(0L);
    // Test aid (-DSTARTUP_DEBUG): print a checkpoint after each startup step below,
    // so a crash during boot (e.g. a linker/multilib mismatch - see README_ATARI.md,
    // LINKCPU) can be narrowed down to the step that never printed its checkpoint.
#ifdef STARTUP_DEBUG
    printf("DEBUG A: after Super()%c%c", 13, 10);
#endif
    // Test aid (-DEARLY_RETURN_TEST): return immediately, before any engine code
    // runs, to prove whether a crash happens in this program's own code at all or
    // earlier, in the C-runtime startup (crt0) that calls main() - see README_ATARI.md.
#ifdef EARLY_RETURN_TEST
    printf("DEBUG EARLY: reached main, returning now%c%c", 13, 10);
    Cconin();
    return 0;
#endif

    // Mega STE only: CPU speed/cache control register (bit0 = 16MHz, bit1 = cache).
    // The register does not exist on other machines, so check the machine cookie first.
    long mch = 0;
    Getcookie(C__MCH, &mch);
    if (mch == 0x00010010L)
        *(volatile uint8_t*)0xFF8E21 = 3;
#ifdef STARTUP_DEBUG
    printf("DEBUG B: after Getcookie, mch=%lx%c%c", (unsigned long)mch, 13, 10);
#endif

    config.load(); // hardcoded defaults, no XML (see frontend/config.cpp)
#ifdef STARTUP_DEBUG
    printf("DEBUG C: after config.load()%c%c", 13, 10);
#endif
    hw_detect();                        // CPU, CT60, RAM, measured clock (atari/hwdetect.hpp)
    atari_load_options("outrun.ini");   // optional: shadows, scenery, cadence, sound (see atari/options.hpp)
    if (atari_opt.rompath[0])
    {
        // rompath = . : the ROMs sit next to the program; otherwise the folder given, with a separator.
        std::string rp = atari_opt.rompath;
        if (rp == ".") { rp = ""; config.data.res_path = ""; }   // res\*.bin next to the program too
        else if (rp[rp.size() - 1] != '\\' && rp[rp.size() - 1] != '/') rp += "\\";
        config.data.rom_path = rp;
    }
    if (atari_opt.mod)
    {
        void atari_mod_probe();   // atari/modplayer.cpp
        atari_mod_probe();        // the Music\TRACKn.MOD files are looked for now, not when a tune starts
    }
#ifdef STARTUP_DEBUG
    printf("DEBUG D: after atari_load_options%c%c", 13, 10);
#endif

    if (!roms.load_revb_roms(config.sound.fix_samples))
    {
        printf("Could not load the OutRun ROM set from roms\\ (see README_ATARI.md).\r\nPress a key.\r\n");
        Cconin();
        quit_func(1);
        return 0;
    }

    config.set_fps(config.video.fps);
    if (!video.init(&roms, &config.video))
        quit_func(1);

    audio.init();

    input.init(config.controls.pad_id,
               config.controls.keyconfig, config.controls.padconfig,
               config.controls.analog,    config.controls.axis,
               config.controls.invert,    config.controls.asettings);

#ifdef MUSIC_RENDER
    // Offline tool (-DMUSIC_RENDER -DMUSIC_ID=0x85 -DMUSIC_SECONDS=100): plays one sound command
    // through the sound driver without the game.
    //   default:            writes what the FM and PCM chips produce to music_ym.raw / music_pcm.raw
    //                       (mono, signed 8 bit, at the mixing rate)
    //   -DMUSIC_LOGONLY:    no synthesis (fast); only the FM register writes are logged to music_log.bin
    extern FILE* g_ym_log;
    extern int g_render_tick;
    {
        outrun.init();                          // boots the sound driver
        osoundint.has_booted = true;            // (normally set when the attract mode starts)
#ifdef MUSIC_LOGONLY
#ifdef MUSIC_LOGNAME   // -DMUSIC_LOGNAME=MAGICAL.LOG (no quotes: they do not survive build_atari.sh)
#define MUSIC_STR2(x) #x
#define MUSIC_STR(x) MUSIC_STR2(x)
        g_ym_log = fopen(MUSIC_STR(MUSIC_LOGNAME), "wb");
#else
        g_ym_log = fopen("music_log.bin", "wb");
#endif
        FILE* fy = 0; FILE* fp = 0;
#else
        FILE* fy = fopen("music_ym.raw", "wb");
        FILE* fp = fopen("music_pcm.raw", "wb");
#endif
        const int TICKS = MUSIC_SECONDS * config.fps;
        double sum_y = 0, sum_p = 0;
        uint32_t n = 0;
        for (int t = 0; t < TICKS; t++)
        {
            g_render_tick = t;
            if (t == 30) osoundint.queue_sound_service(MUSIC_ID);
            osoundint.tick();
#ifdef MUSIC_LOGONLY
            osoundint.pcm->stream_update();     // sample-end flags are read by the driver
            osoundint.ym->skip_frame();
#else
            osoundint.pcm->stream_update();
            osoundint.ym->stream_update();
            const int16_t* yb = osoundint.ym->get_buffer();
            const int16_t* pb = osoundint.pcm->get_buffer();
            const uint32_t frames = osoundint.ym->buffer_size / 2;
            for (uint32_t i = 0; i < frames; i++)
            {
                int32_t y = (yb[2 * i] + yb[2 * i + 1]) / 2;
                int32_t p = (pb[2 * i] + pb[2 * i + 1]) / 2;
                sum_y += (double)y * y;  sum_p += (double)p * p;  n++;
                fputc((int8_t)(y >> 8), fy);
                fputc((int8_t)(p >> 8), fp);
            }
#endif
            if ((t % 300) == 0) { printf("tick %d%c%c", t, 13, 10); }
        }
        if (fy) fclose(fy);
        if (fp) fclose(fp);
        if (g_ym_log) fclose(g_ym_log);
        printf("RENDERED frames=%lu rms_ym=%ld rms_pcm=%ld%c%c", (unsigned long)n,
               (long)(n ? __builtin_sqrt(sum_y / n) : 0), (long)(n ? __builtin_sqrt(sum_p / n) : 0), 13, 10);
        quit_func(0);
        return 0;
    }
#endif

#ifdef AUTOPLAY
    state = STATE_INIT_GAME; // test runs: straight into the game
#else
    state = STATE_INIT_MENU; // the options menu first (atari/optmenu.hpp)
#endif

#ifdef PLATFORM_FALCON
    // Picture pacing on the VBL (atari/vsync.hpp, outrun.ini vsync): on a ~60 Hz display by default.
    g_vsync_hz_x100 = 0;
    if (atari_opt.vsync && vsync_install())
    {
        g_vsync_hz_x100 = vsync_measure_hz_x100();
        // automatic: a ~60 Hz display and a 68040/68060 (a 16 MHz 68030 draws a picture in about
        // a second: nothing to gain, the v0.39 loop stays)
        g_vsync_on = atari_opt.vsync == 1 || (g_hw.cpu >= 40 && g_vsync_hz_x100 >= 5500 && g_vsync_hz_x100 <= 6500);
        if (!g_vsync_on) vsync_remove();
    }
    g_vsync_idle = vsync_idle;
#ifndef OLD_PACING
    if (g_vsync_on) main_loop_vsync(); else
#endif
#endif
    main_loop();
    // main_loop() already ran quit_cleanup() (stops the sound DMA, restores the keyboard vector,
    // puts the video mode back) right before returning here - see the STATE_QUIT handling in
    // tick()/main_loop(). A plain return, not quit_func()/Pterm(): see quit_func()'s own comment
    // for why - this is what actually gets back to a working desktop when launched, as this
    // program always is, from the AUTO folder, before EmuTOS's own desktop/AES exists yet.
    //
    // With MiNT running the program was started from a multitasking AES (XaAES, N.AES), which
    // does not redraw the desktop by itself when a program that took the screen over returns:
    // ask it to (see atari/gemredraw.cpp). AES calls belong in user mode, so supervisor mode is
    // left first; this is only done under MiNT, where leaving it is safe - the plain TOS/EmuTOS
    // path above stays exactly as it was.
    {
        long mint = 0;
        if (Getcookie(C_MiNT, &mint) == 0)
        {
            SuperToUser((void*)g_user_ssp);
            atari_redraw_desktop();
        }
    }
    return 0;
}
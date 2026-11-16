// STOVL.TOS - CT60 micro-benchmark: can the 68060 compute while its writes to ST-RAM go
// through the Falcon bus? (the hypothesis behind the ST write pump, src/main/atari/stpump.hpp)
//
// Every test writes the same 18 000 longs (about one Cannonball picture) to an ST-RAM buffer
// and/or runs the same amount of "work" (a block of register ALU instructions + fast-RAM
// reads/writes like a sprite loop), timed with the 200 Hz counter + Timer C (26 us):
//   A  ST writes alone (move.l loop, one burst)
//   B  work alone
//   C  work with one ST long written after each block          (ideal spread)
//   D  work with the real pump (atari_st_pump, K longs every K blocks), K = 1, 2, 4, 8
//   E  the same 72 KB copied Fast -> ST with MOVE16 bursts (16 bytes each), alone
//   F  work with one MOVE16 burst (4 longs) after every 4 blocks          (hybrid: bursts spread)
//   G  the game's real conversion (atari_truecolor_diff, 224 rows, half the pairs changed so
//      that ~72 KB are written) into ST-RAM, and H the same into fast RAM: G - H is the ST
//      share of the real conversion loop on this machine.
// overlap % = (A + B - X) / min(A, B): 100 = the ST time is completely hidden, 0 = none.
// Results to JOURNAUX\STOVL.LOG; argument AUTO = no key at the end. 68060 code: refuses to run
// below a 68040 (_CPU cookie).  Build: tools/ct60/build_stovl.sh
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <mint/osbind.h>
#include <mint/cookie.h>
#include "atari/stpump.hpp"

extern "C" void atari_copy16(void* dst, const void* src, uint32_t lines);
extern "C" void atari_truecolor_diff(const uint16_t* px, uint16_t* shadow, const uint16_t* pal, uint16_t* dst, uint32_t longs);
static uint32_t* st_al;                      // st_buf, 16-byte aligned (MOVE16)
static uint32_t* fast_img;                   // 72 KB source for MOVE16, 16-byte aligned
static uint16_t* idx;                        // 320x224 colour indices
static uint16_t* shadow_ref;                 // what the "screen" shows: half the pairs differ
static uint16_t* shadow;                     // working copy, reset before each run
static uint16_t* pal;                        // 8192 RGB565 entries
static uint16_t* st_screen;                  // 320x224x2 in ST-RAM
static uint16_t* fast_screen;                // same in fast RAM

static const int NL = 18000;                 // longs per "picture"
static volatile uint32_t* st_buf;            // ST-RAM, 72 KB
static uint32_t* fast_src;                   // fast RAM, work data
static uint32_t* fast_dst;
static uint32_t* job;                        // pump list, 2 longs per long
static int g_test, g_k;
static uint32_t g_t[16];
static uint32_t g_sink;

static inline uint32_t fine_now()
{
    volatile uint32_t* hz200 = (volatile uint32_t*)0x4BA;
    volatile uint8_t* tcdr = (volatile uint8_t*)0xFFFFFA23L;
    uint32_t h1, h2, c;
    do { h1 = *hz200; c = *tcdr; h2 = *hz200; } while (h1 != h2);
    if (c == 0 || c > 192) c = 192;
    return h1 * 192 + (192 - c);
}

// One block of work: 16 fast-RAM longs read, combined, written (like a sprite row).
static inline void work_block(int i)
{
    const uint32_t* s = fast_src + ((i * 16) & 0x3fff);
    uint32_t* d = fast_dst + ((i * 16) & 0x3fff);
    uint32_t a = g_sink;
    for (int j = 0; j < 16; j++) { uint32_t v = s[j]; a = (a + v) ^ (v >> 3); d[j] = a; }
    g_sink = a;
}

static long run_super()
{
    uint32_t t0, t1;
    switch (g_test)
    {
    case 0:   // A
        t0 = fine_now();
        for (int i = 0; i < NL; i++) st_buf[i] = (uint32_t)i;
        t1 = fine_now(); break;
    case 1:   // B
        t0 = fine_now();
        for (int i = 0; i < NL; i++) work_block(i);
        t1 = fine_now(); break;
    case 2:   // C
        t0 = fine_now();
        for (int i = 0; i < NL; i++) { work_block(i); st_buf[i] = (uint32_t)i; }
        t1 = fine_now(); break;
    case 4:   // E
        t0 = fine_now();
        atari_copy16(st_al, fast_img, NL * 4 / 16);
        t1 = fine_now(); break;
    case 5:   // F
        t0 = fine_now();
        for (int i = 0; i < NL; i++) { work_block(i); if ((i & 3) == 3) atari_copy16(st_al + (i - 3), fast_img + (i - 3), 1); }
        t1 = fine_now(); break;
    case 6:   // G
    case 7:   // H
    {
        uint16_t* dst = g_test == 6 ? st_screen : fast_screen;
        std::memcpy(shadow, shadow_ref, 320 * 224 * 2);
        t0 = fine_now();
        for (int r = 0; r < 224; r++) atari_truecolor_diff(idx + r * 320, shadow + r * 320, pal, dst + r * 320, 160);
        t1 = fine_now(); break;
    }
    default:  // D
    {
        for (int i = 0; i < NL; i++) { job[2 * i] = (uint32_t)(st_buf + i); job[2 * i + 1] = (uint32_t)i; }
        g_pump_rd = job; g_pump_end = job + 2 * NL; g_pump_k = (uint32_t)(g_k - 1);
        t0 = fine_now();
        for (int i = 0; i < NL; i++) { work_block(i); if (i % g_k == g_k - 1) st_pump(); }
        atari_st_flush();
        t1 = fine_now(); break;
    }
    }
    g_t[g_test] = t1 - t0;
    return 0;
}

static uint32_t measure(int test, int k)
{
    uint32_t best = 0xffffffffu;
    for (int r = 0; r < 5; r++) { g_test = test; g_k = k; Supexec(run_super); if (g_t[test] < best) best = g_t[test]; }
    return best;   // 26.04 us units
}

int main(int argc, char** argv)
{
    const bool automatic = argc > 1 && std::strcmp(argv[1], "AUTO") == 0;
    long cpu = 0;
    Getcookie(C__CPU, &cpu);
    Dcreate("JOURNAUX");
    FILE* f = std::fopen("JOURNAUX\\STOVL.LOG", "a");
    if (cpu < 40)
    {
        std::printf("STOVL needs a 68040/68060 (_CPU=%ld)\r\n", cpu);
        if (f) { std::fprintf(f, "STOVL refused: _CPU=%ld\r\n", cpu); std::fclose(f); }
        return 1;
    }
    st_buf = (volatile uint32_t*)Mxalloc(NL * 4 + 16, 0);
    fast_src = (uint32_t*)Mxalloc(0x4000 * 4 + 64, 3);
    fast_dst = (uint32_t*)Mxalloc(0x4000 * 4 + 64, 3);
    job = (uint32_t*)Mxalloc(NL * 8 + 16, 3);
    fast_img = (uint32_t*)Mxalloc(NL * 4 + 16, 3);
    idx = (uint16_t*)Mxalloc(320 * 224 * 2, 3);
    shadow_ref = (uint16_t*)Mxalloc(320 * 224 * 2, 3);
    shadow = (uint16_t*)Mxalloc(320 * 224 * 2, 3);
    pal = (uint16_t*)Mxalloc(8192 * 2, 3);
    st_screen = (uint16_t*)Mxalloc(320 * 224 * 2, 0);
    fast_screen = (uint16_t*)Mxalloc(320 * 224 * 2, 3);
    if (!st_buf || !fast_src || !fast_dst || !job || !fast_img || !idx || !shadow_ref || !shadow || !pal || !st_screen || !fast_screen
        || (long)st_buf < 0 || (long)st_screen < 0) { if (f) { std::fprintf(f, "STOVL: no memory\r\n"); std::fclose(f); } return 2; }
    st_al = (uint32_t*)(((uint32_t)st_buf + 15) & ~15u);
    fast_img = (uint32_t*)(((uint32_t)fast_img + 15) & ~15u);
    for (int i = 0; i < NL; i++) fast_img[i] = (uint32_t)i * 40503u;
    for (int i = 0; i < 8192; i++) pal[i] = (uint16_t)(i * 7);
    for (int i = 0; i < 320 * 224; i++) idx[i] = (uint16_t)(((uint32_t)i * 2654435761u >> 19) & 0x1fff);
    for (int i = 0; i < 320 * 224; i++) shadow_ref[i] = ((i >> 1) & 1) ? (uint16_t)(idx[i] ^ 1) : idx[i];   // every other pair differs
    for (int i = 0; i < 0x4000; i++) fast_src[i] = (uint32_t)i * 2654435761u;
    char l[160];
    const uint32_t A = measure(0, 1), B = measure(1, 1), C = measure(2, 1);
    std::sprintf(l, "STOVL cpu=%ld st_buf=%08lx A_st_only_us=%lu B_work_only_us=%lu C_spread_us=%lu",
                 cpu, (unsigned long)st_buf, (unsigned long)(A * 2604 / 100), (unsigned long)(B * 2604 / 100), (unsigned long)(C * 2604 / 100));
    std::printf("%s\r\n", l); if (f) std::fprintf(f, "%s\r\n", l);
    const uint32_t mn = A < B ? A : B;
    std::sprintf(l, "STOVL overlap_C_pct=%ld", (long)(((long)A + (long)B - (long)C) * 100 / (long)(mn ? mn : 1)));
    std::printf("%s\r\n", l); if (f) std::fprintf(f, "%s\r\n", l);
    static const int KS[4] = { 1, 2, 4, 8 };
    for (int q = 0; q < 4; q++)
    {
        const uint32_t D = measure(3, KS[q]);
        std::sprintf(l, "STOVL pump_k=%d D_us=%lu overlap_pct=%ld", KS[q], (unsigned long)(D * 2604 / 100),
                     (long)(((long)A + (long)B - (long)D) * 100 / (long)(mn ? mn : 1)));
        std::printf("%s\r\n", l); if (f) std::fprintf(f, "%s\r\n", l);
    }
    const uint32_t E = measure(4, 1), F = measure(5, 1);
    std::sprintf(l, "STOVL move16_E_us=%lu move16_spread_F_us=%lu overlap_F_pct=%ld", (unsigned long)(E * 2604 / 100), (unsigned long)(F * 2604 / 100),
                 (long)(((long)E + (long)B - (long)F) * 100 / (long)((E < B ? E : B) ? (E < B ? E : B) : 1)));
    std::printf("%s\r\n", l); if (f) std::fprintf(f, "%s\r\n", l);
    const uint32_t G = measure(6, 1), H = measure(7, 1);
    std::sprintf(l, "STOVL conv_st_G_us=%lu conv_fast_H_us=%lu st_share_us=%ld", (unsigned long)(G * 2604 / 100), (unsigned long)(H * 2604 / 100),
                 (long)(((long)G - (long)H) * 2604 / 100));
    std::printf("%s\r\n", l); if (f) std::fprintf(f, "%s\r\n", l);
    if (f) std::fclose(f);
    if (!automatic) { std::printf("Press a key.\r\n"); Cconin(); }
    return 0;
}

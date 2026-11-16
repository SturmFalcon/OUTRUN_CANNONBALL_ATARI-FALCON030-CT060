| ST-RAM write pump (see stpump.hpp). 68000 code apart from nothing CPU specific.

	.data
	.even
	.globl	_g_pump_rd
	.globl	_g_pump_end
	.globl	_g_pump_k
_g_pump_rd:	.long	0
_g_pump_end:	.long	0
_g_pump_k:	.long	3

	.text
	.even
| void atari_st_pump(void): writes up to g_pump_k + 1 listed longs. Every register preserved,
| so that assembler loops can call it with their registers live.
	.globl	_atari_st_pump
_atari_st_pump:
	movem.l	%d0/%a0-%a2,-(%sp)
	move.l	_g_pump_rd,%a0
	move.l	_g_pump_end,%a2
	move.l	_g_pump_k,%d0
1:	cmpa.l	%a2,%a0
	bhs.s	2f
	move.l	(%a0)+,%a1
	move.l	(%a0)+,(%a1)
	dbra	%d0,1b
2:	move.l	%a0,_g_pump_rd
	movem.l	(%sp)+,%d0/%a0-%a2
	rts

| void atari_st_flush(void): writes everything still listed.
	.globl	_atari_st_flush
_atari_st_flush:
	move.l	%a2,-(%sp)
	move.l	_g_pump_rd,%a0
	move.l	_g_pump_end,%a2
	bra.s	2f
1:	move.l	(%a0)+,%a1
	move.l	(%a0)+,(%a1)
2:	cmpa.l	%a2,%a0
	blo.s	1b
	move.l	%a0,_g_pump_rd
	move.l	(%sp)+,%a2
	rts

#if defined(__mc68020__) || defined(__mc68030__) || defined(__mc68040__) || defined(__mc68060__)
| uint32_t* atari_truecolor_stage(const uint16_t* px, uint16_t* shadow, const uint16_t* pal,
|                                 uint16_t* dst, uint32_t longs, uint32_t* wr)
| As atari_truecolor_diff (truecolor_asm.S), but each changed pair is listed at wr as
| (screen address, RGB565 pair) instead of being written. Returns the new end of the list.
	.globl	_atari_truecolor_stage
_atari_truecolor_stage:
	movem.l	%d2/%a2-%a4,-(%sp)		| 4 regs = 16 bytes
	move.l	20(%sp),%a0			| px
	move.l	24(%sp),%a3			| shadow
	move.l	28(%sp),%a1			| pal
	move.l	32(%sp),%a2			| dst
	move.l	36(%sp),%d1
	move.l	40(%sp),%a4			| wr
	subq.l	#1,%d1
ts_loop:
	move.l	(%a0)+,%d0
	cmp.l	(%a3)+,%d0
	beq.s	ts_same
	move.l	%d0,-4(%a3)
	move.w	%d0,%d2
	swap	%d0
	move.w	(%a1,%d0.w*2),%d0
	swap	%d0
	move.w	(%a1,%d2.w*2),%d0
	move.l	%a2,(%a4)+
	move.l	%d0,(%a4)+
ts_same:
	addq.l	#4,%a2
	dbra	%d1,ts_loop
	move.l	%a4,%d0
	movem.l	(%sp)+,%d2/%a2-%a4
	rts
#endif

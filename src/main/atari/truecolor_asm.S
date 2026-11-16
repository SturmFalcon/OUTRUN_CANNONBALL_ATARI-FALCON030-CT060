| void atari_truecolor(const uint16_t* px, const uint16_t* pal, uint16_t* dst, uint32_t blocks4)
| dst[i] = pal[px[i]] for 4*blocks4 pixels (blocks4 must be 1..65536).
| px values must be < 0x2000 (word table, byte offset = 2*index).

	.text
	.even
#if defined(__mc68020__) || defined(__mc68030__) || defined(__mc68040__) || defined(__mc68060__)
| 68020+: two pixels per long write, scaled-index table lookups.
	.globl	_atari_truecolor
_atari_truecolor:
	movem.l	%d2-%d4/%a2,-(%sp)		| a2 is callee-saved (4 regs = 16 bytes)
	move.l	20(%sp),%a0
	move.l	24(%sp),%a1
	move.l	28(%sp),%a2
	move.l	32(%sp),%d1
	subq.l	#1,%d1
	moveq	#-1,%d3				| previous pair of indices (never a valid pair)
	moveq	#0,%d4
tc_loop:
	| Flat areas (sky, road stripes, grass) repeat the same pair of indices: reuse the last result.
	move.l	(%a0)+,%d0
	cmp.l	%d3,%d0
	beq.s	tc_same1
	move.l	%d0,%d3
	move.w	%d0,%d2
	swap	%d0
	move.w	(%a1,%d0.w*2),%d0
	swap	%d0
	move.w	(%a1,%d2.w*2),%d0
	move.l	%d0,%d4
tc_same1:
	move.l	%d4,(%a2)+
	move.l	(%a0)+,%d0
	cmp.l	%d3,%d0
	beq.s	tc_same2
	move.l	%d0,%d3
	move.w	%d0,%d2
	swap	%d0
	move.w	(%a1,%d0.w*2),%d0
	swap	%d0
	move.w	(%a1,%d2.w*2),%d0
	move.l	%d0,%d4
tc_same2:
	move.l	%d4,(%a2)+
	subq.l	#1,%d1
	bpl.s	tc_loop
	movem.l	(%sp)+,%d2-%d4/%a2
	rts
#else
	.globl	_atari_truecolor
_atari_truecolor:
	move.l	%a2,-(%sp)			| a2 is callee-saved
	move.l	8(%sp),%a0
	move.l	12(%sp),%a1
	move.l	16(%sp),%a2
	move.l	20(%sp),%d1
	subq.l	#1,%d1
tc_loop:
	move.w	(%a0)+,%d0
	add.w	%d0,%d0
	move.w	0(%a1,%d0.w),(%a2)+
	move.w	(%a0)+,%d0
	add.w	%d0,%d0
	move.w	0(%a1,%d0.w),(%a2)+
	move.w	(%a0)+,%d0
	add.w	%d0,%d0
	move.w	0(%a1,%d0.w),(%a2)+
	move.w	(%a0)+,%d0
	add.w	%d0,%d0
	move.w	0(%a1,%d0.w),(%a2)+
	dbra	%d1,tc_loop
	move.l	(%sp)+,%a2
	rts
#endif

| void atari_truecolor_x2(const uint16_t* px, const uint16_t* pal, uint16_t* dst, uint32_t rows)
| Half-resolution frame -> 16-bit screen: each source row of 160 pixels becomes
| two identical rows of 320 pixels (2x2 pixel doubling).  rows must be 1..65536.
	.globl	_atari_truecolor_x2
_atari_truecolor_x2:
	movem.l	%d2-%d4/%a2-%a3,-(%sp)		| 5 regs = 20 bytes
	move.l	24(%sp),%a0
	move.l	28(%sp),%a1
	move.l	32(%sp),%a2
	move.l	36(%sp),%d3
	subq.l	#1,%d3
x2_row:
	lea	640(%a2),%a3			| second output line
	move.w	#159,%d4
x2_px:
	move.w	(%a0)+,%d0
	add.w	%d0,%d0
	move.w	0(%a1,%d0.w),%d1
	move.w	%d1,%d2
	swap	%d2
	move.w	%d1,%d2
	move.l	%d2,(%a2)+
	move.l	%d2,(%a3)+
	dbra	%d4,x2_px
	lea	640(%a2),%a2			| a2 is at the start of line 2: skip it
	dbra	%d3,x2_row
	movem.l	(%sp)+,%d2-%d4/%a2-%a3
	rts
| uint32_t atari_rowsync(const uint32_t* src, uint32_t* prev, uint32_t longs)
| Compares src with prev (longs must be 1..65536).  Returns 0 if they are identical;
| otherwise copies src over prev (from the first difference on) and returns 1.
	.globl	_atari_rowsync
_atari_rowsync:
	move.l	4(%sp),%a0
	move.l	8(%sp),%a1
	move.l	12(%sp),%d1
	subq.l	#1,%d1
rs_cmp:
	move.l	(%a0)+,%d0
	cmp.l	(%a1)+,%d0
	dbne	%d1,rs_cmp
	beq.s	rs_same
	subq.l	#4,%a0
	subq.l	#4,%a1
rs_copy:
	move.l	(%a0)+,(%a1)+
	dbra	%d1,rs_copy
	moveq	#1,%d0
	rts
rs_same:
	moveq	#0,%d0
	rts

| uint32_t atari_rowcmp(const uint32_t* a, const uint32_t* b, uint32_t longs)
| Returns 0 if the two blocks are identical, 1 otherwise (longs must be 1..65536).
	.text
	.even
	.globl	_atari_rowcmp
_atari_rowcmp:
	move.l	4(%sp),%a0
	move.l	8(%sp),%a1
	move.l	12(%sp),%d1
	subq.l	#1,%d1
rc_loop:
	cmpm.l	(%a0)+,(%a1)+
	dbne	%d1,rc_loop
	beq.s	rc_same
	moveq	#1,%d0
	rts
rc_same:
	moveq	#0,%d0
	rts

#if defined(MOVE16_DIFF) && (defined(__mc68040__) || defined(__mc68060__))
| void atari_truecolor_diff16(const uint16_t* px, uint16_t* shadow, const uint16_t* pal,
|                             uint16_t* dst, uint32_t blocks, uint16_t* buf16)
| [EXPÉRIMENTAL, -DMOVE16_DIFF] Hybrid of atari_truecolor_diff and MOVE16: the row is taken
| 8 pixels (16 bytes, 4 index longs) at a time; a block whose 4 longs all match the shadow is
| skipped, otherwise the shadow is updated, the 8 pixels are converted into buf16 (fast RAM,
| 16-byte aligned) and pushed to dst with one MOVE16 burst. dst must be 16-byte aligned.
| Writes more bytes than the pair diff (whole 16-byte blocks) but only as bursts: which wins on
| the Falcon's ST bus is for the real CT60 to tell. blocks must be 1..65536.
	.text
	.even
	.globl	_atari_truecolor_diff16
_atari_truecolor_diff16:
	movem.l	%d2-%d5/%a2-%a4,-(%sp)		| 7 regs = 28 bytes
	move.l	32(%sp),%a0			| px
	move.l	36(%sp),%a3			| shadow
	move.l	40(%sp),%a1			| pal
	move.l	44(%sp),%a2			| dst
	move.l	48(%sp),%d1
	move.l	52(%sp),%a4			| buf16
	subq.l	#1,%d1
t16_loop:
	movem.l	(%a0)+,%d2-%d5			| 8 indices
	cmp.l	(%a3),%d2
	bne.s	t16_conv
	cmp.l	4(%a3),%d3
	bne.s	t16_conv
	cmp.l	8(%a3),%d4
	bne.s	t16_conv
	cmp.l	12(%a3),%d5
	bne.s	t16_conv
	lea	16(%a3),%a3
	lea	16(%a2),%a2
	dbra	%d1,t16_loop
	bra.s	t16_done
t16_conv:
	movem.l	%d2-%d5,(%a3)
	lea	16(%a3),%a3
	.irp	r,%d2,%d3,%d4,%d5
	move.w	\r,%d0
	swap	\r
	move.w	(%a1,\r\().w*2),\r
	swap	\r
	move.w	(%a1,%d0.w*2),\r
	move.l	\r,(%a4)+
	.endr
	lea	-16(%a4),%a4
	move16	(%a4)+,(%a2)+
	lea	-16(%a4),%a4
	dbra	%d1,t16_loop
t16_done:
	movem.l	(%sp)+,%d2-%d5/%a2-%a4
	rts
#endif

#if defined(__mc68040__) || defined(__mc68060__)
| void atari_copy16(void* dst, const void* src, uint32_t lines)
| Copies `lines` 16-byte lines with MOVE16 (one burst each); dst and src must be 16-byte aligned.
| 68040/68060 only.  Used to push converted rows to the (slow) ST-RAM screen.
	.text
	.even
	.globl	_atari_copy16
_atari_copy16:
	move.l	4(%sp),%a0
	move.l	8(%sp),%a1
	move.l	12(%sp),%d0
	subq.l	#1,%d0
c16_loop:
	move16	(%a1)+,(%a0)+
	dbra	%d0,c16_loop
	rts
#endif

| void atari_truecolor_diff(const uint16_t* px, uint16_t* shadow, const uint16_t* pal,
|                           uint16_t* dst, uint32_t longs)
| Like atari_truecolor, two pixels at a time, but only where the pair of indices differs from
| `shadow` - what dst shows already (video_falcon.cpp keeps one such copy per screen buffer,
| in fast RAM). Changed pairs are converted, written to dst and copied into shadow; unchanged
| ones cost a fast-RAM compare instead of a screen write. longs must be 1..65536.
#if defined(__mc68040__) || defined(__mc68060__)
| 68040/68060 version: two pairs per turn. When both pairs match the shadow (much of the sky,
| the road and the scenery from one picture to the next) a turn costs two loads, two compares
| and no write; otherwise the changed pair(s) are converted and written as before. Same
| result, word for word, as the one-pair loop below (kept for the 68030).
	.macro	TVCONV reg
	move.l	\reg,-4(%a3)
	move.w	\reg,%d2
	swap	\reg
	move.w	(%a1,\reg\().w*2),\reg
	swap	\reg
	move.w	(%a1,%d2.w*2),\reg
	move.l	\reg,(%a2)+
	.endm
	.text
	.even
	.globl	_atari_truecolor_diff
_atari_truecolor_diff:
	movem.l	%d2-%d3/%a2-%a3,-(%sp)		| 4 regs = 16 bytes
	move.l	20(%sp),%a0			| px
	move.l	24(%sp),%a3			| shadow
	move.l	28(%sp),%a1			| pal
	move.l	32(%sp),%a2			| dst
	move.l	36(%sp),%d1
	lsr.l	#1,%d1				| pairs of pairs; carry = one pair left over
	bcc.s	tw_even
	move.l	(%a0)+,%d0
	cmp.l	(%a3)+,%d0
	beq.s	tw_one_same
	TVCONV	%d0
	bra.s	tw_even
tw_one_same:
	addq.l	#4,%a2
tw_even:
	subq.l	#1,%d1
	bmi.s	tw_done
tw_loop:
	move.l	(%a0)+,%d0
	move.l	(%a0)+,%d3
	cmp.l	(%a3)+,%d0
	bne.s	tw_first
	cmp.l	(%a3)+,%d3
	bne.s	tw_second_only
	addq.l	#8,%a2
	dbra	%d1,tw_loop
	bra.s	tw_done
tw_first:
	TVCONV	%d0
	cmp.l	(%a3)+,%d3
	bne.s	tw_second
	addq.l	#4,%a2
	dbra	%d1,tw_loop
	bra.s	tw_done
tw_second_only:
	addq.l	#4,%a2
tw_second:
	TVCONV	%d3
	dbra	%d1,tw_loop
tw_done:
	movem.l	(%sp)+,%d2-%d3/%a2-%a3
	rts
#else
	.text
	.even
	.globl	_atari_truecolor_diff
_atari_truecolor_diff:
	movem.l	%d2/%a2-%a3,-(%sp)		| 3 regs = 12 bytes
	move.l	16(%sp),%a0			| px
	move.l	20(%sp),%a3			| shadow
	move.l	24(%sp),%a1			| pal
	move.l	28(%sp),%a2			| dst
	move.l	32(%sp),%d1
	subq.l	#1,%d1
tv_loop:
	move.l	(%a0)+,%d0
	cmp.l	(%a3)+,%d0
	beq.s	tv_same
	move.l	%d0,-4(%a3)
	move.w	%d0,%d2
	swap	%d0
#if defined(__mc68020__) || defined(__mc68030__) || defined(__mc68040__) || defined(__mc68060__)
	move.w	(%a1,%d0.w*2),%d0
	swap	%d0
	move.w	(%a1,%d2.w*2),%d0
#else
	add.w	%d0,%d0
	move.w	0(%a1,%d0.w),%d0
	swap	%d0
	add.w	%d2,%d2
	move.w	0(%a1,%d2.w),%d0
#endif
	move.l	%d0,(%a2)+
	dbra	%d1,tv_loop
	movem.l	(%sp)+,%d2/%a2-%a3
	rts
tv_same:
	addq.l	#4,%a2
	dbra	%d1,tv_loop
	movem.l	(%sp)+,%d2/%a2-%a3
	rts
#endif

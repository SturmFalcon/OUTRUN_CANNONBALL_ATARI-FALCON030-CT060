| Timing loop for the CPU clock measurement (see hwdetect.cpp).
| C prototype (extern "C"):
|   void atari_hw_spin(uint32_t iterations);   iterations >= 1
| Each iteration is 16 dependent "add.l d1,d1" (each needs the result of the one before, so
| no two of them can run together, whatever the CPU) then subq/bne. Expected cost per
| iteration, from the Motorola manuals, code in the instruction cache:
|   68060: 16 cycles (one add per cycle; subq pairs with an add; the predicted bne is folded)
|   68040: about 18, 68030: about 40.
| Plain 68000 code, so the same object works in every build.

	.text
	.even
	.globl	_atari_hw_spin
_atari_hw_spin:
	move.l	4(%sp),%d0
	moveq	#1,%d1
	.balign	16
spin_loop:
	add.l	%d1,%d1
	add.l	%d1,%d1
	add.l	%d1,%d1
	add.l	%d1,%d1
	add.l	%d1,%d1
	add.l	%d1,%d1
	add.l	%d1,%d1
	add.l	%d1,%d1
	add.l	%d1,%d1
	add.l	%d1,%d1
	add.l	%d1,%d1
	add.l	%d1,%d1
	add.l	%d1,%d1
	add.l	%d1,%d1
	add.l	%d1,%d1
	add.l	%d1,%d1
	subq.l	#1,%d0
	bne.s	spin_loop
	rts

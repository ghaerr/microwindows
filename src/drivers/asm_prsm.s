| Routines to interface C to the graphics card in the Prisma
| These replace the ROM routines.
|
| 0.1  16/02/99  G.Harvey	first created
| 0.11 19/02/99  G.Harvey	optimise line drawing (c_ldraw4())
| 0.2  20/02/99  G.Harvey	add read/write pixel
| 0.3  21/02/99  G.Harvey	add video init routines
| 0.4  23/02/99  G.Harvey	add r1pix
| 0.41 26/02/99  G.Harvey	add w1pix
| 0.5  27/02/99  G.Harvey	add rd_rect, wr_rect
| 0.51 02/03/99  G.Harvey	add wr_rect5

| Base addr of GC video buffer control registers

GC_BASE		= 0x200440

| GC Video Buffer write registers (offsets from the base addr)

GCW_CE		= 0x00	| chip enable
GCW_PSEL	= 0x04	| plane select
GCW_INTEN	= 0x08	| intensity write data
GCW_CMD		= 0x0c	| command
GCW_CMAP	= 0x10	| colour map address / data
GCW_DAC		= 0x14	| DAC / video control
GCW_PY_SM	= 0x18	| pixel Y address / start single mem cycle
GCW_IN_SM	= 0x1c	| intensity write data / start single mem cycle
GCW_PX_SL	= 0x20	| pixel X address / start line
GCW_PX_SM	= 0x24	| pixel X address / start single mem cycle
GCW_PY		= 0x28	| pixel Y address
GCW_PX		= 0x2c	| pixel X address
GCW_LG1		= 0x30	| line generation algorithm
GCW_LG2		= 0x34	| line generation algorithm
GCW_LG3		= 0x38	| line generation algorithm
GCW_LG4		= 0x3c	| line generation algorithm

| GC Video Buffer read registers (offsets from the base addr)

GCR_INTEN	= 0x00	| read intensity
GCR_RESV	= 0x04	| reserved
GCR_CMAP	= 0x08	| read colour map
GCR_STATUS	= 0x0c	| read VBUSY, VBLANK status
GCR_IN_SM	= 0x10	| read intensity / start single mem cycle

	.text

| Video init 1
| from LAB_0367 at 0x00104760

	.even
.globl	init_scr
init_scr:
init1:
	movel	d0,sp@-
	movel	a0,sp@-
	lea	GC_BASE,a0	| base of video control registers
	clrl	a0@(GCW_LG3)
	tstl	a0@(524)	| 0x20064c - CMD reg but inhibit command
init1_1:
	btst	#6,a0@(524)	| GCR_STATUS with command inhibit
	beq	init1_1		| wait for VBLANK ? 
init1_2:
	btst	#5,a0@(524)	| GCR_STATUS
	bne	init1_2		| VBLANK ?
	clrl	a0@(GCW_CMAP)	| colour map
	moveq	#0x17,d0
	clrl	a0@(GCW_CMAP)
	movel	d0,a0@(GCW_DAC)
	moveq	#0x1b,d0
	clrl	a0@(GCW_CMAP)
	movel	d0,a0@(GCW_DAC)
	moveq	#0x1f,d0
	clrl	a0@(GCW_CMAP)
	movel	d0,a0@(GCW_DAC)
	moveq	#6,d0
	moveb	d0,init_flag1	| ??
	moveq	#7,d0
	moveb	d0,line_flag
	clrl	d0
	moveb	line_flag,d1
	btst	#2,d1
	beq	init1_3
	orw	#0x100,d0
init1_3:
	notb	d1
	andb	#3,d1
	orb	d1,d0
	clrl	d1
	moveb	init_flag1,d1
	lsll	#5,d1
	orl	d1,d0
	movel	d0,a0@(GCW_DAC)
	movel	#0x1ff,line_selmask
	bsr	init3
	bsr	init2
	moveal	sp@+,a0
	movel	sp@+,d0
	rts

| Video init 2
| LAB_036B at 0x0010047f4

	.even
init2:
	link	a6,#0
	moveml	d2-d7/a2-a5,sp@-
	moveal	#0x118808,a3
	clrw	init_flag2
init2_1:
	moveq	#0x10,d0
	cmpw	init_flag2,d0
	ble	init2_7
	movew	init_flag2,init_flag3
	moveb	a3@+,d7
	andw	#0xff,d7
	moveb	a3@+,d6
	extw	d6
	moveb	a3@+,d5
	andw	#0xff,d5
	moveb	a3@+,d4
	extw	d4
	moveb	a3@+,d3
	andw	0xff,d3
	moveb	a3@+,d2
	extw	d2
	moveal	init_flag4,a2
	clrw	init_flag5
init2_2:
	moveq	#16,d0
	cmpw	init_flag5,d0	| 06F4
	ble	init2_6
	movew	init_flag3,d1	| 06F8
	extl	d1
	moveq	#24,d0
	lsll	d0,d1
	movel	d1,init_flag6	| 6F0
	movew	d7,d1
	extl	d1
	moveq	#16,d0
	lsll	d0,d1
	orl	d1,init_flag6	| 6F0
	movew	d5,d1
	extl	d1
	lsll	#8,d1
	orl	d1,init_flag6	| 6F0
	movew	d3,d1
	extl	d1
	orl	d1,init_flag6
	movel	init_flag6,a2@+	| store map entry in table
	movew	d7,d1
	extl	d1
	movew	d6,d0
	extl	d0
	addl	d0,d1
	movew	d1,d7
	movew	d5,d0
	extl	d0
	movew	d4,d1
	extl	d1
	addl	d1,d0
	movew	d0,d5
	movew	d3,d0
	extl	d0
	movew	d2,d1
	extl	d1
	addl	d1,d0
	movew	d0,d3
	cmpw	#0xff,d7
	ble	init2_3
	movew	#0xff,d7
init2_3:
	cmpw	#0xff,d5
	ble	init2_4
	movew	#0xff,d5
init2_4:
	cmpw	#0xff,d3
	ble	init2_5
	movew	#0xff,d3
init2_5:
	moveq	#16,d0
	addw	d0,init_flag3	| 06F8
	addqw	#1,init_flag5	| 06F4
	bra	init2_2
init2_6:
	pea	init_flag4	| start of colour map
	moveq	#16,d0		| no. of entries
	movel	d0,sp@-
	bsr	c_pcmap	| write colour map to video board
	addql	#8,sp
	addqw	#1,init_flag2	| 06F6
	bra	init2_1
init2_7:
	moveml	sp@+,d2-d7/a2-a5
	unlk	a6
	rts


| C entry to program the colour map
| c_pcmap(int len, int *map)
|
| the format of the map appears to be an array of longwords, each word
| contains an index value and the values for R, G and B, all 8 bits.
| Format is: :INDEX:RED:GREEN:BLUE:

	.even
.globl	c_pcmap
c_pcmap:
	link	a6,#0
	movel	a5,sp@-
	movel	d7,sp@-
	moveal	a6@(12),a5	| point to colour map
	movel	a6@(8),d7	| no. of entries
	bsr	pcmap1
	movel	sp@+,d7
	moveal	sp@+,a5
	unlk	a6
	rts

| program colour map 1
| looks like you can only set a maximum of 127 entries at a time

	.even
pcmap1:
	moveml	d0-d7/a0-a6,sp@-
	moveq	#0x17,d0
	moveq	#0x1b,d1
	moveq	#0x1f,d2
	subql	#1,d7
	btst	#7,d7
	bne	pcmap1_1	| bra if more than 127 entries
	bsr	pcmap2
	bra	pcmap1_2	| done
pcmap1_1:
	subl	#0x7f,d7
	movel	d7,sp@-
	moveq	#0x7f,d7
	bsr	pcmap2		| first 127 entries
	movel	sp@+,d7
	bsr	pcmap2		| remainder of entries
pcmap1_2:
	moveml	sp@+,d0-d7/a0-a6
	rts

| program colour map 2
| this subroutine actually writes to the colour map on the video
| board

	.even
pcmap2:
	lea	GC_BASE,a0	| pint to control registers
	tstl	a0@(524)	| wait for VBLANK
pcmap2_1:
	btst	#6,a0@(524)
	beq	pcmap2_1
pcmap2_2:
	btst	#6,a0@(524)
	bne	pcmap2_2
pcmap2_3:
	movel	a5@+,d3		| map entry
	movew	d3,d5
	swap	d3
	movel	d5,d4
	rorl	#8,d4
	movel	d3,a0@(GCW_CMAP) | upper 8 bits is addr, lower is data
	movel	d0,a0@(GCW_DAC)	| write RED
	moveb	d4,d3
	movel	d3,a0@(GCW_CMAP)
	movel	d1,a0@(GCW_DAC)	| write GREEN
	moveb	d5,d3
	movel	d3,a0@(GCW_CMAP)
	movel	d2,a0@(GCW_DAC)	| write BLUE
	tstw	d7
	dbeq	d7,pcmap2_3	| loop
	clrl	a0@(GCW_CMAP)
	clrl	d0
|	moveb	line_flag,d1
	moveb	#7,d1		| line_flag when ROM monitor active
	btst	#2,d1
	beq	pcmap2_4
	orw	#0x100,d0	| overlay on
pcmap2_4:
	notb	d1
	andb	#3,d1
	orb	d1,d0
	clrl	d1
|	moveb	init_flag1,d1
	moveb	#6,d1		| init_flag1 when ROM monitor active
	lsll	#5,d1		| overlay colour ?
	orl	d1,d0
	movel	d0,a0@(GCW_DAC)	| set overlay colour & operating mode
	rts

| Init 3
| LAB_0389 at 0x001004b0c

	.even
init3:
	moveml	d0-d7/a0,sp@-
	movel	line_selmask,d6
	moveq	#5,d7
	clrl	d0
	clrl	d5
	moveq	#-1,d1
	movel	d1,d2
	movew	#0xfb00,d1
	movew	#0xf602,d2
	movel	#0x04fe,d3
	moveq	#7,d4
	lea	GC_BASE,a0	| base of video control registers
init3_1:
	movel	d0,a0@(GCW_INTEN)	| intensity
	movel	d6,a0@(GCW_PSEL)	| plane select
	movel	#0x0e,a0@(GCW_CMD)	| command
	movel	d1,a0@(GCW_LG2)		| line draw
	movel	d0,a0@(GCW_LG1)		| line draw
	movel	d2,a0@(GCW_LG2)		| line draw
	movel	d3,a0@(GCW_LG3)		| line draw
	movel	d4,a0@(GCW_LG4)		| line draw
	movel	d5,a0@(GCW_PY)		| pixel Y addr
	movel	d0,a0@(GCW_PX_SL)	| pixel X addr, start line
	addql	#1,d5			| incr Y addr
	cmpw	d7,d5
	bne	init3_1
	clrl	d2
	movel	#0x03fc,d3
	moveq	#19,d0
	moveq	#27,d1
	movel	d6,a0@(GCW_PSEL)	| plane select
	tstl	a0@(524)		| 0x20064c - CMD reg but inhibit cmd
init3_2:
	movel	a0@(524),d4
	btst	#0x1e,d4
	beq	init3_2
init3_3:
	movel	a0@(524),d4
	btst	#0x1e,d4
	bne	init3_3
	movel	d0,a0@(GCW_CMD)		| command
	movel	d2,a0@(GCW_PY_SM)	| pixel Y addr, start mem cycle
init3_4:
	movel	d1,a0@(GCW_CMD)		| command
	movel	d3,a0@(GCW_PY_SM)	| pixel Y addr, start mem cycle
	subql	#4,d3
	bpl	init3_4
	moveml	sp@+,d0-d7/a0
	rts

| in-lined version of ldraw

	.even
.globl	c_ldraw4
c_ldraw4:
	link	a6,#0
	moveml	d0-d7/a0-a6,sp@-
	lea	gr_data,a1	| point to data space
	movel	a6@(8),d2	| x1
	movel	a6@(12),d3	| y1
	movel	a6@(16),d1	| x2
	movel	a6@(20),d0	| y2
	moveb	#0x03,line_flag		| magic number ?

	moveq	#1,d5
	movel	d5,a1@(40)	| set x direction flag
	movel	d5,a1@(44)	| set y direction flag
	subl	d2,d1		| (d1-d2)->d1, (x2 - x1) -> d1
	movel	d1,a1@(16)	| delta-x

	subl	d3,d0		| (y2 - y1) -> d0
	movel	d0,a1@(20)	| delta-y
	cmpl	d1,d0		| delta-y - delta-x
	bgt	ldraw4_6	| bra if delta-y > delta-x
	movel	d5,d0		| set d0 to 1
	bra	ldraw4_7
ldraw4_6:
	clrl	d0		| set d0 to 0
ldraw4_7:
	movel	d0,a1@(48)	| set to 0 if delta-y > delta-x, else set to 1
	beq	ldraw4_8	| bra if delta-y > delta-x
	movel	a1@(20),d5	| delta-y to d5
	lsll	#1,d5		| shift left (mul by 2)
	movel	d5,d4		| copy to d4
	subl	a1@(16),d4	| (2 * dy) - dx -> d4
	subl	a1@(40),d4	| d4 - xdir -> d4
	movel	a1@(16),d6	| dx -> d6
	movel	a1@(20),d7	| dy -> d7
	subl	d6,d7		| dy - dx -> d7
	lsll	#1,d7		| shift left, d7 = ND/NA
	bra	ldraw4_9
ldraw4_8:
	movel	a1@(16),d5	| dx
	lsll	#1,d5		| shift left (mul by 2)
	movel	d5,d4
	subl	a1@(20),d4	| (2 * dx) - dy
	subl	a1@(44),d4	| ydir
	movel	a1@(20),d6	| dy
	movel	a1@(16),d7	| dx
	subl	d6,d7
	lsll	#1,d7		| shift left, d7 = ND/NA
ldraw4_9:

| by this point, d4 = ND/NA, d5 = NB

	subql	#1,d6		| d6 = NC
	movel	a1@(40),d0	| xdir
	lsll	#1,d0
	orl	a1@(44),d0	| ydir
	lsll	#1,d0
	orl	a1@(48),d0	| SX/SY

	lea	GC_BASE,a0	| point to video control registers
	movel	a6@(24),a0@(GCW_INTEN)		| intensity
	movel	a6@(28),a0@(GCW_PSEL)		| plane select
	movel	#0x0e,a0@(GCW_CMD)		| write, 4x5, 1 of 20
	tstl	a1@(16)
	bne	ldraw41
	tstl	a1@(20)
	bne	ldraw41
	movel	d3,a0@(GCW_PY)		| pixel Y addr
	movel	d2,a0@(GCW_PX_SM)	| pixel X addr, start single mem
	bra	ldraw4_done
ldraw41:
	movel	d4,a0@(GCW_LG2)		| line gen, ND15-ND0
	movel	d5,a0@(GCW_LG1)		| line gen, NB15-NB0
	movel	d7,a0@(GCW_LG2)		| line gen, ND15-ND0
	movel	d6,a0@(GCW_LG3)		| line-gen, NC11-NC0
	movel	d0,a0@(GCW_LG4)		| line-gen, SX, SY, XMAJ
	movel	d3,a0@(GCW_PY)		| pixel Y
	movel	d2,a0@(GCW_PX_SL)	| pixel X, start line
ldraw4_done:
	moveml	sp@+,d0-d7/a0-a6
	unlk	a6
	rts

| in-lined version of ldraw including y loop counter

	.even
.globl	c_ldraw4y
c_ldraw4y:
	link	a6,#0
	moveml	d0-d7/a0-a2,sp@-
	lea	gr_data,a1	| point to data space
	movel	a6@(8),d2	| x1
	movel	a6@(12),d3	| y1
	movel	a6@(20),a2	| y3
ldraw4y_1:
	cmpl	d3,a2
	jge	ldraw4y_2
	jra	ldraw4y_11
ldraw4y_2:
	movel	a6@(16),d1	| x2
	movel	a6@(12),d0	| y2 = y1
	moveb	#0x03,line_flag	| magic number ?

	moveq	#1,d5
	movel	d5,a1@(40)	| set x direction flag
	movel	d5,a1@(44)	| set y direction flag
	subl	d2,d1		| (d1-d2)->d1, (x2 - x1) -> d1
	movel	d1,a1@(16)	| delta-x

	subl	d3,d0		| (y2 - y1) -> d0
	movel	d0,a1@(20)	| delta-y
	cmpl	d1,d0		| delta-y - delta-x
	bgt	ldraw4y_6	| bra if delta-y > delta-x
	movel	d5,d0		| set d0 to 1
	bra	ldraw4y_7
ldraw4y_6:
	clrl	d0		| set d0 to 0
ldraw4y_7:
	movel	d0,a1@(48)	| set to 0 if delta-y > delta-x, else set to 1
	beq	ldraw4y_8	| bra if delta-y > delta-x
	movel	a1@(20),d5	| delta-y to d5
	lsll	#1,d5		| shift left (mul by 2)
	movel	d5,d4		| copy to d4
	subl	a1@(16),d4	| (2 * dy) - dx -> d4
	subl	a1@(40),d4	| d4 - xdir -> d4
	movel	a1@(16),d6	| dx -> d6
	movel	a1@(20),d7	| dy -> d7
	subl	d6,d7		| dy - dx -> d7
	lsll	#1,d7		| shift left, d7 = ND/NA
	bra	ldraw4y_9
ldraw4y_8:
	movel	a1@(16),d5	| dx
	lsll	#1,d5		| shift left (mul by 2)
	movel	d5,d4
	subl	a1@(20),d4	| (2 * dx) - dy
	subl	a1@(44),d4	| ydir
	movel	a1@(20),d6	| dy
	movel	a1@(16),d7	| dx
	subl	d6,d7
	lsll	#1,d7		| shift left, d7 = ND/NA
ldraw4y_9:

| by this point, d4 = ND/NA, d5 = NB

	subql	#1,d6		| d6 = NC
	movel	a1@(40),d0	| xdir
	lsll	#1,d0
	orl	a1@(44),d0	| ydir
	lsll	#1,d0
	orl	a1@(48),d0	| SX/SY

	lea	GC_BASE,a0	| point to video control registers
	movel	a6@(24),a0@(GCW_INTEN)		| intensity
	movel	#0xff,a0@(GCW_PSEL)		| plane select
	movel	#0x0e,a0@(GCW_CMD)		| write, 4x5, 1 of 20
	tstl	a1@(16)
	bne	ldraw4y_10
	tstl	a1@(20)
	bne	ldraw4y_10
	movel	d3,a0@(GCW_PY)		| pixel Y addr
	movel	d2,a0@(GCW_PX_SM)	| pixel X addr, start single mem
	bra	ldraw4y_done
ldraw4y_10:
	movel	d4,a0@(GCW_LG2)		| line gen, ND15-ND0
	movel	d5,a0@(GCW_LG1)		| line gen, NB15-NB0
	movel	d7,a0@(GCW_LG2)		| line gen, ND15-ND0
	movel	d6,a0@(GCW_LG3)		| line-gen, NC11-NC0
	movel	d0,a0@(GCW_LG4)		| line-gen, SX, SY, XMAJ
	movel	d3,a0@(GCW_PY)		| pixel Y
	movel	d2,a0@(GCW_PX_SL)	| pixel X, start line
ldraw4y_done:
	addql	#1,d3		| y++
	jra	ldraw4y_1
ldraw4y_11:
	moveml	sp@+,d0-d7/a0-a2
	unlk	a6
	rts


| in-lined version of ldraw including y loop counter
| this version optimised for horizontal lines

	.even
.globl	c_ldraw5y
c_ldraw5y:
	link	a6,#0
	moveml	d0-d7/a0-a2,sp@-
	lea	gr_data,a1	| point to data space
	lea	GC_BASE,a0	| base of video control registers
	movel	a6@(24),a0@(GCW_INTEN)	| intensity
	movel	#0xff,a0@(GCW_PSEL)	| plane select
	movel	a6@(8),d2	| x1
	movel	a6@(12),d3	| y1
	movel	a6@(16),d1	| x2
	movel	a6@(20),a2	| y2

	subl	d2,d1		| (x2 - x1) -> d1
	clrl	d4		| d4 = (2 * dy) = NA
	clrl	d5		| d5 = (2 * dy) = NB
	subl	d1,d4		| (2 * dy) - dx -> d4
	subql	#1,d4		| d4 - SX -> d4
	movel	d1,d6		| dx -> d6
	movel	d5,d7		| dy -> d7
	subl	d6,d7		| dy - dx -> d7
	lsll	#1,d7		| shift left, d7 = ND

| by this point, d4 = ND, d5 = NB

	subql	#1,d6		| d6 = NC
	moveq	#7,d0		| SX, SY, XMAJ

| we are drawing a series of horizontal lines, all the same length
| so no need to re-calculate all the magic numbers each time

ldraw5y_1:
	cmpl	d3,a2		| y1 <= y2
	jge	ldraw5y_2
	jra	ldraw5y_12
ldraw5y_2:
	movel	#0x0e,a0@(GCW_CMD)	| write, 4x5, 1 of 20
	tstl	d1			| dx
	bne	ldraw5y_10
	movel	d3,a0@(GCW_PY)		| pixel Y addr
	movel	d2,a0@(GCW_PX_SM)	| pixel X addr, start single mem
	bra	ldraw5y_11
ldraw5y_10:
	movel	d4,a0@(GCW_LG2)		| NA
	movel	d5,a0@(GCW_LG1)		| NB
	movel	d7,a0@(GCW_LG2)		| ND
	movel	d6,a0@(GCW_LG3)		| NC
	movel	d0,a0@(GCW_LG4)		| SX, SY, XMAJ
	movel	d3,a0@(GCW_PY)		| pixel Y
	movel	d2,a0@(GCW_PX_SL)	| pixel X, start line
ldraw5y_11:
	addql	#1,d3			| y++
	jra	ldraw5y_1
ldraw5y_12:
	moveml	sp@+,d0-d7/a0-a2
	unlk	a6
	rts

| in-lined version of ldraw including x loop counter
| this version optimised for vertical lines

	.even
.globl	c_ldraw5x
c_ldraw5x:
	link	a6,#0
	moveml	d0-d7/a0-a2,sp@-
	lea	gr_data,a1	| point to data space
	lea	GC_BASE,a0	| base of video control registers
	movel	a6@(24),a0@(GCW_INTEN)	| intensity
	movel	#0xff,a0@(GCW_PSEL)	| plane select
	movel	a6@(8),d2	| x1
	movel	a6@(12),d3	| y1
	movel	a6@(20),d1	| y2
	movel	a6@(16),a2	| x2

	subl	d3,d1		| (y2 - y1) -> d1
	clrl	d4		| d4 = (2 * dx) = NA
	clrl	d5		| d5 = (2 * dx) = NB
	subl	d1,d4		| (2 * dx) - dy -> d4
	subql	#1,d4		| d4 - SY -> d4
	movel	d1,d6		| dy -> d6
	movel	d5,d7		| dx -> d7
	subl	d6,d7		| dx - dy -> d7
	lsll	#1,d7		| shift left, d7 = ND

| by this point, d4 = ND, d5 = NB

	subql	#1,d6		| d6 = NC
	moveq	#6,d0		| SX, SY, XMAJ

| we are drawing a series of vertical lines, all the same length
| so no need to re-calculate all the magic numbers each time

ldraw5x_1:
	cmpl	d2,a2		| x1 <= x2
	jge	ldraw5x_2
	jra	ldraw5x_12
ldraw5x_2:
	movel	#0x0e,a0@(GCW_CMD)	| write, 4x5, 1 of 20
	tstl	d1			| dy
	bne	ldraw5x_10
	movel	d3,a0@(GCW_PY)		| pixel Y addr
	movel	d2,a0@(GCW_PX_SM)	| pixel X addr, start single mem
	bra	ldraw5x_11
ldraw5x_10:
	movel	d4,a0@(GCW_LG2)		| NA
	movel	d5,a0@(GCW_LG1)		| NB
	movel	d7,a0@(GCW_LG2)		| ND
	movel	d6,a0@(GCW_LG3)		| NC
	movel	d0,a0@(GCW_LG4)		| SX, SY, XMAJ
	movel	d3,a0@(GCW_PY)		| pixel Y
	movel	d2,a0@(GCW_PX_SL)	| pixel X, start line
ldraw5x_11:
	addql	#1,d2			| x++
	jra	ldraw5x_1
ldraw5x_12:
	moveml	sp@+,d0-d7/a0-a2
	unlk	a6
	rts

| read 1 pixel from the screen, handle the bizarre layout

	.even
.globl	r1pix3
r1pix3:
	link	a6,#0
	moveml	d1-d5/a0,sp@-
	movew	#1023,d5
	subw	a6@(14),d5	| d5 is y as 16-bit value
	movew	d5,d4
	andib	#0xfc,d4	| d4 is y0
	movew	#3,d0
	andw	d5,d0		| y & 3
	movel	#3,d3		| ensure top word is clear
	subw	d0,d3		| 3 - (y & 3)
	aslw	#3,d3		| << 3
	movel	a6@(8),d0	| x
	divuw	#20,d0		| x % 20
	clrw	d0		| clear quotient
	swap	d0		| put remainder in lower 16 bits
	divuw	#5,d0		| (x % 20) / 5
	aslw	#3,d0		| << 3
	addw	d0,d3		| ys in d3
	movew	#24,d0
	cmpw	d3,d0
	jge	r1pix3_1
	movew	#-32,d0
	addw	d0,d3
r1pix3_1:
	lea	GC_BASE,a0		| base of video control registers
	movel	a6@(16),a0@(GCW_PSEL)	| plane select mask
	movel	#0x07,a0@(GCW_LG4)
	movel	#0x66,a0@(GCW_CMD)
	movel	a6@(8),a0@(GCW_PX)	| x
	movel	d4,a0@(GCW_PY)		| y
	movel	a0@(GCR_IN_SM),d0	| read intensity, start single mem cycle
	movel	a0@(GCR_IN_SM),d0
	movel	d0,d1
	moveq	#0,d0
	notb	d0		| d0 = 0xff
	asll	d3,d0		| << fs
	andl	d1,d0		| (ic & (0xff << fs)
	asrl	d3,d0		| >> ys
	andl	#0xff,d0
	moveml	sp@+,d1-d5/a0
	unlk	a6
	rts

| read a rectangular array of pixels from the screen
| void rd_rect(int x1, int y1, int x2, int y2, char *buf)

	.even
.globl	rd_rect2
rd_rect2:
	link	a6,#0
	moveml	d1-d7/a0-a1,sp@-
	movel	#1023,d7
	subl	a6@(20),d7	| d7 is y2 as 16-bit value
	movel	d7,a6@(20)
	movel	#1023,d6
	subl	a6@(12),d6	| d6 is y1 as 16-bit value
	movel	d6,a6@(12)
	movel	a6@(8),d5	| d5 is x
	lea	GC_BASE,a0	| base of video control registers
	movel	a6@(24),a1	| buf
rd_rect2_1:
	cmpw	a6@(18),d5	| x < x2
	jlt	rd_rect2_2
	jra	rd_rect2_8
rd_rect2_2:
	movel	d5,d4		| d4 is xs
	divuw	#20,d4		| x % 20
	clrw	d4		| clear quotient
	swap	d4		| remainder in lower word
	divuw	#5,d4		| (x % 20) / 5
	aslw	#3,d4		| ((x % 20) / 5) << 3
rd_rect2_3:
	cmpw	d7,d6		| y > y2
	jgt	rd_rect2_4
	jra	rd_rect2_7
rd_rect2_4:
	movel	d6,d3		| d3 is y0
	andib	#0xfc,d3	| y & 0xfffc
	moveq	#3,d0
	andw	d6,d0		| y & 3
	moveq	#3,d2		| ensure top word is clear
	subw	d0,d2		| 3 - (y & 3)
	aslw	#3,d2		| << 3
	addw	d4,d2		| ys in d2
	moveq	#24,d0
	cmpw	d2,d0
	jge	rd_rect2_5
	movew	#-32,d0
	addw	d0,d2
rd_rect2_5:
	movel	#0xff,a0@(GCW_PSEL)	| plane select mask
	movel	#0x07,a0@(GCW_LG4)
	movel	#0x66,a0@(GCW_CMD)
	movel	d5,a0@(GCW_PX)		| x
	movel	d3,a0@(GCW_PY)		| y0
	movel	a0@(GCR_IN_SM),d1	| read intensity, start single mem cycle
	movel	a0@(GCR_IN_SM),d1
	moveq	#0,d0
	notb	d0		| d0 = 0xff
	asll	d2,d0		| << fs
	andl	d1,d0		| (ic & (0xff << fs)
	asrl	d2,d0		| >> ys
	andl	#0xff,d0
	moveb	d0,a1@+		| *buf++
rd_rect2_6:
	subqw	#1,d6		| y--
	jra	rd_rect2_3
rd_rect2_7:
	addqw	#1,d5		| x++
	movel	a6@(12),d6	| y = y1
	jra	rd_rect2_1
rd_rect2_8:
	moveml	sp@+,d1-d7/a0-a1
	unlk	a6
	rts


| write a rectangular array of pixels to the screen
| void wr_rect(int x1, int y1, int x2, int y2, char *buf)

	.even
.globl	wr_rect2
wr_rect2:
	link	a6,#0
	moveml	d1-d7/a0-a1,sp@-
	movel	#1023,d7
	subl	a6@(20),d7	| d7 is y2 as 16-bit value
	movel	d7,a6@(20)
	movel	#1023,d6
	subl	a6@(12),d6	| d6 is y1 as 16-bit value
	movel	d6,a6@(12)
	movel	a6@(8),d5	| d5 is x
	lea	GC_BASE,a0	| base of video control registers
	movel	a6@(24),a1	| buf
wr_rect2_1:
	cmpw	a6@(18),d5	| x < x2
	jlt	wr_rect2_2
	jra	wr_rect2_8
wr_rect2_2:
	movel	d5,d4		| d4 is xs
	divuw	#20,d4		| x % 20
	clrw	d4		| clear quotient
	swap	d4		| remainder in lower word
	divuw	#5,d4		| (x % 20) / 5
	aslw	#3,d4		| ((x % 20) / 5) << 3
wr_rect2_3:
	cmpw	d7,d6		| y > y2
	jgt	wr_rect2_4
	jra	wr_rect2_7
wr_rect2_4:
	movel	d6,d3		| d3 is y0
	andib	#0xfc,d3	| y & 0xfffc
	movel	#0xff,a0@(GCW_PSEL)	| plane select mask
	movel	#0x07,a0@(GCW_LG4)
	movel	#0x66,a0@(GCW_CMD)
	movel	d5,a0@(GCW_PX)		| x
	movel	d3,a0@(GCW_PY)		| y0
	movel	a0@(GCR_IN_SM),d1	| read intensity, start single mem cycle
	movel	a0@(GCR_IN_SM),d1	| d1 is ic
	moveq	#3,d0
	andw	d6,d0		| y & 3
	moveq	#3,d2		| ensure top word is clear
	subw	d0,d2		| 3 - (y & 3)
	aslw	#3,d2		| << 3
	addw	d4,d2		| ys in d2
	moveq	#24,d0
	cmpw	d2,d0
	jge	wr_rect2_5
	movew	#-32,d0
	addw	d0,d2
wr_rect2_5:
	moveq	#0,d0
	notb	d0		| d0 = 0xff
	asll	d2,d0		| <<fs
	notl	d0		| invert
	andl	d1,d0		| (ic & (0cff << ys))
	clrl	d1
	moveb	a1@+,d1		| *buf++
	asll	d2,d1		| << ys
	orl	d1,d0
	movel	#0xff,a0@(GCW_PSEL)	| plane select mask
	movel	#0x07,a0@(GCW_LG4)
	movel	#0x6e,a0@(GCW_CMD)
	movel	d5,a0@(GCW_PX)		| x
	movel	d3,a0@(GCW_PY)		| y0
	movel	d0,a0@(GCW_IN_SM)	| write inten, start single mem cycle
wr_rect2_6:
	subqw	#1,d6		| y--
	jra	wr_rect2_3
wr_rect2_7:
	addqw	#1,d5		| x++
	movel	a6@(12),d6	| y = y1
	jra	wr_rect2_1
wr_rect2_8:
	moveml	sp@+,d1-d7/a0-a1
	unlk	a6
	rts

| write a rectangular array of pixels to the screen
| void wr_rect4(int x1, int y1, int x2, int y2, char *buf)
|
| This version writes the block with the x axis as the inner loop,
| this is slower than wr_rect2() but is necessary for the VNC
| CopyDataToScreen() function.

	.even
.globl	wr_rect4
wr_rect4:
	link	a6,#0
	moveml	d1-d7/a0-a1,sp@-
	movel	#1023,d7
	subl	a6@(20),d7	| d7 is y2 as 16-bit value
	movel	d7,a6@(20)
	movel	#1023,d6
	subl	a6@(12),d6	| d6 is y1 as 16-bit value
	movel	d6,a6@(12)
	movel	a6@(8),d5	| d5 is x
	lea	GC_BASE,a0	| base of video control registers
	movel	a6@(24),a1	| buf
wr_rect4_1:
	cmpw	d7,d6		| y > y2
	jgt	wr_rect4_2
	jra	wr_rect4_8
wr_rect4_2:
	movel	d6,d3		| d3 is y0
	andib	#0xfc,d3	| y & 0xfffc
	moveq	#3,d0
	andw	d6,d0		| y & 3
	moveq	#3,d2		| ensure top word is clear
	subw	d0,d2		| 3 - (y & 3)
	aslw	#3,d2		| << 3
wr_rect4_3:
	cmpw	a6@(18),d5	| x < x2
	jlt	wr_rect4_4
	jra	wr_rect4_7
wr_rect4_4:
	movel	#0xff,a0@(GCW_PSEL)	| plane select mask
	movel	#0x07,a0@(GCW_LG4)
	movel	#0x66,a0@(GCW_CMD)
	movel	d5,a0@(GCW_PX)		| x
	movel	d3,a0@(GCW_PY)		| y0
	movel	a0@(GCR_IN_SM),d1	| read intensity, start single mem cycle
	movel	a0@(GCR_IN_SM),d1	| d1 is ic
	movel	d5,d4		| d4 is xs
	divuw	#20,d4		| x % 20
	clrw	d4		| clear quotient
	swap	d4		| remainder in lower word
	divuw	#5,d4		| (x % 20) / 5
	aslw	#3,d4		| ((x % 20) / 5) << 3
	addw	d2,d4		| ys in d2, xs in d4
	moveq	#24,d0
	cmpw	d4,d0
	jge	wr_rect4_5
	movew	#-32,d0
	addw	d0,d4
wr_rect4_5:
	moveq	#0,d0
	notb	d0		| d0 = 0xff
	asll	d4,d0		| <<fs
	notl	d0		| invert
	andl	d1,d0		| (ic & (0cff << ys))
	clrl	d1
	moveb	a1@+,d1		| *buf++
	asll	d4,d1		| << ys
	orl	d1,d0
	movel	#0xff,a0@(GCW_PSEL)	| plane select mask
	movel	#0x07,a0@(GCW_LG4)
	movel	#0x6e,a0@(GCW_CMD)
	movel	d5,a0@(GCW_PX)		| x
	movel	d3,a0@(GCW_PY)		| y0
	movel	d0,a0@(GCW_IN_SM)	| write inten, start single mem cycle
wr_rect4_6:
	addqw	#1,d5		| x++
	jra	wr_rect4_3
wr_rect4_7:
	subqw	#1,d6		| y--
	movel	a6@(8),d5	| x = x1
	jra	wr_rect4_1
wr_rect4_8:
	moveml	sp@+,d1-d7/a0-a1
	unlk	a6
	rts

| write a rectangular array of pixels to the screen
| void wr_rect5(int x1, int y1, int x2, int y2, char *buf)
|
| This version writes the block with the x axis as the inner loop,
| this is slower than wr_rect2() but is necessary for the VNC
| CopyDataToScreen() function.
|
| This version uses a lookup table for the pixel mask and shift count.
| This removes one of the divide instructions (divides are slow).

	.even
.globl	wr_rect5
wr_rect5:
	link	a6,#0
	moveml	d1-d7/a0-a2,sp@-
	movel	#1023,d7
	subl	a6@(20),d7	| d7 is y2
	movel	#1023,d6
	subl	a6@(12),d6	| d6 is y1
	movel	a6@(8),d5	| d5 is x
	lea	GC_BASE,a0	| base of video control registers
	movel	a6@(24),a1	| buf
	lea	pm_tab,a2	| base of lookup table
	movel	#0xff,a0@(GCW_PSEL)	| plane select mask
wr_rect5_1:
	cmpl	d7,d6		| y > y2
	jle	wr_rect5_8
	movel	d6,d3		| d3 is y0
	andib	#0xfc,d3	| y & 0xfffc
	moveq	#3,d2
	andl	d6,d2		| y & 3
	asll	#3,d2		| (y & 3) * 8
wr_rect5_3:
	cmpl	a6@(16),d5	| x < x2
	jge	wr_rect5_7
	movel	#0x07,a0@(GCW_LG4)
	movel	#0x66,a0@(GCW_CMD)
	movel	d5,a0@(GCW_PX)		| x
	movel	d3,a0@(GCW_PY)		| y0
	movel	a0@(GCR_IN_SM),d1	| read intensity, start single mem cycle
	movel	a0@(GCR_IN_SM),d1	| d1 is ic
	movel	d5,d4		| d4 is xs
	divuw	#20,d4		| x % 20
	clrw	d4		| clear quotient
	swap	d4		| remainder in lower word
	asll	#5,d4		| *32
	addl	d2,d4		| ((x % 20) * 32) + ((y % 4) * 8)
	movel	a2@(d4:w:1),d0	| fetch mask
	andl	d1,d0		| (ic & mask)
|	andl	a2@(d4:w:1),d0	| (ic & mask)
	clrl	d1
	moveb	a1@+,d1		| *buf++
	addql	#4,d4
	movel	a2@(d4:w:1),d4	| fetch shift count
	asll	d4,d1		| << ys
	orl	d1,d0
	movel	#0x07,a0@(GCW_LG4)
	movel	#0x6e,a0@(GCW_CMD)
	movel	d5,a0@(GCW_PX)		| x
	movel	d3,a0@(GCW_PY)		| y0
	movel	d0,a0@(GCW_IN_SM)	| write inten, start single mem cycle
wr_rect5_6:
	addql	#1,d5		| x++
	jra	wr_rect5_3
wr_rect5_7:
	subql	#1,d6		| y--
	movel	a6@(8),d5	| x = x1
	jra	wr_rect5_1
wr_rect5_8:
	moveml	sp@+,d1-d7/a0-a2
	unlk	a6
	rts

| write 1 pixel to the screen, handle the bizarre layout

	.even
.globl	w1pix3
w1pix3:
	link	a6,#0
	moveml	d1-d5/a0,sp@-
	movew	#1023,d5
	subw	a6@(14),d5	| d5 is y as 16-bit value
	movew	d5,d4
	andib	#0xfc,d4	| d4 is y0
	lea	GC_BASE,a0		| base of video control registers
	movel	a6@(16),a0@(GCW_PSEL)	| plane select mask
	movel	#0x07,a0@(GCW_LG4)
	movel	#0x66,a0@(GCW_CMD)
	movel	a6@(8),a0@(GCW_PX)	| x
	movel	d4,a0@(GCW_PY)		| y0
	movel	a0@(GCR_IN_SM),d0	| read intensity, start single mem cycle
	movel	a0@(GCR_IN_SM),d0	| 4 pixels in d0
	movel	d0,d3		| ic in d3
	movew	#3,d0
	andw	d5,d0		| y & 3
	movel	#3,d2		| ensure top word is clear
	subw	d0,d2		| 3 - (y & 3)
	aslw	#3,d2		| << 3
	movel	a6@(8),d0	| x
	divuw	#20,d0		| x % 20
	clrw	d0		| clear quotient
	swap	d0		| put remainder in lower 16 bits
	divuw	#5,d0		| (x % 20) / 5
	aslw	#3,d0		| << 3
	addw	d0,d2		| ys in d2
	movew	#24,d0
	cmpw	d2,d0
	jge	w1pix3_1
	movew	#-32,d0
	addw	d0,d2
w1pix3_1:
	moveq	#0,d0
	notb	d0		| d0 = 0xff
	asll	d2,d0		| << ys
	notl	d0		| invert
	andl	d3,d0		| (ic & (0xff << ys)
	movel	a6@(20),d1	| i
	asll	d2,d1		| i << ys
	orl	d1,d0
	movel	a6@(16),a0@(GCW_PSEL)	| plane select mask
	movel	#0x07,a0@(GCW_LG4)
	movel	#0x6e,a0@(GCW_CMD)
	movel	a6@(8),a0@(GCW_PX)	| x
	movel	d4,a0@(GCW_PY)		| y0
	movel	d0,a0@(GCW_IN_SM)	| write inten, start single mem cycle
	moveml	sp@+,d1-d5/a0
	unlk	a6
	rts

| data

	.data

| lookup table for wr_rect5()
| this gives the pixel mask and shift count to locate each pixel in 
| the 4 x 20 array pattern used by the Prisma video board

pm_tab:		.long	0x00ffffff, 24	|  0,0
		.long	0xff00ffff, 16	|  0,1
		.long	0xffff00ff,  8	|  0,2
		.long	0xffffff00,  0	|  0,3
		.long	0x00ffffff, 24	|  1,0
		.long	0xff00ffff, 16	|  1,1
		.long	0xffff00ff,  8	|  1,2
		.long	0xffffff00,  0	|  1,3
		.long	0x00ffffff, 24	|  2,0
		.long	0xff00ffff, 16	|  2,1
		.long	0xffff00ff,  8	|  2,2
		.long	0xffffff00,  0	|  2,3
		.long	0x00ffffff, 24	|  3,0
		.long	0xff00ffff, 16	|  3,1
		.long	0xffff00ff,  8	|  3,2
		.long	0xffffff00,  0	|  3,3
		.long	0x00ffffff, 24	|  4,0
		.long	0xff00ffff, 16	|  4,1
		.long	0xffff00ff,  8	|  4,2
		.long	0xffffff00,  0	|  4,3
		.long	0xffffff00,  0	|  5,0
		.long	0x00ffffff, 24	|  5,1
		.long	0xff00ffff, 16	|  5,2
		.long	0xffff00ff,  8	|  5,3
		.long	0xffffff00,  0	|  6,0
		.long	0x00ffffff, 24	|  6,1
		.long	0xff00ffff, 16	|  6,2
		.long	0xffff00ff,  8	|  6,3
		.long	0xffffff00,  0	|  7,0
		.long	0x00ffffff, 24	|  7,1
		.long	0xff00ffff, 16	|  7,2
		.long	0xffff00ff,  8	|  7,3
		.long	0xffffff00,  0	|  8,0
		.long	0x00ffffff, 24	|  8,1
		.long	0xff00ffff, 16	|  8,2
		.long	0xffff00ff,  8	|  8,3
		.long	0xffffff00,  0	|  9,0
		.long	0x00ffffff, 24	|  9,1
		.long	0xff00ffff, 16	|  9,2
		.long	0xffff00ff,  8	|  9,3
		.long	0xffff00ff,  8	| 10,0
		.long	0xffffff00,  0	| 10,1
		.long	0x00ffffff, 24	| 10,2
		.long	0xff00ffff, 16	| 10,3
		.long	0xffff00ff,  8	| 11,0
		.long	0xffffff00,  0	| 11,1
		.long	0x00ffffff, 24	| 11,2
		.long	0xff00ffff, 16	| 11,3
		.long	0xffff00ff,  8	| 12,0
		.long	0xffffff00,  0	| 12,1
		.long	0x00ffffff, 24	| 12,2
		.long	0xff00ffff, 16	| 12,3
		.long	0xffff00ff,  8	| 13,0
		.long	0xffffff00,  0	| 13,1
		.long	0x00ffffff, 24	| 13,2
		.long	0xff00ffff, 16	| 13,3
		.long	0xffff00ff,  8	| 14,0
		.long	0xffffff00,  0	| 14,1
		.long	0x00ffffff, 24	| 14,2
		.long	0xff00ffff, 16	| 14,3
		.long	0xff00ffff, 16	| 15,0
		.long	0xffff00ff,  8	| 15,1
		.long	0xffffff00,  0	| 15,2
		.long	0x00ffffff, 24	| 15,3
		.long	0xff00ffff, 16	| 16,0
		.long	0xffff00ff,  8	| 16,1
		.long	0xffffff00,  0	| 16,2
		.long	0x00ffffff, 24	| 16,3
		.long	0xff00ffff, 16	| 17,0
		.long	0xffff00ff,  8	| 17,1
		.long	0xffffff00,  0	| 17,2
		.long	0x00ffffff, 24	| 17,3
		.long	0xff00ffff, 16	| 18,0
		.long	0xffff00ff,  8	| 18,1
		.long	0xffffff00,  0	| 18,2
		.long	0x00ffffff, 24	| 18,3
		.long	0xff00ffff, 16	| 19,0
		.long	0xffff00ff,  8	| 19,1
		.long	0xffffff00,  0	| 19,2
		.long	0x00ffffff, 24	| 19,3


| temporary storage for line drawing routine

gr_data:
line_x1:	.long	0	|  0 (668)
line_y1:	.long	0	|  4 (66c)
line_x2:	.long	0	|  8 (670)
line_y2:	.long	0	| 12 (674)
		.long	0	| 16 (678)
		.long	0	| 20 (67c)
		.long	0	| 24 (680)
		.long	0	| 28 (684)
		.long	0	| 32 (688)
		.long	0	| 36 (68c)
line_xdir:	.long	0	| 40 (690)
line_ydir:	.long	0	| 44 (694)
		.long	0	| 48 (698)
line_selmask:	.long	0	| 52 (69c)
line_inten:	.long	0	| 56 (6A0)
line_flag:	.word	0	| 60 (6a4)
init_flag1:	.word	0	| 62 (6a6)
		.long	0	| 64 (6a8)
		.long	0	| 64 (6ac)
init_flag4:	.long	0	| 64 (6b0)
		.long	0	| 64 (6b4)
		.long	0	| 64 (6b8)
		.long	0	| 64 (6bc)
		.long	0	| 64 (6c0)
		.long	0	| 64 (6c4)
		.long	0	| 64 (6c8)
		.long	0	| 64 (6cc)
		.long	0	| 64 (6d0)
		.long	0	| 64 (6d4)
		.long	0	| 64 (6d8)
		.long	0	| 64 (6dc)
		.long	0	| 64 (6e0)
		.long	0	| 64 (6e4)
		.long	0	| 64 (6e8)
		.long	0	| 64 (6ec)
init_flag6:	.long	0	| 64 (6f0)
init_flag5:	.word	0	| 64 (6f4)
init_flag2:	.word	0	| 64 (6f6)
init_flag3:	.word	0	| 64 (6f8)



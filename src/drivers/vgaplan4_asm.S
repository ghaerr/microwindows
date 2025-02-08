// int10 and INLINE_FP routines for ia16-elf-gcc compiler
// Copyright (c) 1999, 2019, 2025 Greg Haerr <greg@censoft.com>

.set INLINE_FP,1       // =1 to inline xxx_FP functions (no ASM calls)

	.code16
	.text
//
// es:bp = int10(int ax,int bx)
//  Call video bios using interrupt 10h
//
//FARADDR int10(int ax,int bx)
	.global	int10
int10:
	push	%bp
	mov		%sp,%bp
	push	%es
	push	%ds
	push	%si
	push	%di

	mov		4(%bp),%ax	// get first arg
	mov		6(%bp),%bx	// get second arg
	int		$0x10
	mov		%es,%dx		// return es:bp
	mov		%bp,%ax

	pop		%di
	pop		%si
	pop		%ds
	pop		%es
	pop		%bp
	ret

.if !INLINE_FP
//
// Return the byte at far address
//
//unsigned char GETBYTE_FP(FARADDR addr)
//
	.global	GETBYTE_FP
GETBYTE_FP:
	push	%bp
	mov		%sp,%bp
	mov		%ds,%dx

	lds		4(%bp),%bx
	mov		(%bx),%al	// get byte at ds:bx
	xor		%ah,%ah

	mov		%dx,%ds
	pop		%bp
	ret

//
// Put the byte at far address
//
//void PUTBYTE_FP(FARADDR addr,unsigned char val)
//
	.global	PUTBYTE_FP
PUTBYTE_FP:
	push	%bp
	mov		%sp,%bp
	mov		%ds,%dx

	lds		4(%bp),%bx
	mov		8(%bp),%al	// al = val
	mov		%al,(%bx)	// put type at ds:bx

	mov		%dx,%ds
	pop		%bp
	ret

//
// Read-modify-write the byte at far address
//
//void RMW_FP(FARADDR addr)
	.global	RMW_FP
RMW_FP:
	push	%bp
	mov		%sp,%bp
	mov		%ds,%dx

	lds		4(%bp),%bx
	or		%al,(%bx)	// rmw byte at ds:bx, al value doesn't matter

	mov		%dx,%ds
	pop		%bp
	ret

//
// Or the byte at far address
//
//void ORBYTE_FP(FARADDR addr,unsigned char val)
	.global	ORBYTE_FP
ORBYTE_FP:
	push	%bp
	mov		%sp,%bp
	mov		%ds,%dx

	lds		4(%bp),%bx
	mov		8(%bp),%al	// al = val
	or		%al,(%bx)	// or byte at ds:bx

	mov		%dx,%ds
	pop		%bp
	ret

//
// And the byte at far address
//
//void ANDBYTE_FP(FARADDR addr,unsigned char val)
	.global	ANDBYTE_FP
ANDBYTE_FP:
	push	%bp
	mov		%sp,%bp
	mov		%ds,%dx

	lds		4(%bp),%bx
	mov		8(%bp),%al	// al = val
	and		%al,(%bx)	// and byte at ds:bx

	mov		%dx,%ds
	pop		%bp
	ret

//
// Input byte from i/o port
//
//int inportb(int port)
	.global	inportb
inportb:
	push	%bp
	mov		%sp,%bp

	mov		4(%bp),%dx	// dx = port
	in		%dx,%al		// input byte
	xor		%ah,%ah

	pop		%bp
	ret

//
// Output byte to i/o port
//
//void outportb(int port,unsigned char data)
	.global	outportb
outportb:
	push	%bp
	mov		%sp,%bp

	mov		4(%bp),%dx	// dx = port
	mov		6(%bp),%al	// al = data
	out		%al,%dx

	pop		%bp
	ret

//
// Output word i/o port
//
//void outport(int port,int data)
	.global	outport
outport:
	push	%bp
	mov		%sp,%bp

	mov		4(%bp),%dx	// dx = port
	mov		6(%bp),%ax	// ax = data
	out		%ax,%dx

	pop		%bp
	ret
.endif

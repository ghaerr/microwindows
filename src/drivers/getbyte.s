# 05/08/2000 Michael Temari <Michael@TemWare.Com>
# GETBYTE/PUTBYTE for MINIX
! sections

.sect .text; .sect .rom; .sect .data; .sect .bss

#include "/usr/src/kernel/protect.h"

.extern	_GETBYTE_FP
.extern	_PUTBYTE_FP
.extern	_RMW_FP

.sect .bss
.sect .text

        .align  16
_GETBYTE_FP:
	mov	bx,ds
        mov	ecx, 0x17
        mov	ds, cx
        mov	edx,4(esp)              ! offset
        sub	ax, ax
        movb	al,(edx)                ! byte to get
        mov	ds,bx
        ret

        .align  16
_PUTBYTE_FP:
	mov	bx,ds
        mov	ecx, 0x17
        mov	ds, cx
        mov	eax,4(esp)              ! offset
        mov	edx,4+4(esp)            ! data byte
        movb	(eax),dl                ! byte to store
        mov	ds,bx
        ret

        .align  16
_RMW_FP:
        mov	bx,ds
        mov	ecx, 0x17
        mov	ds, cx
        mov	eax,4(esp)              ! offset
        orb	(eax),cl                ! byte to store
        mov	ds,bx
        ret

/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Header file for EGA/VGA 16 color 4 planes screen driver
 * Added functions for Hercules access
 *
 */
#define SLOWVGA		0	/* =1 for outb rather than outw instructions*/

#ifdef __PACIFIC__
#define HAVEBLIT	0
#else
#define HAVEBLIT	1	/* =0 to exclude blitting in vgaplan4 drivers*/
#endif

#if UNIX & !ELKS
#define HAVEFARPTR	1
#define FAR
#endif

#if LINUX
#if !(defined(ARCH_LINUX_POWERPPC) || defined(ARCH_LINUX_SPARC))
#ifdef __GLIBC__
#include <sys/io.h>		/* for outb def's, requires -O */
#else
#include <asm/io.h>		/* for outb def's on 2.3.x*/
#endif
#endif
#include <unistd.h>
#define HAVEFARPTR	1
#define FAR
#define HAVEIOPERM	1	/* has ioperm() system call*/
#endif

#if MSDOS
#define HAVEFARPTR	1	/* compiler has _far extension*/
#ifdef __PACIFIC__
#include <dos.h>
#define FAR		far
#else
#define FAR		_far
#endif
#endif

#if __rtems__
#define HAVEFARPTR	1
#define FAR
#include <i386_io.h>
#endif

#if MSDOS | ELKS
/* make far ptr*/
#define MK_FP(seg,ofs)	((FARADDR)(((unsigned long)(seg) << 16) | (unsigned)(ofs)))
#define EGA_BASE 	MK_FP(0xa000, 0)
#else
#define EGA_BASE 	((unsigned char *)0xa0000)
#endif

#if HAVEFARPTR
/* far ptr access to screen*/
typedef volatile unsigned char FAR * FARADDR;

#if _MINIX
/* get byte at address*/
extern unsigned char GETBYTE_FP(FARADDR);

/* put byte at address*/
extern void PUTBYTE_FP(FARADDR,unsigned char);

/* read-modify-write at address*/
extern void RMW_FP(FARADDR);
#else
/* get byte at address*/
#define GETBYTE_FP(addr)	(*(FARADDR)(addr))

/* put byte at address*/
#define PUTBYTE_FP(addr,val)	((*(FARADDR)(addr)) = (val))

/* read-modify-write at address*/
#define RMW_FP(addr)		((*(FARADDR)(addr)) |= 1)

/* or byte at address*/
#define ORBYTE_FP(addr,val)	((*(FARADDR)(addr)) |= (val))

/* and byte at address*/
#define ANDBYTE_FP(addr,val)	((*(FARADDR)(addr)) &= (val))
#endif
#else

/* for bcc with no _far extension*/
typedef unsigned long	FARADDR;

/* get byte at address*/
extern unsigned char GETBYTE_FP(FARADDR);

/* put byte at address*/
extern void PUTBYTE_FP(FARADDR,unsigned char);

/* read-modify-write at address*/
extern void RMW_FP(FARADDR);

/* or byte at address*/
extern void ORBYTE_FP(FARADDR,unsigned char);

/* and byte at address*/
extern void ANDBYTE_FP(FARADDR,unsigned char);
#endif


#if MSDOS
#define outb(val,port)	outp(port,val)
#endif

#if ELKS
#define outb(val,port)	outportb(port,val)
#define outw(val,port)	outport(port,val)

extern int  inportb(int port);
extern void outportb(int port,unsigned char data);
extern void outport(int port,int data);
#endif

/* external routines*/
FARADDR		int10(int ax,int bx);

/* external routines implementing planar ega/vga access*/

/* vgaplan4.c portable C, asmplan4.s asm, or ELKS asm elkplan4.c driver*/
int		ega_init(PSD psd);
void 		ega_drawpixel(PSD psd,unsigned int x,unsigned int y,
			MWPIXELVAL c);
MWPIXELVAL 	ega_readpixel(PSD psd,unsigned int x,unsigned int y);
void		ega_drawhorzline(PSD psd,unsigned int x1,unsigned int x2,
			unsigned int y,MWPIXELVAL c);
void		ega_drawvertline(PSD psd,unsigned int x,unsigned int y1,
			unsigned int y2, MWPIXELVAL c);
#if HAVEBLIT
/* memplan4.c*/
void	 	ega_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w,
			MWCOORD h,PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op);
#endif

/* vgainit.c - direct hw init*/
void		ega_hwinit(void);
void		ega_hwterm(void);

#if _MINIX
#define	outb(v, p)	outb(p, v)
#define	outw(v, p)	outw(p, v)
#endif

#if SLOWVGA
/* use outb rather than outw instructions for older, slower VGA's*/

/* Program the Set/Reset Register for drawing in color COLOR for write
   mode 0. */
#define set_color(c)		{ outb (0, 0x3ce); outb (c, 0x3cf); }

/* Set the Enable Set/Reset Register. */
#define set_enable_sr(mask) { outb (1, 0x3ce); outb (mask, 0x3cf); }

/* Select the Bit Mask Register on the Graphics Controller. */
#define select_mask() 		{ outb (8, 0x3ce); }

/* Program the Bit Mask Register to affect only the pixels selected in
   MASK.  The Bit Mask Register must already have been selected with
   select_mask (). */
#define set_mask(mask)		{ outb (mask, 0x3cf); }

#define select_and_set_mask(mask) { outb (8, 0x3ce); outb (mask, 0x3cf); }

/* Set the Data Rotate Register.  Bits 0-2 are rotate count, bits 3-4
   are logical operation (0=NOP, 1=AND, 2=OR, 3=XOR). */
#define set_op(op) 		{ outb (3, 0x3ce); outb (op, 0x3cf); }

/* Set the Memory Plane Write Enable register. */
#define set_write_planes(mask) { outb (2, 0x3c4); outb (mask, 0x3c5); }

/* Set the Read Map Select register. */
#define set_read_plane(plane)	{ outb (4, 0x3ce); outb (plane, 0x3cf); }

/* Set the Graphics Mode Register.  The write mode is in bits 0-1, the
   read mode is in bit 3. */
#define set_mode(mode) 		{ outb (5, 0x3ce); outb (mode, 0x3cf); }

#else /* !SLOWVGA*/
/* use outw rather than outb instructions for new VGAs*/

/* Program the Set/Reset Register for drawing in color COLOR for write
   mode 0. */
#define set_color(c)		{ outw ((c)<<8, 0x3ce); }

/* Set the Enable Set/Reset Register. */
#define set_enable_sr(mask) 	{ outw (1|((mask)<<8), 0x3ce); }

/* Select the Bit Mask Register on the Graphics Controller. */
#define select_mask() 		{ outb (8, 0x3ce); }

/* Program the Bit Mask Register to affect only the pixels selected in
   MASK.  The Bit Mask Register must already have been selected with
   select_mask (). */
#define set_mask(mask)		{ outb (mask, 0x3cf); }

#define select_and_set_mask(mask) { outw (8|((mask)<<8), 0x3ce); }

/* Set the Data Rotate Register.  Bits 0-2 are rotate count, bits 3-4
   are logical operation (0=NOP, 1=AND, 2=OR, 3=XOR). */
#define set_op(op) 		{ outw (3|((op)<<8), 0x3ce); }

/* Set the Memory Plane Write Enable register. */
#define set_write_planes(mask) { outw (2|((mask)<<8), 0x3c4); }

/* Set the Read Map Select register. */
#define set_read_plane(plane)	{ outw (4|((plane)<<8), 0x3ce); }

/* Set the Graphics Mode Register.  The write mode is in bits 0-1, the
   read mode is in bit 3. */
#define set_mode(mode) 		{ outw (5|((mode)<<8), 0x3ce); }

#endif /* SLOWVGA*/

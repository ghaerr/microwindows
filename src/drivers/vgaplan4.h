/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Header file for EGA/VGA 16 color 4 planes screen driver
 * Added functions for Hercules access
 *
 */
#define SLOWVGA		0	/* =1 for outb rather than outw instructions*/

#if ELKS
#define HAVEFARPTR	1       /* =1 compiler has __far extension */
#define INLINE_FP       1       /* =1 to inline xxx_FP functions */
#define FAR         __far
#elif _MINIX
#define HAVEFARPTR	1
#define INLINE_FP       0
#define FAR
#include <ibm/portio.h>
#elif MSDOS
#define HAVEFARPTR	1
#define INLINE_FP       1
#define FAR         _far
#elif RTEMS
  #define FAR
  #define HAVEFARPTR    1
  #define INLINE_FP     1
  #if defined(__i386__)
    #include <i386_io.h>
  #else
    #define outb(val,port)	/* outportb(port,val) */
    #define outw(val,port)	/* outport(port,val) */
  #endif
#endif

#if MSDOS | ELKS
#define MK_FP(seg,ofs)	((FARADDR)(((unsigned long)(seg) << 16L) | (unsigned)(ofs)))
#define EGA_BASE 	MK_FP(0xa000, 0)
#else
#define EGA_BASE 	((unsigned char *)0xa0000)
#endif

/* far ptr access to screen*/
#if HAVEFARPTR
typedef volatile unsigned char FAR * FARADDR;
#else
typedef unsigned long	FARADDR;
#endif

#if INLINE_FP
#define GETBYTE_FP(addr)	(*(FARADDR)(addr))              /* get byte at address*/
#define PUTBYTE_FP(addr,val)	((*(FARADDR)(addr)) = (val))    /* put byte at address*/
#define RMW_FP(addr)		((*(FARADDR)(addr)) |= 1)       /* read-modify-write */
#define ORBYTE_FP(addr,val)	((*(FARADDR)(addr)) |= (val))   /* or byte at address*/
#define ANDBYTE_FP(addr,val)	((*(FARADDR)(addr)) &= (val))   /* and byte at address*/
#else   /* for compilers with no FAR extension*/
extern unsigned char GETBYTE_FP(FARADDR);                       /* get byte at address*/
extern void PUTBYTE_FP(FARADDR,unsigned char);                  /* put byte at address*/
extern void RMW_FP(FARADDR);                                    /* read-modify-write */
extern void ORBYTE_FP(FARADDR,unsigned char);                   /* or byte at address*/
extern void ANDBYTE_FP(FARADDR,unsigned char);                  /* and byte at address*/
#endif

#if ELKS
#include <arch/io.h>
#elif _MINIX | MSDOS
#define	outb(v, p)	outb(p, v)
#define	outw(v, p)	outw(p, v)
#else
#define outb(val,port)	outportb(port,val)
#define outw(val,port)	outport(port,val)
extern int  inportb(int port);
extern void outportb(int port,unsigned char data);
extern void outport(int port,int data);
#endif

/* external routines*/
FARADDR		int10(int ax,int bx);

/* EGA/VGA planar-4 drivers in vgaplan4_*.c */
int		vga_init(PSD psd);
int		cga_init(PSD psd);
int		pc98_init(PSD psd);
void		pc98_drawhorzline(PSD psd,MWCOORD x1,MWCOORD x2, MWCOORD y,MWPIXELVAL c);
void	 	vga_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w,
			MWCOORD h,PSD srcpsd,MWCOORD srcx,MWCOORD srcy,int op);
#if HWINIT
void		ega_hwinit(void);       /* HWINIT direct hw init in vgainit.c*/
void		ega_hwterm(void);
#endif

#if SLOWVGA     /* use outb rather than outw instructions for older, slower VGA's*/
#define set_color(c)		{ outb (0, 0x3ce); outb (c, 0x3cf); }
#define set_enable_sr(mask)     { outb (1, 0x3ce); outb (mask, 0x3cf); }
#define select_mask() 		{ outb (8, 0x3ce); }
#define set_mask(mask)		{ outb (mask, 0x3cf); }
#define select_and_set_mask(mask) { outb (8, 0x3ce); outb (mask, 0x3cf); }
#define set_op(op) 		{ outb (3, 0x3ce); outb (op, 0x3cf); }
#define set_write_planes(mask)  { outb (2, 0x3c4); outb (mask, 0x3c5); }
#define set_read_plane(plane)	{ outb (4, 0x3ce); outb (plane, 0x3cf); }
#define set_mode(mode) 		{ outb (5, 0x3ce); outb (mode, 0x3cf); }
#else

/* use outw rather than outb instructions for new VGAs*/

/* Program the Set/Reset Register for drawing in color COLOR for write mode 0. */
#define set_color(c)		{ outw ((c)<<8, 0x3ce); }

/* Set the Enable Set/Reset Register. */
#define set_enable_sr(mask) 	{ outw (1|((mask)<<8), 0x3ce); }

/* Select the Bit Mask Register on the Graphics Controller. */
#define select_mask() 		{ outb (8, 0x3ce); }

/* Program the Bit Mask Register to affect only the pixels selected in
   MASK.  The Bit Mask Register must already have been selected with select_mask (). */
#define set_mask(mask)		{ outb (mask, 0x3cf); }
#define select_and_set_mask(mask) { outw (8|((mask)<<8), 0x3ce); }

/* Set the Data Rotate Register.  Bits 0-2 are rotate count, bits 3-4
   are logical operation (0=NOP, 1=AND, 2=OR, 3=XOR). */
#define set_op(op) 		{ outw (3|((op)<<8), 0x3ce); }

/* Set the Memory Plane Write Enable register. */
#define set_write_planes(mask)  { outw (2|((mask)<<8), 0x3c4); }

/* Set the Read Map Select register. */
#define set_read_plane(plane)	{ outw (4|((plane)<<8), 0x3ce); }

/* Set the Graphics Mode Register.  The write mode is in bits 0-1, the
   read mode is in bit 3. */
#define set_mode(mode) 		{ outw (5|((mode)<<8), 0x3ce); }

#endif /* SLOWVGA*/

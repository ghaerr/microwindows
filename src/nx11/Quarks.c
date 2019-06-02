/* $Xorg: Quarks.c,v 1.4 2000/08/17 19:44:51 cpqbld Exp $ */

/***********************************************************
Copyright 1987, 1988, 1990 by Digital Equipment Corporation, Maynard,

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/*

Copyright 1987, 1988, 1990, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/lib/X11/Quarks.c,v 1.3 2001/01/17 19:41:42 dawes Exp $ */

/*#include "Xlibint.h"*/
#include "nxlib.h"
#include "X11/Xresource.h"
#include <string.h> /* avoid bzero warning */

#ifndef	HAVE_BZERO
#define	bzero(ptr,n)		memset(ptr, 0, n)
#endif

/* Not cost effective, at least for vanilla MIT clients */
/* #define PERMQ */

typedef unsigned long Signature;
typedef unsigned long Entry;
#ifdef PERMQ
typedef unsigned char Bits;
#endif

static XrmQuark nextQuark = 1;	/* next available quark number */
static unsigned long quarkMask = 0;
static Entry zero = 0;
static Entry *quarkTable = &zero; /* crock */
static unsigned long quarkRehash;
static XrmString **stringTable = NULL;
#ifdef PERMQ
static Bits **permTable = NULL;
#endif
static XrmQuark nextUniq = -1;	/* next quark from XrmUniqueQuark */

#define QUANTUMSHIFT	8
#define QUANTUMMASK	((1 << QUANTUMSHIFT) - 1)
#define CHUNKPER	8
#define CHUNKMASK	((CHUNKPER << QUANTUMSHIFT) - 1)

#define LARGEQUARK	((Entry)0x80000000L)
#define QUARKSHIFT	18
#define QUARKMASK	((LARGEQUARK - 1) >> QUARKSHIFT)
#define XSIGMASK	((1L << QUARKSHIFT) - 1)

#define STRQUANTSIZE	(sizeof(XrmString) * (QUANTUMMASK + 1))
#ifdef PERMQ
#define QUANTSIZE	(STRQUANTSIZE + \
			 (sizeof(Bits) * ((QUANTUMMASK + 1) >> 3)))
#else
#define QUANTSIZE	STRQUANTSIZE
#endif

#define HASH(sig) ((sig) & quarkMask)
#define REHASHVAL(sig) ((((sig) % quarkRehash) + 2) | 1)
#define REHASH(idx,rehash) ((idx + rehash) & quarkMask)
#define NAME(q) stringTable[(q) >> QUANTUMSHIFT][(q) & QUANTUMMASK]
#ifdef PERMQ
#define BYTEREF(q) permTable[(q) >> QUANTUMSHIFT][((q) & QUANTUMMASK) >> 3]
#define ISPERM(q) (BYTEREF(q) & (1 << ((q) & 7)))
#define SETPERM(q) BYTEREF(q) |= (1 << ((q) & 7))
#define CLEARPERM(q) BYTEREF(q) &= ~(1 << ((q) & 7))
#endif

/* Permanent memory allocation */

#define WALIGN sizeof(unsigned long)
#define DALIGN sizeof(double)

#define NEVERFREETABLESIZE ((8192-12) & ~(DALIGN-1))
static char *neverFreeTable = NULL;
static int  neverFreeTableSize = 0;

static char *permalloc(length)
    register unsigned int length;
{
    char *ret;

    if (neverFreeTableSize < length) {
	if (length >= NEVERFREETABLESIZE)
	    return Xmalloc(length);
	if (! (ret = Xmalloc(NEVERFREETABLESIZE)))
	    return (char *) NULL;
	neverFreeTableSize = NEVERFREETABLESIZE;
	neverFreeTable = ret;
    }
    ret = neverFreeTable;
    neverFreeTable += length;
    neverFreeTableSize -= length;
    return(ret);
}

#ifndef WORD64
typedef struct {char a; double b;} TestType1;
typedef struct {char a; unsigned long b;} TestType2;
#endif

#ifdef XTHREADS
static char *_Xpermalloc();

char *Xpermalloc(length)
    unsigned int length;
{
    char *p;

    _XLockMutex(_Xglobal_lock);
    p = _Xpermalloc(length);
    _XUnlockMutex(_Xglobal_lock);
    return p;
}
#define Xpermalloc _Xpermalloc

static
#endif /* XTHREADS */
char *Xpermalloc(length)
    unsigned int length;
{
    int i;

    if (neverFreeTableSize && length < NEVERFREETABLESIZE) {
#ifndef WORD64
	if ((sizeof(TestType1) !=
	     (sizeof(TestType2) - sizeof(unsigned long) + sizeof(double))) &&
	    !(length & (DALIGN-1)) &&
	    ((i = (NEVERFREETABLESIZE - neverFreeTableSize) & (DALIGN-1)))) {
	    neverFreeTableSize -= DALIGN - i;
	    neverFreeTable += DALIGN - i;
	} else
#endif
	    if ((i = (NEVERFREETABLESIZE - neverFreeTableSize) & (WALIGN-1))) {
		neverFreeTableSize -= WALIGN - i;
		neverFreeTable += WALIGN - i;
	    }
    }
    return permalloc(length);
}

static Bool
ExpandQuarkTable()
{
    unsigned long oldmask, newmask;
    register char c, *s;
    register Entry *oldentries, *entries;
    register Entry entry;
    register int oldidx, newidx, rehash;
    Signature sig;
    XrmQuark q;

    oldentries = quarkTable;
    if ((oldmask = quarkMask))
	newmask = (oldmask << 1) + 1;
    else {
	if (!stringTable) {
	    stringTable = (XrmString **)Xmalloc(sizeof(XrmString *) *
						CHUNKPER);
	    if (!stringTable)
		return False;
	    stringTable[0] = (XrmString *)NULL;
	}
#ifdef PERMQ
	if (!permTable)
	    permTable = (Bits **)Xmalloc(sizeof(Bits *) * CHUNKPER);
	if (!permTable)
	    return False;
#endif
	stringTable[0] = (XrmString *)Xpermalloc(QUANTSIZE);
	if (!stringTable[0])
	    return False;
#ifdef PERMQ
	permTable[0] = (Bits *)((char *)stringTable[0] + STRQUANTSIZE);
#endif
	newmask = 0x1ff;
    }
    entries = (Entry *)Xmalloc(sizeof(Entry) * (newmask + 1));
    if (!entries)
	return False;
    bzero((char *)entries, sizeof(Entry) * (newmask + 1));
    quarkTable = entries;
    quarkMask = newmask;
    quarkRehash = quarkMask - 2;
    for (oldidx = 0; oldidx <= oldmask; oldidx++) {
	if ((entry = oldentries[oldidx])) {
	    if (entry & LARGEQUARK)
		q = entry & (LARGEQUARK-1);
	    else
		q = (entry >> QUARKSHIFT) & QUARKMASK;
	    for (sig = 0, s = NAME(q); (c = *s++); )
		sig = (sig << 1) + c;
	    newidx = HASH(sig);
	    if (entries[newidx]) {
		rehash = REHASHVAL(sig);
		do {
		    newidx = REHASH(newidx, rehash);
		} while (entries[newidx]);
	    }
	    entries[newidx] = entry;
	}
    }
    if (oldmask)
	Xfree((char *)oldentries);
    return True;
}

#if NeedFunctionPrototypes
XrmQuark _XrmInternalStringToQuark(
    register _Xconst char *name, register int len, register Signature sig,
    Bool permstring)
#else
XrmQuark _XrmInternalStringToQuark(name, len, sig, permstring)
    register XrmString name;
    register int len;
    register Signature sig;
    Bool permstring;
#endif
{
    register XrmQuark q;
    register Entry entry;
    register int idx, rehash;
    register int i;
    register char *s1, *s2;
    char *new;

    rehash = 0;
    idx = HASH(sig);
    _XLockMutex(_Xglobal_lock);
    while ((entry = quarkTable[idx])) {
	if (entry & LARGEQUARK)
	    q = entry & (LARGEQUARK-1);
	else {
	    if ((entry - sig) & XSIGMASK)
		goto nomatch;
	    q = (entry >> QUARKSHIFT) & QUARKMASK;
	}
	for (i = len, s1 = (char *)name, s2 = NAME(q); --i >= 0; ) {
	    if (*s1++ != *s2++)
		goto nomatch;
	}
	if (*s2) {
nomatch:    if (!rehash)
		rehash = REHASHVAL(sig);
	    idx = REHASH(idx, rehash);
	    continue;
	}
#ifdef PERMQ
	if (permstring && !ISPERM(q)) {
	    Xfree(NAME(q));
	    NAME(q) = (char *)name;
	    SETPERM(q);
	}
#endif
	_XUnlockMutex(_Xglobal_lock);
	return q;
    }
    if (nextUniq == nextQuark)
	goto fail;
    if ((nextQuark + (nextQuark >> 2)) > quarkMask) {
	if (!ExpandQuarkTable())
	    goto fail;
	_XUnlockMutex(_Xglobal_lock);
	return _XrmInternalStringToQuark(name, len, sig, permstring);
    }
    q = nextQuark;
    if (!(q & QUANTUMMASK)) {
	if (!(q & CHUNKMASK)) {
	    if (!(new = Xrealloc((char *)stringTable,
				 sizeof(XrmString *) *
				 ((q >> QUANTUMSHIFT) + CHUNKPER))))
		goto fail;
	    stringTable = (XrmString **)new;
#ifdef PERMQ
	    if (!(new = Xrealloc((char *)permTable,
				 sizeof(Bits *) *
				 ((q >> QUANTUMSHIFT) + CHUNKPER))))
		goto fail;
	    permTable = (Bits **)new;
#endif
	}
	new = Xpermalloc(QUANTSIZE);
	if (!new)
	    goto fail;
	stringTable[q >> QUANTUMSHIFT] = (XrmString *)new;
#ifdef PERMQ
	permTable[q >> QUANTUMSHIFT] = (Bits *)(new + STRQUANTSIZE);
#endif
    }
    if (!permstring) {
	s2 = (char *)name;
#ifdef PERMQ
	name = Xmalloc(len+1);
#else
	name = permalloc(len+1);
#endif
	if (!name)
	    goto fail;
	for (i = len, s1 = (char *)name; --i >= 0; )
	    *s1++ = *s2++;
	*s1++ = '\0';
#ifdef PERMQ
	CLEARPERM(q);
    }
    else {
	SETPERM(q);
#endif
    }
    NAME(q) = (char *)name;
    if (q <= QUARKMASK)
	entry = (q << QUARKSHIFT) | (sig & XSIGMASK);
    else
	entry = q | LARGEQUARK;
    quarkTable[idx] = entry;
    nextQuark++;
    _XUnlockMutex(_Xglobal_lock);
    return q;
 fail:
    _XUnlockMutex(_Xglobal_lock);
    return NULLQUARK;
}

#if NeedFunctionPrototypes
XrmQuark XrmStringToQuark(
    _Xconst char *name)
#else
XrmQuark XrmStringToQuark(name)
    XrmString name;
#endif
{
    register char c, *tname;
    register Signature sig = 0;

    if (!name)
	return (NULLQUARK);
    
    for (tname = (char *)name; (c = *tname++); )
	sig = (sig << 1) + c;

    return _XrmInternalStringToQuark(name, tname-(char *)name-1, sig, False);
}

#if NeedFunctionPrototypes
XrmQuark XrmPermStringToQuark(
    _Xconst char *name)
#else
XrmQuark XrmPermStringToQuark(name)
    XrmString name;
#endif
{
    register char c, *tname;
    register Signature sig = 0;

    if (!name)
	return (NULLQUARK);

    for (tname = (char *)name; (c = *tname++); )
	sig = (sig << 1) + c;

    return _XrmInternalStringToQuark(name, tname-(char *)name-1, sig, True);
}

XrmQuark XrmUniqueQuark()
{
    XrmQuark q;

    _XLockMutex(_Xglobal_lock);
    if (nextUniq == nextQuark)
	q = NULLQUARK;
    else
	q = nextUniq--;
    _XUnlockMutex(_Xglobal_lock);
    return q;
}

XrmString XrmQuarkToString(quark)
    register XrmQuark quark;
{
    XrmString s;

    _XLockMutex(_Xglobal_lock);
    if (quark <= 0 || quark >= nextQuark)
    	s = NULLSTRING;
    else {
#ifdef PERMQ
	/* We have to mark the quark as permanent, since the caller might hold
	 * onto the string pointer forver.
	 */
	SETPERM(quark);
#endif
	s = NAME(quark);
    }
    _XUnlockMutex(_Xglobal_lock);
    return s;
}

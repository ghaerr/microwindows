/***********************************************************************

bitvector.h - some macros for dealing with bitvectors

Copyright (C) 1991 Dean Rubine

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License. See ../COPYING for
the full agreement.

 **********************************************************************/

/*
  Bit vector package

  Used so that it's easier when we need more than 8*sizeof(int) bits
  in a the vector.

  Usage:
	Before including this file define the identifier BITS_PER_VECTOR

	BITS_PER_VECTOR must be one of the following values:

		16
		32
		64
		128

	The high tech preprocessor hacking is sure to be nonportable.  The
	use of include in this file is not to include files, it is to
	print out error messages.  Ugly, I know, but what can I do?

	You may include this file more than one in a single C file!
	By default, when this file is included it defines a type
	BitVector.  You may change the name of this type (necessary
	to avoid redefinitions when included more than one) by defining:

	#define BV_TYPE_NAME	MyBitVectorTypeName

	The usual sequence for including this file is thus:

	#undef BV_TYPE_NAME
	#undef BITS_PER_VECTOR
	#define BV_TYPE_NAME	BVTypeName
	#define BITS_PER_VECTOR	how-many-bits-per-vector

	WARNING: Once the file is re-included do not attempt to manipulate any
	other vectors besides the newest type for the rest of the file or until
	the file is included again.
*/

/*
  -------------     check BITS_PER_VECTOR     -----------------
 */
#ifndef BITS_PER_VECTOR
#	define	BITS_PER_VECTOR  32
#endif

#if (BITS_PER_VECTOR != 16) && (BITS_PER_VECTOR != 32) && (BITS_PER_VECTOR != 64) && (BITS_PER_VECTOR != 128)

#	include "****** illegal value for BITS_PER_VECTOR  ******"

#endif

/*
  -------------      machine dependent stuff     -----------------
 */

#ifndef	BITS_PER_INT


#	ifdef unix
#		define BITS_PER_INT	32
#	else
#		define BITS_PER_INT	16  /* IBM XT Lattice C */
#	endif

#	define	BV_CHECK_MACHINE_ASSUMPTIONS() \
	 	if(BITS_PER_INT != 8*sizeof(int)) \
			error("BV_CHECK_ASSUMPTIONS");

#endif

/*
  ---- If this file has been included already, redefine everything ----
 */

#	undef	BV_INDEX_MASK
#	undef	BV_INDEX_SHIFT
#	undef	INTS_PER_VECTOR
#	undef	VECTOR_SIZE_CHECK
#	undef	SET_BIT_VECTOR
#	undef	IS_SET
#	undef	ASSIGN_BIT_VECTOR
#	undef	CLEAR_BIT_VECTOR
#	undef	BIT_SET
#	undef	BIT_CLEAR

/* 
  --------------- round up to int size -------------------
 */

#if BITS_PER_VECTOR < BITS_PER_INT
#	undef	BITS_PER_VECTOR
#	define	BITS_PER_VECTOR	BITS_PER_INT
#endif

/*
  ------------- Compute index shift and mask to avoid division -----
 */

#define	BV_INDEX_MASK	(BITS_PER_INT - 1)

#if BITS_PER_INT==16
#	define	BV_INDEX_SHIFT	4
#endif

#if BITS_PER_INT==32
#	define	BV_INDEX_SHIFT	5
#endif

#ifndef BV_INDEX_SHIFT
#	include "****** bad value for BITS_PER_INT  ******"
#endif

/*
 ------------- Compute INTS_PER_VECTOR ------------------
 */

#if BITS_PER_INT==BITS_PER_VECTOR
#	define	INTS_PER_VECTOR	1
#else
#	if 2*BITS_PER_INT==BITS_PER_VECTOR
#		define	INTS_PER_VECTOR	2
#	else
#		define	INTS_PER_VECTOR	(BITS_PER_VECTOR / BITS_PER_INT)
#	endif
#endif 


#define	BV_SIZE_CHECK(nbits_needed) \
	if(nbits_needed > BITS_PER_VECTOR) \
		error("%s line %d - %d bits needed, %d is vector size", \
			__FILE__, __LINE__, nbits_needed, BITS_PER_VECTOR);

#ifndef BV_TYPE_NAME
#	define	BV_TYPE_NAME	BitVector
#endif

/*
 ------------- Optimize INTS_PER_VECTOR=1 case
 */

#if INTS_PER_VECTOR==1

typedef int BV_TYPE_NAME[1];

#define	CLEAR_BIT_VECTOR(v)	( (v)[0] = 0 )
#define	SET_BIT_VECTOR(v)	( (v)[0] = -1 )		/* assumes 2's comp */
#define	BIT_SET(bit, v)		( (v)[0] |= (1 << (bit)) )
#define	BIT_CLEAR(bit, v)	( (v)[0] &= ~(1 << (bit)) )
#define	IS_SET(bit, v)		( ((v)[0] >> (bit)) & 01 )
#define	ASSIGN_BIT_VECTOR(v1,v2) ( (v1)[0] = (v2)[0] )

#else

/*
 ------------- Optimize INTS_PER_VECTOR=2 case -------
 */

#if INTS_PER_VECTOR==2

    typedef int BV_TYPE_NAME[2];

#   define	CLEAR_BIT_VECTOR(v) ( (v)[0] = (v)[1] = 0 )
#   define	SET_BIT_VECTOR(v)   ( (v)[0] = (v)[1] = -1 ) /* 2's comp */
#    define	ASSIGN_BIT_VECTOR(v1,v2) \
				   ( (v1)[0] = (v2)[0], (v1)[1] = (v2)[1] )

#else

/*
 ------------- general case -------------------
 */

     typedef int BV_TYPE_NAME[INTS_PER_VECTOR];

#    define	CLEAR_BIT_VECTOR(v)	(  ClearBitVector(INTS_PER_VECTOR, v) )
#    define	SET_BIT_VECTOR(v)	(  SetBitVector(INTS_PER_VECTOR, v) )
#    define	ASSIGN_BIT_VECTOR(v1,v2) \
				   ( AssignBitVector(INTS_PER_VECTOR, v1, v2) )

#endif


#define	BIT_SET(bit, v)	\
	( (v[bit>>BV_INDEX_SHIFT]) |= (1 << (bit&BV_INDEX_MASK)) )

#define	BIT_CLEAR(bit, v) \
	( (v[bit>>BV_INDEX_SHIFT]) &= ~(1 << (bit&BV_INDEX_MASK)) )

#define	IS_SET(bit, v) \
	( ((v[bit>>BV_INDEX_SHIFT]) >> (bit&BV_INDEX_MASK)) & 01 )

#endif

/* TODO: make efficient */

#define	OR(v, v1, v2) ( BitVectorOr((v), (v1), (v2), INTS_PER_VECTOR) )
#define	AND(v, v1, v2) ( BitVectorAnd((v), (v1), (v2), INTS_PER_VECTOR) )
#define	NO_BITS_SET(v)	( BitVectorNoBitsSet( (v), INTS_PER_VECTOR ) )

int bitcount();	/* max, bv */
char *BitVectorToString(); /* max, bv */
void StringToBitVector(); /* string, max, bv */
int BitVectorDeQ();	  /* element = BitVectorDeQ(max, bv); */

int *BitVectorOr();
int *BitVectorAnd();
int BitVectorNoBitsSet();

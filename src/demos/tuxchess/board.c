/*
 *	BOARD.C
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 */


#include "defs.h"
#include "data.h"
#include "protos.h"


/* init() sets the board to the initial game state */

void init()
{
	int i;

	for (i = 0; i < 64; ++i) {
		color[i] = init_color[i];
		piece[i] = init_piece[i];
	}
	side = LIGHT;
	xside = DARK;
	castle = 15;
	ep = -1;
	fifty = 0;
	ply = 0;
	hply = 0;
	first_move[0] = 0;
}


/* in_check() returns TRUE if side s is in check and FALSE
   otherwise. It just scans the board to find side s's king
   and calls attack() to see if it's being attacked. */

BOOL in_check(int s)
{
	int i;

	for (i = 0; i < 64; ++i)
		if (piece[i] == KING && color[i] == s)
			return attack(i, s ^ 1);
	return TRUE;  /* shouldn't get here */
}


/* attack() returns TRUE if square sq is being attacked by side
   s and FALSE otherwise. */

BOOL attack(int sq, int s)
{
	int i, j, n;

	for (i = 0; i < 64; ++i)
		if (color[i] == s) {
			if (piece[i] == PAWN) {
				if (s == LIGHT) {
					if (COL(i) != 0 && i - 9 == sq)
						return TRUE;
					if (COL(i) != 7 && i - 7 == sq)
						return TRUE;
				}
				else {
					if (COL(i) != 0 && i + 7 == sq)
						return TRUE;
					if (COL(i) != 7 && i + 9 == sq)
						return TRUE;
				}
			}
			else
				for (j = 0; j < offsets[piece[i]]; ++j)
					for (n = i;;) {
						n = mailbox[mailbox64[n] + offset[piece[i]][j]];
						if (n == -1)
							break;
						if (n == sq)
							return TRUE;
						if (color[n] != EMPTY)
							break;
						if (!slide[piece[i]])
							break;
					}
		}
	return FALSE;
}


/* gen() generates pseudo-legal moves for the current position.
   It scans the board to find friendly pieces and then determines
   what squares they attack. When it finds a piece/square
   combination, it calls gen_push to put the move on the "move
   stack." */

void gen()
{
	int i, j, n;

	/* so far, we have no moves for the current ply */
	first_move[ply + 1] = first_move[ply];

	for (i = 0; i < 64; ++i)
		if (color[i] == side) {
			if (piece[i] == PAWN) {
				if (side == LIGHT) {
					if (COL(i) != 0 && color[i - 9] == DARK)
						gen_push(i, i - 9, 17);
					if (COL(i) != 7 && color[i - 7] == DARK)
						gen_push(i, i - 7, 17);
					if (color[i - 8] == EMPTY) {
						gen_push(i, i - 8, 16);
						if (i >= 48 && color[i - 16] == EMPTY)
							gen_push(i, i - 16, 24);
					}
				}
				else {
					if (COL(i) != 0 && color[i + 7] == LIGHT)
						gen_push(i, i + 7, 17);
					if (COL(i) != 7 && color[i + 9] == LIGHT)
						gen_push(i, i + 9, 17);
					if (color[i + 8] == EMPTY) {
						gen_push(i, i + 8, 16);
						if (i <= 15 && color[i + 16] == EMPTY)
							gen_push(i, i + 16, 24);
					}
				}
			}
			else
				for (j = 0; j < offsets[piece[i]]; ++j)
					for (n = i;;) {
						n = mailbox[mailbox64[n] + offset[piece[i]][j]];
						if (n == -1)
							break;
						if (color[n] != EMPTY) {
							if (color[n] == xside)
								gen_push(i, n, 1);
							break;
						}
						gen_push(i, n, 0);
						if (!slide[piece[i]])
							break;
					}
		}

	/* generate castle moves */
	if (side == LIGHT) {
		if (castle & 1)
			gen_push(E1, G1, 2);
		if (castle & 2)
			gen_push(E1, C1, 2);
	}
	else {
		if (castle & 4)
			gen_push(E8, G8, 2);
		if (castle & 8)
			gen_push(E8, C8, 2);
	}
	
	/* generate en passant moves */
	if (ep != -1) {
		if (side == LIGHT) {
			if (COL(ep) != 0 && color[ep + 7] == LIGHT && piece[ep + 7] == PAWN)
				gen_push(ep + 7, ep, 21);
			if (COL(ep) != 7 && color[ep + 9] == LIGHT && piece[ep + 9] == PAWN)
				gen_push(ep + 9, ep, 21);
		}
		else {
			if (COL(ep) != 0 && color[ep - 9] == DARK && piece[ep - 9] == PAWN)
				gen_push(ep - 9, ep, 21);
			if (COL(ep) != 7 && color[ep - 7] == DARK && piece[ep - 7] == PAWN)
				gen_push(ep - 7, ep, 21);
		}
	}
}


/* gen_caps() is basically a copy of gen() that's modified to
   only generate capture and promote moves. It's used by the
   quiescence search. */

void gen_caps()
{
	int i, j, n;

	first_move[ply + 1] = first_move[ply];
	for (i = 0; i < 64; ++i)
		if (color[i] == side) {
			if (piece[i]==PAWN) {
				if (side == LIGHT) {
					if (COL(i) != 0 && color[i - 9] == DARK)
						gen_push(i, i - 9, 17);
					if (COL(i) != 7 && color[i - 7] == DARK)
						gen_push(i, i - 7, 17);
					if (i <= 15 && color[i - 8] == EMPTY)
						gen_push(i, i - 8, 16);
				}
				if (side == DARK) {
					if (COL(i) != 0 && color[i + 7] == LIGHT)
						gen_push(i, i + 7, 17);
					if (COL(i) != 7 && color[i + 9] == LIGHT)
						gen_push(i, i + 9, 17);
					if (i >= 48 && color[i + 8] == EMPTY)
						gen_push(i, i + 8, 16);
				}
			}
			else
				for (j = 0; j < offsets[piece[i]]; ++j)
					for (n = i;;) {
						n = mailbox[mailbox64[n] + offset[piece[i]][j]];
						if (n == -1)
							break;
						if (color[n] != EMPTY) {
							if (color[n] == xside)
								gen_push(i, n, 1);
							break;
						}
						if (!slide[piece[i]])
							break;
					}
		}
	if (ep != -1) {
		if (side == LIGHT) {
			if (COL(ep) != 0 && color[ep + 7] == LIGHT && piece[ep + 7] == PAWN)
				gen_push(ep + 7, ep, 21);
			if (COL(ep) != 7 && color[ep + 9] == LIGHT && piece[ep + 9] == PAWN)
				gen_push(ep + 9, ep, 21);
		}
		else {
			if (COL(ep) != 0 && color[ep - 9] == DARK && piece[ep - 9] == PAWN)
				gen_push(ep - 9, ep, 21);
			if (COL(ep) != 7 && color[ep - 7] == DARK && piece[ep - 7] == PAWN)
				gen_push(ep - 7, ep, 21);
		}
	}
}


/* gen_push() puts a move on the move stack, unless it's a
   pawn promotion that needs to be handled by gen_promote().
   It also assigns a score to the move for alpha-beta move
   ordering. If the move is a capture, it uses MVV/LVA
   (Most Valuable Victim/Least Valuable Attacker). Otherwise,
   it uses the move's history heuristic value. Note that
   1,000,000 is added to a capture move's score, so it
   always gets ordered above a "normal" move. */

void gen_push(int from, int to, int bits)
{
	gen_t *g;
	
	if (bits & 16) {
		if (side == LIGHT) {
			if (to <= H8) {
				gen_promote(from, to, bits);
				return;
			}
		}
		else {
			if (to >= A1) {
				gen_promote(from, to, bits);
				return;
			}
		}
	}
	g = &gen_dat[first_move[ply + 1]++];
	g->m.b.from = (char)from;
	g->m.b.to = (char)to;
	g->m.b.promote = 0;
	g->m.b.bits = (char)bits;
	if (color[to] != EMPTY)
		g->score = 1000000 + (piece[to] * 10) - piece[from];
	else
		g->score = history[from][to];
}


/* gen_promote() is just like gen_push(), only it puts 4 moves
   on the move stack, one for each possible promotion piece */

void gen_promote(int from, int to, int bits)
{
	int i;
	gen_t *g;
	
	for (i = KNIGHT; i <= QUEEN; ++i) {
		g = &gen_dat[first_move[ply + 1]++];
		g->m.b.from = (char)from;
		g->m.b.to = (char)to;
		g->m.b.promote = (char)i;
		g->m.b.bits = (char)(bits | 32);
		g->score = 1000000 + (i * 10);
	}
}


/* makemove() makes a move. If the move is illegal, it
   undoes whatever it did and returns FALSE. Otherwise, it
   returns TRUE. */

BOOL makemove(move_bytes m)
{
	
	/* test to see if a castle move is legal and move the rook
	   (the king is moved with the usual move code later) */
	if (m.bits & 2) {
		int from, to;

		if (in_check(side))
			return FALSE;
		switch (m.to) {
			case 62:
				if (color[F1] != EMPTY || color[G1] != EMPTY ||
						attack(F1, xside) || attack(G1, xside))
					return FALSE;
				from = H1;
				to = F1;
				break;
			case 58:
				if (color[B1] != EMPTY || color[C1] != EMPTY || color[D1] != EMPTY ||
						attack(C1, xside) || attack(D1, xside))
					return FALSE;
				from = A1;
				to = D1;
				break;
			case 6:
				if (color[F8] != EMPTY || color[G8] != EMPTY ||
						attack(F8, xside) || attack(G8, xside))
					return FALSE;
				from = H8;
				to = F8;
				break;
			case 2:
				if (color[B8] != EMPTY || color[C8] != EMPTY || color[D8] != EMPTY ||
						attack(C8, xside) || attack(D8, xside))
					return FALSE;
				from = A8;
				to = D8;
				break;
			default:  /* shouldn't get here */
				from = -1;
				to = -1;
				break;
		}
		color[to] = color[from];
		piece[to] = piece[from];
		color[from] = EMPTY;
		piece[from] = EMPTY;
	}

	/* back up information so we can take the move back later. */
	hist_dat[hply].m.b = m;
	hist_dat[hply].capture = piece[(int)m.to];
	hist_dat[hply].castle = castle;
	hist_dat[hply].ep = ep;
	hist_dat[hply].fifty = fifty;
	++ply;
	++hply;

	/* update the castle, en passant, and
	   fifty-move-draw variables */
	castle &= castle_mask[(int)m.from] & castle_mask[(int)m.to];
	if (m.bits & 8) {
		if (side == LIGHT)
			ep = m.to + 8;
		else
			ep = m.to - 8;
	}
	else
		ep = -1;
	if (m.bits & 17)
		fifty = 0;
	else
		++fifty;

	/* move the piece */
	color[(int)m.to] = side;
	if (m.bits & 32)
		piece[(int)m.to] = m.promote;
	else
		piece[(int)m.to] = piece[(int)m.from];
	color[(int)m.from] = EMPTY;
	piece[(int)m.from] = EMPTY;

	/* erase the pawn if this is an en passant move */
	if (m.bits & 4) {
		if (side == LIGHT) {
			color[m.to + 8] = EMPTY;
			piece[m.to + 8] = EMPTY;
		}
		else {
			color[m.to - 8] = EMPTY;
			piece[m.to - 8] = EMPTY;
		}
	}

	/* switch sides and test for legality (if we can capture
	   the other guy's king, it's an illegal position and
	   we need to take the move back) */
	side ^= 1;
	xside ^= 1;
	if (in_check(xside)) {
		takeback();
		return FALSE;
	}
	return TRUE;
}


/* takeback() is very similar to makemove(), only backwards :)  */

void takeback()
{
	move_bytes m;

	side ^= 1;
	xside ^= 1;
	--ply;
	--hply;
	m = hist_dat[hply].m.b;
	castle = hist_dat[hply].castle;
	ep = hist_dat[hply].ep;
	fifty = hist_dat[hply].fifty;
	color[(int)m.from] = side;
	if (m.bits & 32)
		piece[(int)m.from] = PAWN;
	else
		piece[(int)m.from] = piece[(int)m.to];
	if (hist_dat[hply].capture == EMPTY) {
		color[(int)m.to] = EMPTY;
		piece[(int)m.to] = EMPTY;
	}
	else {
		color[(int)m.to] = xside;
		piece[(int)m.to] = hist_dat[hply].capture;
	}
	if (m.bits & 2) {
		int from, to;

		switch(m.to) {
			case 62:
				from = F1;
				to = H1;
				break;
			case 58:
				from = D1;
				to = A1;
				break;
			case 6:
				from = F8;
				to = H8;
				break;
			case 2:
				from = D8;
				to = A8;
				break;
			default:  /* shouldn't get here */
				from = -1;
				to = -1;
				break;
		}
		color[to] = side;
		piece[to] = ROOK;
		color[from] = EMPTY;
		piece[from] = EMPTY;
	}
	if (m.bits & 4) {
		if (side == LIGHT) {
			color[m.to + 8] = xside;
			piece[m.to + 8] = PAWN;
		}
		else {
			color[m.to - 8] = xside;
			piece[m.to - 8] = PAWN;
		}
	}
}

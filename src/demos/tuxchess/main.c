/*
 *	TuxChess - Copyright (c) 2002, Steven J. Merrifield
 *	Chess engine - Copyright 1997 Tom Kerrigan
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/timeb.h>
#include "defs.h"
#include "data.h"
#include "protos.h"

#define MWINCLUDECOLORS
#include "nano-X.h"
#define TITLE 		"TuxChess"
#define SCANCODES	64
#define BM_WIDTH 	394
#define BM_HEIGHT 	413


struct keycolumn {
        short xoffset;
        short scancode;
};

struct keyrow {
        short yoffset;
        short height;
        struct keycolumn columns[9];
};

struct keyrow keyrows[8] = {
        {0, 49, 
         {{0,0}, {49,1}, {98,2}, {147,3}, {196,4}, {245,5}, {294,6}, {343,7}, {999,-1}}},
        {49, 49, 
         {{0,8}, {49,9}, {98,10}, {147,11}, {196,12}, {245,13}, {294,14}, {343,15}, {999,-1}}},
        {98, 49, 
         {{0,16}, {49,17}, {98,18}, {147,19}, {196,20}, {245,21}, {294,22}, {343,23}, {999,-1}} },
        {147, 49, 
         {{0,24}, {49,25}, {98,26}, {147,27}, {196,28}, {245,29}, {294,30}, {343,31}, {999,-1}}},
        {196, 49, 
         {{0,32}, {49,33}, {98,34}, {147,35}, {196,36}, {245,37}, {294,38}, {343,39}, {999,-1}}},
        {245, 49, 
         {{0,40}, {49,41}, {98,42}, {147,43}, {196,44}, {245,45}, {294,46}, {343,47}, {999,-1}}},
        {294, 49, 
         {{0,48}, {49,49}, {98,50}, {147,51}, {196,52}, {245,53}, {294,54}, {343,55}, {999,-1}}},
        {343, 49, 
         {{0,56}, {49,57}, {98,58}, {147,59}, {196,60}, {245,61}, {294,62}, {343,63}, {999,-1}}}
};

static char *board_position[SCANCODES] = {
	"A8","B8","C8","D8","E8","F8","G8","H8",
	"A7","B7","C7","D7","E7","F7","G7","H7",
	"A6","B6","C6","D6","E6","F6","G6","H6",
	"A5","B5","C5","D5","E5","F5","G5","H5",
	"A4","B4","C4","D4","E4","F4","G4","H4",
	"A3","B3","C3","D3","E3","F3","G3","H3",
	"A2","B2","C2","D2","E2","F2","G2","H2",
	"A1","B1","C1","D1","E1","F1","G1","H1"};


static GR_WINDOW_ID master;	/* id for whole window */             
static GR_WINDOW_ID board;	/* id for board area */
static GR_WINDOW_ID text;	/* id for text area */       

static GR_GC_ID	gc;             /* board graphics context */
static GR_GC_ID text_gc;	/* text area graphics context */

GR_IMAGE_ID     board_image_id;         
GR_IMAGE_INFO   board_info;         
int board_w,board_h;

GR_IMAGE_ID     w_p_image_id;	/* white pawn */
GR_IMAGE_INFO   w_p_info;         
int w_p_w,w_p_h;

GR_IMAGE_ID     w_n_image_id;	/* white knight */
GR_IMAGE_INFO   w_n_info;         
int w_n_w,w_n_h;

GR_IMAGE_ID	w_b_image_id;	/* white bishop */
GR_IMAGE_INFO	w_b_info;
int w_b_w,w_b_h;

GR_IMAGE_ID	w_r_image_id;	/* white rook */
GR_IMAGE_INFO	w_r_info;
int w_r_w,w_r_h;

GR_IMAGE_ID	w_q_image_id;	/* white queen */
GR_IMAGE_INFO	w_q_info;
int w_q_w,w_q_h;

GR_IMAGE_ID	w_k_image_id;	/* white king */
GR_IMAGE_INFO	w_k_info;
int w_k_w,w_k_h;

GR_IMAGE_ID     b_p_image_id;	/* black pawn */
GR_IMAGE_INFO   b_p_info;         
int b_p_w,b_p_h;

GR_IMAGE_ID     b_n_image_id;	/* black knight */
GR_IMAGE_INFO   b_n_info;         
int b_n_w,b_n_h;

GR_IMAGE_ID	b_b_image_id;	/* black bishop */
GR_IMAGE_INFO	b_b_info;
int b_b_w,b_b_h;

GR_IMAGE_ID	b_r_image_id;	/* black rook */
GR_IMAGE_INFO	b_r_info;
int b_r_w,b_r_h;

GR_IMAGE_ID	b_q_image_id;	/* black queen */
GR_IMAGE_INFO	b_q_info;
int b_q_w,b_q_h;

GR_IMAGE_ID	b_k_image_id;	/* black king */
GR_IMAGE_INFO	b_k_info;
int b_k_w,b_k_h;


extern int load_images(void);
int start_square = 1;
char st_sq[3];
int from = 999;
int to = 999;

/* ***********************************************************/
/* get_ms() returns the milliseconds elapsed since midnight,
   January 1, 1970. */
BOOL ftime_ok = FALSE;  /* does ftime return milliseconds? */
int get_ms()
{
	struct timeb timebuffer;
	ftime(&timebuffer);
	if (timebuffer.millitm != 0)
		ftime_ok = TRUE;
	return (timebuffer.time * 1000) + timebuffer.millitm;
}

 
/* ***********************************************************/
static void gprintf(char s[])
{
	static char lasttext[128];

        GrSetGCForeground(text_gc,BLACK);
        GrFillRect(text, text_gc, 0, 0, 394,20);
        GrSetGCForeground(text_gc,WHITE);
        GrSetGCBackground(text_gc,BLACK);
	if (!s)
		s = lasttext;
        GrText(text, text_gc, 5, 14, s, strlen(s),0);
	if (s != lasttext)
		strcpy(lasttext, s);
}     


/* ***********************************************************/
static void process_scancode(int scancode)
{
	char sq[3];
	char fin_sq[3];
	int i;
	char temp[5];

	strcpy(sq,"");
	for (i=0;i<SCANCODES;i++)
	{
		if (i == scancode)
		{
			strcpy(sq,board_position[i]);
			break;
		}
	}


	if (start_square)
	{
		gprintf(sq);
		strcpy(st_sq,sq);
		start_square = 0;
		from = scancode;
		to = 999;
	}
	else
	{
		strcpy(fin_sq,sq);
		start_square = 1;
		to = scancode;
		sprintf(temp,"%s%s",st_sq,fin_sq);
		gprintf(temp);
	}
}

/* ***********************************************************/
/* borrowed from nxkbd */
static void mouse_hit(int x, int y) 
{
        int row, column;

        for (row = 0; row < 8; row++) {
                if (y >= keyrows[row].yoffset &&
		    y < keyrows[row].yoffset+keyrows[row].height) {
                        for (column = 0; column < 8; column++) {
                                if (keyrows[row].columns[column].xoffset == 999) {
                                        fprintf(stderr, "off end of row\n");
                                        return;
                                }
                                if (x < keyrows[row].columns[column + 1].xoffset) {
                                        int scancode = keyrows[row].columns[column].scancode;
                                        process_scancode(scancode);
                                        return;
                                }
                        }
                }
        }
        fprintf(stderr, "off bottom\n");
}


/* ***********************************************************/
int main(int argc, char* argv[])
{
        GR_EVENT event;
	GR_WM_PROPERTIES props;

	int computer_side;
	char s[256];
	int i;
	BOOL found;
	char temp[50];	

        if (GrOpen() < 0) {
                fprintf(stderr, "tuxchess: cannot open graphics\n");
                exit(1);
        }

	load_images();

        master = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, BM_WIDTH, BM_HEIGHT, 0, WHITE, WHITE);
        board = GrNewWindow((GR_WINDOW_ID) master, 0, 0, 394, 394, 0, WHITE, WHITE);
        text = GrNewWindow((GR_WINDOW_ID) master, 0, 393, 394, 20, 0, WHITE, BLACK); 

        GrSelectEvents(master, GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_BUTTON_DOWN);

	props.flags = GR_WM_FLAGS_PROPS | GR_WM_FLAGS_TITLE;
	props.props = GR_WM_PROPS_BORDER | GR_WM_PROPS_CAPTION | GR_WM_PROPS_CLOSEBOX;
	props.title = TITLE;
	GrSetWMProperties(master, &props);

        GrMapWindow(master);
        GrMapWindow(board);
        GrMapWindow(text);
                                   
        gc = GrNewGC();
        text_gc = GrNewGC();

	init();
	gen();
	max_time = 1 << 25;
	max_depth = 4;

	if (argc > 1)
		computer_side = side;	/* computer plays white */
	else
	{
		computer_side = EMPTY;	/* human plays white */
		gprintf("Make a move...");
	}

        for (;;) 
	{

		if (side == computer_side) 
		{
			/* think about the move and make it */
			think(0);
			if (!pv[0][0].u) {
				gprintf("No legal moves");
				computer_side = EMPTY;
				continue;
			}
			
			sprintf(temp,"Computer's move: %s\n", move_str(pv[0][0].b));
			gprintf(temp);
			makemove(pv[0][0].b);
			ply = 0;
			gen();
			print_board();
			print_result();
			continue;
		}
		
                GrGetNextEvent(&event);
                switch(event.type) 
		{
			case GR_EVENT_TYPE_CLOSE_REQ:
				GrClose();
				exit(0);
				/* no return*/
			case GR_EVENT_TYPE_EXPOSURE:
				print_board();
				gprintf(NULL);
				break;
			case GR_EVENT_TYPE_BUTTON_DOWN:
				mouse_hit(event.button.x, event.button.y);
				break;
		}

		if (to != 999)
		{

			/* loop through the moves to see if it's legal */
			found = FALSE;
			for (i = 0; i < first_move[1]; ++i)
				if (gen_dat[i].m.b.from == from && gen_dat[i].m.b.to == to) 
				{
					found = TRUE;

					/* get the promotion piece right */
					if (gen_dat[i].m.b.bits & 32)
						switch (s[4]) 
						{
							case 'N':
								break;
							case 'B':
								i += 1;
								break;
							case 'R':
								i += 2;
								break;
							default:
								i += 3;
								break;
						}
					break;
				} /* if */

			if (!found || !makemove(gen_dat[i].m.b))
				gprintf("Illegal move.\n");
			else 
			{
				ply = 0;
				gen();
				print_board();
				print_result();
				computer_side = side;
				to = 999;
			}
		} /* if to != 999 */
	} /* for (;;) */

	return(0); /* never reached */
}



/* ***********************************************************/
/* move_str returns a string with move m in coordinate notation */
char *move_str(move_bytes m)
{
	static char str[6];
	char c;

	if (m.bits & 32) {
		switch (m.promote) {
			case KNIGHT:
				c = 'n';
				break;
			case BISHOP:
				c = 'b';
				break;
			case ROOK:
				c = 'r';
				break;
			default:
				c = 'q';
				break;
		}
		sprintf(str, "%c%d%c%d%c",
				COL(m.from) + 'A',
				8 - ROW(m.from),
				COL(m.to) + 'A',
				8 - ROW(m.to),
				c);
	}
	else
		sprintf(str, "%c%d%c%d",
				COL(m.from) + 'A',
				8 - ROW(m.from),
				COL(m.to) + 'A',
				8 - ROW(m.to));
	return str;
}


/* ***********************************************************/
void print_board(void)
{
	int row,column,i,x,y;

	GrDrawImageToFit(board, gc, 1, 0, board_w, board_h, board_image_id); 

	for (row=0;row<8;row++)
	{
		for (column=0;column<8;column++)
		{
			i = keyrows[row].columns[column].scancode;
			switch(color[i])
			{
				case LIGHT:
					switch(piece_char[piece[i]])
					{
						case 'P': 
							x = keyrows[row].columns[column].xoffset;
							y = keyrows[row].yoffset;
							GrDrawImageToFit(board, gc, x+10, y+13, w_p_w, w_p_h, w_p_image_id); 
							break;
       		                              	case 'N': 
							x = keyrows[row].columns[column].xoffset;
							y = keyrows[row].yoffset;
							GrDrawImageToFit(board, gc, x+9, y+9, w_n_w, w_n_h, w_n_image_id); 
							break;
       		                              	case 'B': 
							x = keyrows[row].columns[column].xoffset;
							y = keyrows[row].yoffset;
							GrDrawImageToFit(board, gc, x+9, y+9, w_b_w, w_b_h, w_b_image_id); 
							break;
       		                              	case 'R': 
							x = keyrows[row].columns[column].xoffset;
							y = keyrows[row].yoffset;
							GrDrawImageToFit(board, gc, x+9, y+12, w_r_w, w_r_h, w_r_image_id); 
							break;
       		                              	case 'Q': 
							x = keyrows[row].columns[column].xoffset;
							y = keyrows[row].yoffset;
							GrDrawImageToFit(board, gc, x+8, y+8, w_q_w, w_q_h, w_q_image_id); 
							break;
						case 'K':
							x = keyrows[row].columns[column].xoffset;
							y = keyrows[row].yoffset;
							GrDrawImageToFit(board, gc, x+8, y+5, w_k_w, w_k_h, w_k_image_id); 
							break;
                       		        }
					break;

				case DARK:
					switch(piece_char[piece[i]])
                                        {
						case 'P': 
							x = keyrows[row].columns[column].xoffset;
							y = keyrows[row].yoffset;
							GrDrawImageToFit(board, gc, x+10, y+13, b_p_w, b_p_h, b_p_image_id); 
							break;
       		                              	case 'N': 
							x = keyrows[row].columns[column].xoffset;
							y = keyrows[row].yoffset;
							GrDrawImageToFit(board, gc, x+9, y+9, b_n_w, b_n_h, b_n_image_id); 
							break;
       		                              	case 'B': 
							x = keyrows[row].columns[column].xoffset;
							y = keyrows[row].yoffset;
							GrDrawImageToFit(board, gc, x+9, y+9, b_b_w, b_b_h, b_b_image_id); 
							break;
       		                              	case 'R': 
							x = keyrows[row].columns[column].xoffset;
							y = keyrows[row].yoffset;
							GrDrawImageToFit(board, gc, x+9, y+12, b_r_w, b_r_h, b_r_image_id); 
							break;
       		                              	case 'Q': 
							x = keyrows[row].columns[column].xoffset;
							y = keyrows[row].yoffset;
							GrDrawImageToFit(board, gc, x+8, y+8, b_q_w, b_q_h, b_q_image_id); 
							break;
						case 'K':
							x = keyrows[row].columns[column].xoffset;
							y = keyrows[row].yoffset;
							GrDrawImageToFit(board, gc, x+8, y+5, b_k_w, b_k_h, b_k_image_id); 
							break;
                       		        }
					break;
			} /* switch */
		} /* for col */
	} /* for row */
}


/* ***********************************************************/
/* print_result() checks to see if the game is over */
void print_result()
{
        int i;
 
        /* is there a legal move? */
        for (i = 0; i < first_move[1]; ++i)
                if (makemove(gen_dat[i].m.b)) {
                        takeback();
                        break;
                }
        if (i == first_move[1]) {
                if (in_check(side)) {
                        if (side == LIGHT)
                                gprintf("Black mates");
                        else
                                gprintf("White mates");
                }
                else
                        gprintf("Stalemate");
        }
        else if (reps() == 3)
                gprintf("Draw by repetition");
        else if (fifty >= 100)
                gprintf("Draw by fifty move rule");
}             


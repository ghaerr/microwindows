#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nano-X.h"

#define IMAGE_PATH "demos/tuxchess/images"

extern GR_IMAGE_ID     board_image_id;
extern GR_IMAGE_INFO   board_info;
extern int board_w,board_h;
 
extern GR_IMAGE_ID     w_p_image_id;   /* white pawn */
extern GR_IMAGE_INFO   w_p_info;
extern int w_p_w,w_p_h;

extern GR_IMAGE_ID     w_n_image_id;   /* white knight */
extern GR_IMAGE_INFO   w_n_info;
extern int w_n_w,w_n_h;          

extern GR_IMAGE_ID     w_b_image_id;   /* white bishop */
extern GR_IMAGE_INFO   w_b_info;
extern int w_b_w,w_b_h;                         

extern GR_IMAGE_ID     w_r_image_id;   /* white rook */
extern GR_IMAGE_INFO   w_r_info;
extern int w_r_w,w_r_h;
 
extern GR_IMAGE_ID     w_q_image_id;   /* white queen */
extern GR_IMAGE_INFO   w_q_info;
extern int w_q_w,w_q_h;
 
extern GR_IMAGE_ID     w_k_image_id;   /* white king */
extern GR_IMAGE_INFO   w_k_info;
extern int w_k_w,w_k_h;
                 
extern GR_IMAGE_ID     b_p_image_id;   /* black pawn */
extern GR_IMAGE_INFO   b_p_info;
extern int b_p_w,b_p_h;

extern GR_IMAGE_ID     b_n_image_id;   /* black knight */
extern GR_IMAGE_INFO   b_n_info;
extern int b_n_w,b_n_h;          

extern GR_IMAGE_ID     b_b_image_id;   /* black bishop */
extern GR_IMAGE_INFO   b_b_info;
extern int b_b_w,b_b_h;                         

extern GR_IMAGE_ID     b_r_image_id;   /* black rook */
extern GR_IMAGE_INFO   b_r_info;
extern int b_r_w,b_r_h;
 
extern GR_IMAGE_ID     b_q_image_id;   /* black queen */
extern GR_IMAGE_INFO   b_q_info;
extern int b_q_w,b_q_h;
 
extern GR_IMAGE_ID     b_k_image_id;   /* black king */
extern GR_IMAGE_INFO   b_k_info;
extern int b_k_w,b_k_h;
                 
                                                
int load_images(void)
{
	char buf[128];

	sprintf(buf,"%s/board.gif",IMAGE_PATH);
        if (!(board_image_id = GrLoadImageFromFile(buf, 0))) {
                fprintf(stderr, "Can't load board image file\n");
                exit(-1);
        }
        GrGetImageInfo(board_image_id, &board_info);
        board_w = board_info.width;
        board_h = board_info.height;
 
	/* ****************/
	sprintf(buf,"%s/w_p.gif",IMAGE_PATH);
        if (!(w_p_image_id = GrLoadImageFromFile(buf, 0))) {
                fprintf(stderr, "Can't load white pawn image file\n");
                exit(-1);
        }
        GrGetImageInfo(w_p_image_id, &w_p_info);
        w_p_w = w_p_info.width;
        w_p_h = w_p_info.height;
 
	/* ****************/
	sprintf(buf,"%s/w_n.gif",IMAGE_PATH);
        if (!(w_n_image_id = GrLoadImageFromFile(buf, 0))) {
                fprintf(stderr, "Can't load white knight image file\n");
                exit(-1);
        }
        GrGetImageInfo(w_n_image_id, &w_n_info);
        w_n_w = w_n_info.width;
        w_n_h = w_n_info.height;

 
	/* ****************/
	sprintf(buf,"%s/w_b.gif",IMAGE_PATH);
        if (!(w_b_image_id = GrLoadImageFromFile(buf, 0))) {
                fprintf(stderr, "Can't load white bishop image file\n");
                exit(-1);
        }
        GrGetImageInfo(w_b_image_id, &w_b_info);
        w_b_w = w_b_info.width;
        w_b_h = w_b_info.height;
 
 
	/* ****************/
	sprintf(buf,"%s/w_r.gif",IMAGE_PATH);
        if (!(w_r_image_id = GrLoadImageFromFile(buf, 0))) {
                fprintf(stderr, "Can't load white rook image file\n");
                exit(-1);
        }
        GrGetImageInfo(w_r_image_id, &w_r_info);
        w_r_w = w_r_info.width;
        w_r_h = w_r_info.height;
 
 
	/* ****************/
	sprintf(buf,"%s/w_k.gif",IMAGE_PATH);
        if (!(w_k_image_id = GrLoadImageFromFile(buf, 0))) {
                fprintf(stderr, "Can't load white king image file\n");
                exit(-1);
        }
        GrGetImageInfo(w_k_image_id, &w_k_info);
        w_k_w = w_k_info.width;
        w_k_h = w_k_info.height;
 
 
	/* ****************/
	sprintf(buf,"%s/w_q.gif",IMAGE_PATH);
        if (!(w_q_image_id = GrLoadImageFromFile(buf, 0))) {
                fprintf(stderr, "Can't load white queen image file\n");
                exit(-1);
        }
        GrGetImageInfo(w_q_image_id, &w_q_info);
        w_q_w = w_q_info.width;
        w_q_h = w_q_info.height;
 
 
	/* ****************/
	sprintf(buf,"%s/b_p.gif",IMAGE_PATH);
        if (!(b_p_image_id = GrLoadImageFromFile(buf, 0))) {
                fprintf(stderr, "Can't load black pawn image file\n");
                exit(-1);
        }
        GrGetImageInfo(b_p_image_id, &b_p_info);
        b_p_w = b_p_info.width;
        b_p_h = b_p_info.height;
 
	/* ****************/
	sprintf(buf,"%s/b_n.gif",IMAGE_PATH);
        if (!(b_n_image_id = GrLoadImageFromFile(buf, 0))) {
                fprintf(stderr, "Can't load black knight image file\n");
                exit(-1);
        }
        GrGetImageInfo(b_n_image_id, &b_n_info);
        b_n_w = b_n_info.width;
        b_n_h = b_n_info.height;

 
	/* ****************/
	sprintf(buf,"%s/b_b.gif",IMAGE_PATH);
        if (!(b_b_image_id = GrLoadImageFromFile(buf, 0))) {
                fprintf(stderr, "Can't load black bishop image file\n");
                exit(-1);
        }
        GrGetImageInfo(b_b_image_id, &b_b_info);
        b_b_w = b_b_info.width;
        b_b_h = b_b_info.height;
 
 
	/* ****************/
	sprintf(buf,"%s/b_r.gif",IMAGE_PATH);
        if (!(b_r_image_id = GrLoadImageFromFile(buf, 0))) {
                fprintf(stderr, "Can't load black rook image file\n");
                exit(-1);
        }
        GrGetImageInfo(b_r_image_id, &b_r_info);
        b_r_w = b_r_info.width;
        b_r_h = b_r_info.height;
 
 
	/* ****************/
	sprintf(buf,"%s/b_k.gif",IMAGE_PATH);
        if (!(b_k_image_id = GrLoadImageFromFile(buf, 0))) {
                fprintf(stderr, "Can't load black king image file\n");
                exit(-1);
        }
        GrGetImageInfo(b_k_image_id, &b_k_info);
        b_k_w = b_k_info.width;
        b_k_h = b_k_info.height;
 
 
	/* ****************/
	sprintf(buf,"%s/b_q.gif",IMAGE_PATH);
        if (!(b_q_image_id = GrLoadImageFromFile(buf, 0))) {
                fprintf(stderr, "Can't load black queen image file\n");
                exit(-1);
        }
        GrGetImageInfo(b_q_image_id, &b_q_info);
        b_q_w = b_q_info.width;
        b_q_h = b_q_info.height;
 
	return(0);
}


//#include <stdio.h>
#include <math.h>		// float required for sin/cos in drawArc/drawPie
#include "device.h"
#include "platform.h"

/*
 * Microwindows C++ graphics
 *
 * C++ version
 * 27 Aug 2017 ghaerr
 *
 * Copyright (c) 2017 Greg Haerr, <greg@censoft.com>
 */

// application class
class the_application : public platform {
public:
	// constructor
	the_application() :
		startX(10),
		startY(10)
		{}

	// paint routine called once
	virtual void on_draw()
	{
		// 3d inset on white background
		//processing gfx;
		//graphics gfx;
		//gfx.fill(WHITE);
		//gfx.fillRect(10+2, 10+2, 200-4, 100-4);
		//gfx.draw3dInsetBox(10, 10, 200, 100);
		//gfx.fillRoundRect(10, 120, 200, 100, 15);
		//gfx.draw3dInsetRoundRect(10, 120, 200, 100, 15);

#if 0
		//processing gfx;
		gfx.point(3, 3);
		gfx.rect(10, 10, 200, 100);
		gfx.line(10, 10, 150, 200);
		gfx.roundRect(10, 120, 200, 100, 15);
		gfx.triangle(10, 220, 60, 230, 30, 310);
		gfx.circle(150, 260, 30);
		gfx.arc(50, 330, 30, 0, 270);
#endif
#if 1
		//aggprocessing gfx;
		gfx.point(3, 3);
		//gfx.noStroke();
		//gfx.noFill();
		gfx.strokeWeight(1.1);
		//gfx.fill(BLUE);
		gfx.rect(10, 10, 200, 100);
		//gfx.noFill();
		//gfx.noStroke();
		gfx.strokeWeight(1.2);
		//gfx.fill(MAGENTA);
		gfx.roundRect(10, 120, 200, 100, 15);
		//gfx.stroke(BLACK);
		gfx.strokeWeight(0.5);
		gfx.line(10, 10, 150, 200);
		gfx.strokeWeight(1.0);
		//gfx.fill(LTGREEN);
		gfx.circle(150, 260, 30);
		gfx.ellipse(250, 260, 50, 30);
		gfx.strokeWeight(0.5);
		gfx.triangle(10, 220, 60, 230, 30, 310);
		gfx.strokeWeight(0.4);
		//gfx.noFill();
		gfx.arc(60, 260, 40, 0, 90);
#endif
	}

	//virtual void on_mouse_button_down(int x, int y, unsigned flags) {	printf("down %d,%d, %x\n", x, y, flags); }
	virtual void on_mouse_button_down(int x, int y, unsigned flags)
	{
		if (flags & MWBUTTON_L)
		{
			startX = x;
			startY = y;
		}
		if (flags & MWBUTTON_R)
			wait_mode(!wait_mode());
	}
	virtual void on_mouse_button_up(int x, int y, unsigned flags) {		printf("up   %d,%d, %x\n", x, y, flags); }
	virtual void on_mouse_button_dblclick(int x, int y, unsigned flags){printf("dclk %d,%d, %x\n", x, y, flags); }
	//virtual void on_mouse_move(int x, int y, unsigned flags) {			printf("move %d,%d, %x\n", x, y, flags); }
	virtual void on_arrow_key(int key, int x, int y, unsigned flags) {	printf("arrow %d %d,%d, %x\n", key, x, y, flags); }
	virtual void on_key_press(int key, int x, int y, unsigned flags) {	printf("key   %d %d,%d, %x\n", key, x, y, flags); }
	virtual void on_idle()
	{
		gfx.noStroke();
		gfx.fill(MWRGB(0, 128, 128));
		gfx.rect(0, 0, gfx.width, gfx.height);
		gfx.stroke(BLACK);
		gfx.fill(WHITE);
		gfx.rect(startX, startY, 200, 100);
		gfx.update();
	}

	MWCOORD startX, startY;
};

int main(int ac, char **av)
{
	the_application app;
	//app.caption("Microwindows AGGFX");

	if (app.init())
	{
		return app.run();
	}
	return 1;
}

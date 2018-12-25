#include"../include/tnWidgets.h"
#include<stdlib.h>
#include<time.h>

struct mine {
	TN_WIDGET *button;
	TN_WIDGET *label;
	TN_WIDGET *picture;
	int x,y;
	int ismine;
	int ismarked;
	int labelvalue;
}minefield[10][10];

struct index{
	int i;
	int j;
};

int row[] = { -1,-1,0,1,1,1,0,-1};
int col[] = { 0,1,1,1,0,-1,-1,-1};
	

TN_WIDGET *main_app,*window;
GR_COLOR colors[] = { GR_RGB(0,0,0),
		      GR_RGB(0,0,255),
		      GR_RGB(0,128,0),
		      GR_RGB(255,0,0),
		      GR_RGB(0,255,255),
		      GR_RGB(255,0,255),
		      GR_RGB(255,255,0),
		      GR_RGB(128,128,0),
		      GR_RGB(128,0,128)
		   };
			      
TN_WIDGET *minepic;

void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(widget);
	tnEndApp();
}

void openall(void)
{
	int i,j;
	for(i=0;i<10;i++)
		for(j=0;j<10;j++)
		{
			if(minefield[i][j].button == NULL)
				continue;
			if(minefield[i][j].ismine && !minefield[i][j].ismarked)
			{
				tnDestroyWidget(minefield[i][j].button);
				minefield[i][j].button = NULL;
				minefield[i][j].picture = tnCreateWidget(TN_PICTURE,window,minefield[i][j].x,minefield[i][j].y,TN_HEIGHT,20,TN_WIDTH,20,TN_STRETCH,GR_FALSE,TN_END);
				if( (tnPictureDup(minefield[i][j].picture,minepic))== -1)
					exit(1);
				
			}
			if(!minefield[i][j].ismine && minefield[i][j].ismarked)
			{
				tnDestroyWidget(minefield[i][j].button);
				minefield[i][j].button = NULL;
				minefield[i][j].picture = tnCreateWidget(TN_PICTURE,window,minefield[i][j].x,minefield[i][j].y,TN_FILENAME,"demos/bmp/minehitfalse.bmp",TN_HEIGHT,20,TN_WIDTH,20,TN_STRETCH,GR_FALSE,TN_END);
			}
			if(minefield[i][j].button)
				tnSetEnabled(minefield[i][j].button,GR_FALSE);
		}
	return;
}

void openzero(int i,int j)
{
	int k;
	int i1,j1;
	char s[2];
	tnDestroyWidget(minefield[i][j].button);
	minefield[i][j].button = NULL;
	for(k=0; k < 8; k++)
	{
		i1 = i + row[k];
		j1 = j + col[k];
		if(i1 >= 0 && i1 < 10 && j1 >= 0 && j1 < 10)
		{
			
			if(minefield[i1][j1].button == NULL)
				continue;
			if(minefield[i1][j1].labelvalue == 0)
				openzero(i1,j1);
			else
			{
				tnDestroyWidget(minefield[i1][j1].button);
				minefield[i1][j1].button = NULL;
				s[0] = minefield[i1][j1].labelvalue + '0';
				s[1] = '\0';
				minefield[i1][j1].label = tnCreateWidget(TN_LABEL,window,minefield[i1][j1].x,minefield[i1][j1].y,TN_CAPTION,s,TN_HEIGHT,20,TN_WIDTH,20,TN_FGCOLOR,colors[minefield[i1][j1].labelvalue],TN_END);
			}
		}
	}
	return;
}
					
					
					
	
	
					
	

void minefunc(TN_WIDGET *button,DATA_POINTER dptr)
{
	int cb = tnGetButtonPressed(button);
	struct index *p = (struct index *)dptr;
	char s[2];
	int i = p->i;
	int j = p->j;
	if(!(minefield[i][j].ismarked) && cb & GR_BUTTON_L)
	{
		if(minefield[i][j].ismine)
			openall();
		else
		{
			tnDestroyWidget(button);
			minefield[i][j].button = NULL;
			if(minefield[i][j].labelvalue == 0)
				openzero(i,j);
			else
			{
				s[0] = minefield[i][j].labelvalue + '0';
				s[1] = '\0';
				minefield[i][j].label = tnCreateWidget(TN_LABEL,window,minefield[i][j].x,minefield[i][j].y,TN_CAPTION,s,TN_HEIGHT,20,TN_WIDTH,20,TN_FGCOLOR,colors[minefield[i][j].labelvalue],TN_END);
			}
		}
	}
	if(cb & GR_BUTTON_R)
	{
		if(!(minefield[i][j].ismarked))
		{
			tnSetButtonPixmap(button,"demos/bmp/mineflag.bmp");
			minefield[i][j].ismarked = GR_TRUE;
		}
		else
		{
			tnRemoveButtonPixmap(button);
			minefield[i][j].ismarked = GR_FALSE;
		}
		
	}
	return;
}

int main(int argc, char *argv[])
{
	int i,j,k,l;
	long int r;
	struct index *p;
		srandom(time(NULL));
	for(i=0;i<10;i++)
		for(j=0;j<10;j++)
		{
			minefield[i][j].ismine = GR_FALSE;
			minefield[i][j].labelvalue = 0;
		}
	
	for(k=0;k<20;k++)
	{
		r = random();
		r = r%100;
		i = r /10;
		j = r % 10;					
		if(minefield[i][j].ismine)
		{
			k--;
			continue;
		}
		minefield[i][j].ismine = GR_TRUE;
		for(l=0;l<8;l++)
			if((i + row[l]) >= 0 && (i + row[l]) < 10 && (j+col[l]) >=0 && (j+col[l]) < 10)
				minefield[i+row[l]][j+col[l]].labelvalue += 1;
	}
				
			
				
		
		
	main_app = tnAppInitialize(argc,argv);
	window = tnCreateWidget(TN_WINDOW,main_app,50,50,TN_WIDTH,200,TN_HEIGHT,250,TN_CAPTION,"MineSweeper",TN_END);
	minepic = tnCreateWidget(TN_PICTURE,window,50,50,TN_WIDTH,20,TN_HEIGHT,20,TN_FILENAME,"demos/bmp/minebomb.bmp",TN_VISIBLE,GR_FALSE,TN_END);
	for(i=0;i<10;i++)
		for(j=0;j<10;j++)
		{
			minefield[i][j].button = tnCreateWidget(TN_BUTTON,window,j*20,i*20+50,TN_WIDTH,20,TN_HEIGHT,20,TN_CAPTION," ",TN_BGCOLOR,GR_RGB(244,176,066),TN_END);
			minefield[i][j].x = j*20;
			minefield[i][j].y = i*20+50;
			p = (struct index *)malloc(sizeof(struct index));
			p->i = i;
			p->j = j;
				
			tnRegisterCallBack(minefield[i][j].button,CLICKED,minefunc,p);
		}
	tnRegisterCallBack(window,CLOSED,endapp,NULL);
	tnMainLoop();
	return 0;
}

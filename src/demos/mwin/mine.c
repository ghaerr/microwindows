#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
/*
 * Minesweeper for Microwindows, adapted from MiniGUI
 */
extern int mwCurrentButtons;	/* FIXME */


#ifdef __rtems__
#define  srandom  srand
#define  random   rand
#endif

#define MWINCLUDECOLORS
#include "windows.h"
#include "wintools.h"

typedef struct {	
       	int flag;
      	int value;
       	BOOL test;
       	BOOL hit;
        BOOL bombout;
        BOOL error;
}BOM;

typedef struct  {
	int x;
 	int y;
	BOOL NY;
}NO;

typedef struct {
    int highscore;
    char name[20];
}SCORE;

typedef struct {
	DWORD	dwStyle;
	LPSTR	spCaption;
	HMENU	hMenu;
	HCURSOR	hCursor;
	HICON	hIcon;
	WNDPROC	MainWindowProc;
	int	lx, ty, rx, by;
	COLORREF iBkColor;
	DWORD	dwAddData;
	HWND	hHosting;
} MAINWINCREATE, *PMAINWINCREATE;

#define IDOK	0
#define GetCharWidth()		11
#define GetCharHeight()		13

extern MWIMAGEHDR image_mineflag;
extern MWIMAGEHDR image_mineface;
extern MWIMAGEHDR image_minefacelost;
extern MWIMAGEHDR image_minebomb;
extern MWIMAGEHDR image_minedone;
extern MWIMAGEHDR image_minehitfalse;

HWND CreateMainWindow(PMAINWINCREATE pCreateInfo);
BOOL PtInRect2(const RECT *lprc, int x, int y);
void Draw3DUpFrame(HDC hDC, int l, int t, int r, int b, int fillc);
void SearchGround(HDC hdc,int x,int y);
int Open(HWND hWnd,int x,int y);
LRESULT TestMyWinProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
void InitMyWinCreateInfo(PMAINWINCREATE pCreateInfo);
void InitAbHostedCreateInfo(HWND hHosting, PMAINWINCREATE  pCreateInfo); 
void InitHighScoreCreateInfo (HWND hHosting, PMAINWINCREATE pCreateInfo);
void InitCongratulationCreateInfo (HWND hHosting, PMAINWINCREATE pCreateInfo);
void* TestMyWindow(void* data);
void BombGame(HWND hWnd, int x, int y);
void Finished(HWND hWnd);
void Cancel3DFrame(HDC hdc, int l,int t,int r,int b);
void TextValue(HDC hdc, int x,int y,int value);
void BombOut(HWND hWnd);
void BothButtonDownProc(HWND hWnd, int x, int y);
void DrawDigit(HDC hdc, char* buffer, int CLOCK);

#define  ID_ABOUT  300
#define  ID_NEW    301  
#define  ID_LARGE  302
#define  ID_MIDDLE 303
#define  ID_SMALL  304
#define  ID_CLOSE  305
#define  ID_HIGHSCORE 306
#define  ID_CLOCK  400

#define  WIDTH_LARGEWIN   544
#define  HEIGHT_LARGEWIN  371
#define  WIDTH_MIDDLEWIN  287
#define  HEIGHT_MIDDLEWIN 332
#define  WIDTH_SMALLWIN   178
#define  HEIGHT_SMALLWIN  206

#define  WIDTH_FACE      30
#define  HEIGHT_FACE     30

#define  WIDTH_DIGIT     11
#define  WIDTH_BOMNUM    (2*WIDTH_DIGIT)
#define  HEIGHT_BOMNUM   30

#define  WIDTH_CLOCK     (3*WIDTH_DIGIT)
#define  HEIGHT_CLOCK    30
#define  FREQ_CLOCK      1000

#define  WIDTH_BOX       18
#define  HEIGHT_BOX      18

#if 0
static int winwidth = WIDTH_LARGEWIN;	/* change this for startup mine size */
static int winheight = HEIGHT_LARGEWIN;
static int sg_boxnumx = 30;
static int sg_boxnumy = 18;
static int bombnum = 99;
#endif

#if 1
static int winwidth = WIDTH_MIDDLEWIN;	/* change this for startup mine size */
static int winheight = HEIGHT_MIDDLEWIN;
static int sg_boxnumx = 16;
static int sg_boxnumy = 16;
static int bombnum = 40;
#endif

#if 0
static int winwidth = WIDTH_SMALLWIN;	/* change this for startup mine size */
static int winheight = HEIGHT_SMALLWIN;
static int sg_boxnumx = 8;
static int sg_boxnumy = 8;
static int bombnum = 10;
#endif

static BOM bom[30][18];
static NO NoAdr[540];
static SCORE score[3];
static int itime, leftbombnum;
static int oldx, oldy, adrx, adry;
static int flag_bombout, flag_finished, flag_size = 2;
static int second = 0 ;
static BOOL bTimer;
static HWND hHighscore,hCongratulate; 
static int offsetx;
static int x_face, x_bomnum, x_clock;

void Cancel3DFrame(HDC hdc, int  l, int  t, int w, int h)
{
    HBRUSH hbr;
    RECT rc;

    SetTextColor (hdc,GRAY);
    Rectangle(hdc, l, t, l + w, t + h);
    hbr = CreateSolidBrush(LTGRAY);
    SetRect(&rc, l+1, t+1, l+w, t+h);
    FillRect(hdc, &rc, hbr);
    DeleteObject(hbr);
} 

void TextValue(HDC hdc, int x, int y, int value)
 {
    int color = 0;
    char   va[20]; 
    switch(value)
    {
       case 1:  color=BLUE;       break;
       case 2:  color=GREEN;      break;   
       case 3:  color=RED;        break;
       case 4:  color=MAGENTA;    break;
       case 5:  color=YELLOW;     break;
       case 6:  color=CYAN;       break;
       case 7:  color=RED;    break;
       case 8:  color=GREEN;  break; 
    }
    SetBkColor(hdc,LTGRAY);
    SetTextColor(hdc,color);
    sprintf(va,"%d",value);
    
    TextOut(hdc, x + ((WIDTH_BOX - GetCharWidth ()) >> 1), 
        y + ((HEIGHT_BOX - GetCharHeight ()) >> 1), va, -1);
}  

void SearchGround(HDC hdc,int x,int y)
{
     int x1=0,y1=0;
     int i=1;
     
     bom[x][y].test=TRUE;
     NoAdr[itime].x=x;
     NoAdr[itime].y=y;
     NoAdr[itime].NY=FALSE;
     itime++;
     Cancel3DFrame(hdc,x*WIDTH_BOX+offsetx, y*HEIGHT_BOX+HEIGHT_FACE,
                        WIDTH_BOX, HEIGHT_BOX);
     while( i <= 8 )
     {
         switch(i) 
         {
             case  1:  x1=x-1;  y1=y;    break;
             case  2:  x1=x-1;  y1=y-1;  break;
             case  3:  x1=x-1;  y1=y+1;  break;
             case  4:  x1=x+1;  y1=y;    break;
             case  5:  x1=x+1;  y1=y-1;  break;
             case  6:  x1=x+1;  y1=y+1;  break;
             case  7:  y1=y-1;  x1=x;    break;
             case  8:  y1=y+1;  x1=x;    break;   
         }
         if( x1>=0 && y1>=0 && x1<sg_boxnumx && y1<sg_boxnumy
                 &&!bom[x1][y1].hit&& !bom[x1][y1].test && !bom[x1][y1].value )
               SearchGround(hdc,x1,y1);
                     
         if( x1>=0 && y1>=0 && x1<sg_boxnumx && y1<sg_boxnumy
                            &&!bom[x1][y1].hit
                            &&!bom[x1][y1].test && bom[x1][y1].value!=0 )
         {
             bom[x1][y1].test=TRUE;
             NoAdr[itime].x=x1;
             NoAdr[itime].y=y1;
             NoAdr[itime].NY=TRUE;
             itime++;
             Cancel3DFrame(hdc, x1*WIDTH_BOX+offsetx, y1*HEIGHT_BOX+HEIGHT_FACE,
                                 WIDTH_BOX, HEIGHT_BOX);
             TextValue(hdc, x1*WIDTH_BOX+offsetx, y1*HEIGHT_BOX+HEIGHT_FACE, 
                                 bom[x1][y1].value);
         } 
         i++;
     }
 }
 
BOOL Open(HWND hWnd,int x,int y)
{ 
    int x1=0,y1=0;
    int i=1;
    HDC hdc;

    hdc = GetDC(hWnd);
    while( i <= 8 )
    {
        switch( i ) {
            case  1:  x1=x-1;  y1=y;    break;
            case  2:  x1=x-1;  y1=y-1;  break;
            case  3:  x1=x-1;  y1=y+1;  break;
            case  4:  x1=x+1;  y1=y;    break;
            case  5:  x1=x+1;  y1=y-1;  break;
            case  6:  x1=x+1;  y1=y+1;  break;
            case  7:  y1=y-1;  x1=x;    break;
            case  8:  y1=y+1;  x1=x;    break;   
        }

        if( x1>=0 && y1>=0 && x1<sg_boxnumx && y1<sg_boxnumy
            && !bom[x1][y1].hit && bom[x1][y1].flag) {    
            ReleaseDC(hWnd, hdc);
            return FALSE;
        }
                      
        if( x1>=0 && y1>=0 && x1<sg_boxnumx && y1<sg_boxnumy
                && !bom[x1][y1].test && !bom[x1][y1].value
                && !bom[x1][y1].flag ){
            SearchGround(hdc,x1,y1);
        }   
                     
        if( x1>=0 && y1>=0 && x1<sg_boxnumx && y1<sg_boxnumy
                  && !bom[x1][y1].test && bom[x1][y1].value!=0 )
        {
            bom[x1][y1].test=TRUE;
            NoAdr[itime].x=x1;
            NoAdr[itime].y=y1;
            NoAdr[itime].NY=TRUE;
            itime++;
             Cancel3DFrame(hdc, x1*WIDTH_BOX+offsetx, y1*HEIGHT_BOX+HEIGHT_FACE,
                                 WIDTH_BOX, HEIGHT_BOX);
             TextValue(hdc, x1*WIDTH_BOX+offsetx, y1*HEIGHT_BOX+HEIGHT_FACE, 
                                 bom[x1][y1].value);
        }
        i++; 
    }
    
    ReleaseDC(hWnd, hdc);
    return  TRUE; 
}

void BombOut(HWND hWnd)
{ 
  int i,j;
  HDC hdc;
  
  hdc=GetDC(hWnd);
  
  for (i = 0; i < sg_boxnumx; i++) {
      for (j = 0; j < sg_boxnumy; j++) {
         if (bom[i][j].flag && !bom[i][j].hit) {
            Cancel3DFrame(hdc,i*WIDTH_BOX+offsetx,j*HEIGHT_BOX+HEIGHT_FACE,
                    WIDTH_BOX,HEIGHT_BOX);
                    
	    DrawDIB(hdc, i*WIDTH_BOX+offsetx+1, j*HEIGHT_BOX+1+HEIGHT_FACE,
			&image_minebomb);
            bom[i][j].bombout = TRUE;            
          }
         if (!bom[i][j].flag && bom[i][j].hit){
            Cancel3DFrame(hdc,i*WIDTH_BOX+offsetx,j*HEIGHT_BOX+HEIGHT_FACE,
                    WIDTH_BOX,HEIGHT_BOX);
                    
	    DrawDIB(hdc, i*WIDTH_BOX+offsetx+1, j*HEIGHT_BOX+1+HEIGHT_FACE,
			&image_minehitfalse);
            bom[i][j].error = TRUE;            
                        
          }   
       }
   }

   DrawDIB(hdc, x_face, 0, &image_minefacelost);
   flag_bombout = 1;
   bTimer = FALSE;
   ReleaseDC(hWnd, hdc);
}

void Finished(HWND hWnd)
{ 
    int i,j;
    HDC hdc;
    RECT bombnumber;
    MAINWINCREATE CreateInfo;
  
    hdc = GetDC(hWnd);
  
    for (i = 0; i < sg_boxnumx; i++) {
        for (j = 0; j < sg_boxnumy; j++) {
            if (bom[i][j].flag && !bom[i][j].hit) {
                    
		DrawDIB(hdc, i*WIDTH_BOX+offsetx+3, j*HEIGHT_BOX+3+HEIGHT_FACE,
				 &image_mineflag);
                bom[i][j].hit = TRUE;                     
            }
        }
    }

    DrawDIB(hdc, x_face+1, 1, &image_minedone);
    flag_finished = 1;
    bTimer = FALSE;
    ReleaseDC(hWnd, hdc);
   
    leftbombnum = 0;
                     
    SetRect (&bombnumber, x_bomnum, 0,
                         x_bomnum + WIDTH_BOMNUM, HEIGHT_BOMNUM);
    InvalidateRect (hWnd, &bombnumber, FALSE);
   
    if (second < score[flag_size].highscore){
        InitCongratulationCreateInfo(hWnd, &CreateInfo);
        hCongratulate = CreateMainWindow (&CreateInfo);
    }    
}    

#if 0
HMENU createpmenuabout()
{
    HMENU hmnu;
    MENUITEMINFO mii;
    memset (&mii, 0, sizeof(MENUITEMINFO));
    mii.type        = MFT_STRING;
    mii.id          = 0;
    mii.typedata    = (DWORD)"About";
    hmnu = CreatePopupMenu (&mii);
    
    memset (&mii, 0, sizeof(MENUITEMINFO));
    mii.type        = MFT_STRING ;
    mii.state       = 0;
    mii.id          = ID_ABOUT;
    mii.typedata    = (DWORD)"About Bomb Game";
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    return hmnu;
}

HMENU createpmenustart()
{
    HMENU hmnu;
    MENUITEMINFO mii;
    memset (&mii, 0, sizeof(MENUITEMINFO));
    mii.type        = MFT_STRING;
    mii.id          = 0;
    mii.typedata    = (DWORD)"Start";
    hmnu = CreatePopupMenu (&mii);
    
    memset (&mii, 0, sizeof(MENUITEMINFO));
    mii.type        = MFT_STRING ;
    mii.state       = 0;
    mii.id          = ID_NEW;
    mii.typedata    = (DWORD)"New Game";
    InsertMenuItem(hmnu, 0, TRUE, &mii);
    
    mii.type        = MFT_STRING ;
    mii.state       = 0;
    mii.id          = ID_HIGHSCORE;
    mii.typedata    = (DWORD)"High Score";
    InsertMenuItem(hmnu, 1, TRUE, &mii);
    
    mii.type        = MFT_STRING ;
    mii.state       = 0;
    mii.id          = ID_CLOSE;
    mii.typedata    = (DWORD)"Quit Game";
    InsertMenuItem(hmnu, 2, TRUE, &mii);
    
    return hmnu;
}

HMENU createpmenulevel()
{
    HMENU hmnu;
    MENUITEMINFO mii;
    memset (&mii, 0, sizeof(MENUITEMINFO));
    mii.type        = MFT_STRING;
    mii.id          = 0;
    mii.typedata    = (DWORD)"Level";
    hmnu = CreatePopupMenu (&mii);
    
    mii.type        = MFT_STRING ;
    mii.state       = 0;
    mii.id          = ID_LARGE;
    mii.typedata    = (DWORD)"Large";
    InsertMenuItem(hmnu, 0, TRUE, &mii);
     
    mii.type        = MFT_STRING ;
    mii.state       = 0;
    mii.id          = ID_MIDDLE;
    mii.typedata    = (DWORD)"Middle";
    InsertMenuItem(hmnu, 1, TRUE, &mii);  
    
    mii.type        = MFT_STRING ;
    mii.state       = 0;
    mii.id          = ID_SMALL;
    mii.typedata    = (DWORD)"Small";
    InsertMenuItem(hmnu, 2, TRUE, &mii);  

    return hmnu;
}

HMENU createmenu1()
{
    HMENU hmnu;
    MENUITEMINFO mii;

    hmnu = CreateMenu();

    memset (&mii, 0, sizeof(MENUITEMINFO));
    mii.type        = MFT_STRING;
    mii.id          = 100;
    mii.typedata    = (DWORD)"Start";
    mii.hsubmenu    = createpmenustart();

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type        = MFT_STRING;
    mii.id          = 110;
    mii.typedata    = (DWORD)"Level";
    mii.hsubmenu    = createpmenulevel();
    InsertMenuItem(hmnu, 1, TRUE, &mii);
    
    mii.type        = MFT_STRING;
    mii.id          = 120;
    mii.typedata    = (DWORD)"About";
    mii.hsubmenu    = createpmenuabout();
    InsertMenuItem(hmnu, 2, TRUE, &mii);
                   
    return hmnu;
}
#endif

void BothButtonDownProc(HWND hWnd,int adrx,int adry)
{
    int test_open = 0; 
    int i = 1;
    int adrx1 = 0, adry1 = 0;
    int flag_opened = 1;

    if (!bom[adrx][adry].test)
    return;
    if (!bom[adrx][adry].value)
    return;
    if (bom[adrx][adry].hit)
    return;
    while (i <= 8)
    {
       switch (i)
       {
        case 1:
           adrx1 = adrx-1;
           adry1 = adry;
        break;
                        
        case 2:
           adrx1 = adrx-1;
           adry1 = adry-1;
        break;

        case 3:
           adrx1 = adrx-1;
           adry1 = adry+1;
        break;
        
        case 4:
           adrx1 = adrx;
           adry1 = adry-1;
        break;
        
        case 5:
            adrx1 = adrx;
            adry1 = adry+1;
        break;
        
        case 6:
            adrx1 = adrx+1;
            adry1 = adry; 
        break; 
        
        case 7:
            adrx1 = adrx+1;
            adry1 = adry+1;
        break; 
        
        case 8:
            adrx1 = adrx+1;
            adry1 = adry-1;
        break;
       }
       
       if (adrx1>=0 && adry1>=0 && adrx1<sg_boxnumx && adry1<sg_boxnumy
                            && bom[adrx1][adry1].hit)
           test_open++;
       else
            if(adrx>=0 && adry1>=0 && adrx1<sg_boxnumx && adry1<sg_boxnumy
                            && !bom[adrx1][adry1].test)
                flag_opened = 0;            
       i++;
    }
    if ((test_open == bom[adrx][adry].value) && !flag_opened)
    {
        if (!Open (hWnd, adrx, adry))
            BombOut (hWnd);
        if (itime == (sg_boxnumx*sg_boxnumy-bombnum))
            Finished(hWnd);
             
    }
            
}

void DrawDigit(HDC hdc, char* buffer, int CLOCK)
{
    int x;
    
    if (CLOCK)
        x = x_clock;
    else
        x = x_bomnum;
        
    	SetBkMode(hdc, OPAQUE);
	SetBkColor(hdc, LTGRAY);
	TextOut(hdc, x, 0, buffer, -1);
	return;
}

LRESULT TestMyWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    char  bomn[30], seconds[30];
    int i, j;
    int ran1, ran2;
    static RECT bombregion, face, onerect, bombnumber, clock; 
    static RECT winposition;
    MAINWINCREATE  CreateInfo;

    PAINTSTRUCT ps;
    switch (message) {
      
        case WM_CREATE:
#if 0
    	    FILE* pHighscore;
    	    char  buffer[256];
            if( LoadBitmap(&bmpbom,"res/lei.bmp")<0)
                fprintf(stderr,"bitmap error");
            else
                fValidbom = TRUE;
  
            if( LoadBitmap(&bmpface,"res/face1.bmp")<0)
                fprintf(stderr,"bitmap error");
            else
                fValidface = TRUE;
                
            if( LoadBitmap(&bitmap1,"res/face.bmp")<0)
                fprintf(stderr,"bitmap error");
            else
                fValid1 = TRUE;
                
            if( LoadBitmap(&bmpflag,"res/flag.bmp")<0)
                fprintf(stderr,"bitmap error");
            else
                fValidflag = TRUE;   
                
            if( LoadBitmap(&bmpfinalface,"res/finished.bmp")<0)
                fprintf(stderr,"bitmap error");
            else
                fValidfinalface = TRUE;   
               
            if( LoadBitmap(&bmphitfalse,"res/hitfalse.bmp")<0)
                fprintf(stderr,"bitmap error");
            else
                fValidhitfalse = TRUE;   
                
            for (i = 0; i < 10; i++){
                sprintf(buffer, "res/%d.bmp", i);
                LoadBitmap(sg_bmpDigit + i, buffer);
            }

            if ((pHighscore = fopen("res/.highscore.bomb","r"))){
                for (i = 0; i < 3; i++)
                    fscanf(pHighscore, "%d, %s",
                            &score[i].highscore, score[i].name);
                fclose(pHighscore);
            }
            else
                for (i = 0; i < 3; i++){
                    score[i].highscore = 999;
                    strcpy(score[i].name, "unknown");
                }    
#endif
            SetTimer(hWnd, ID_CLOCK, FREQ_CLOCK, NULL);
            
            PostMessage(hWnd,WM_COMMAND,ID_NEW,0);
        break;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_ABOUT)
            {
               InitAbHostedCreateInfo(hWnd,&CreateInfo);
               CreateMainWindow(&CreateInfo);  
            }
            
            if (LOWORD(wParam) == ID_CLOSE)
            {
                PostMessage(hWnd, WM_CLOSE, 0, 0);
            }
            
            if (LOWORD(wParam) == ID_HIGHSCORE)
            {
               InitHighScoreCreateInfo(hWnd,&CreateInfo);
               hHighscore =  CreateMainWindow(&CreateInfo);  
               ShowWindow(hHighscore,SW_SHOW); 
            }
            
            if (LOWORD(wParam) == ID_LARGE)
            {
                bombnum = 99;
                sg_boxnumx = 30;
                sg_boxnumy = 18;
                winwidth = WIDTH_LARGEWIN;
                flag_size = 2;
                GetWindowRect(hWnd, &winposition);
                MoveWindow(hWnd, winposition.left, winposition.top, 
                           WIDTH_LARGEWIN, HEIGHT_LARGEWIN, FALSE);
                PostMessage(hWnd, WM_COMMAND, ID_NEW, 0);
            }    
            if (LOWORD(wParam) == ID_MIDDLE)
            {
                bombnum = 40;
                sg_boxnumx = 16;
                sg_boxnumy = 16;
                winwidth = WIDTH_MIDDLEWIN;
                flag_size = 1;
                GetWindowRect(hWnd, &winposition);
                MoveWindow(hWnd, winposition.left, winposition.top, 
                           WIDTH_MIDDLEWIN, HEIGHT_MIDDLEWIN, FALSE);
                PostMessage(hWnd, WM_COMMAND, ID_NEW, 0);
            }
            
            if (LOWORD(wParam) == ID_SMALL)
            {
                bombnum = 10;
                sg_boxnumx = 8;
                sg_boxnumy = 8;
                winwidth = WIDTH_SMALLWIN;
                flag_size = 0;
                GetWindowRect(hWnd, &winposition);
                MoveWindow(hWnd, winposition.left, winposition.top, 
                           WIDTH_SMALLWIN, HEIGHT_SMALLWIN, FALSE);
                PostMessage(hWnd, WM_COMMAND, ID_NEW, 0);
            }    
                
            if (LOWORD(wParam) == ID_NEW)
            {
                bTimer = FALSE;
                second = 0;
                itime = 0;
                leftbombnum = bombnum;
                flag_bombout = 0;
                flag_finished = 0;
                x_bomnum = winwidth / 6;
                x_face = (winwidth*2) / 5;
                x_clock = (winwidth*3) / 5;
                offsetx = (winwidth - WIDTH_BOX*sg_boxnumx)/2-2;
                SetRect (&clock, x_clock, 0, 
                            x_clock + WIDTH_CLOCK, HEIGHT_CLOCK);
                            
                SetRect (&face, x_face, 0,
                            x_face + WIDTH_FACE, HEIGHT_FACE);
                            
                SetRect (&bombregion, offsetx, HEIGHT_FACE,
                           WIDTH_BOX*sg_boxnumx+offsetx,
                           HEIGHT_BOX*sg_boxnumy+HEIGHT_FACE);
                     
                SetRect (&bombnumber, x_bomnum, 0,
                     x_bomnum + WIDTH_BOMNUM, HEIGHT_BOMNUM);
                     
               /**************initial bomb value************** */
               
              for (i = 0; i < sg_boxnumx; i++)
                for (j = 0; j < sg_boxnumy; j++)
                { bom[i][j].flag  = 0;
                  bom[i][j].hit   = FALSE;
                  bom[i][j].value = 0;
                  bom[i][j].test  = FALSE;
                  bom[i][j].bombout = FALSE;
                  bom[i][j].error = FALSE;
                 }; 
              for (i = 0; i < (sg_boxnumx*sg_boxnumy); i++)
                  NoAdr[i].NY = FALSE; 
                   
              srandom( time(NULL));
              i = 0;
              while( i < bombnum )
               {
                  ran1 = random()%sg_boxnumx;
                  ran2 = random()%sg_boxnumy;
                  if(!bom[ran1][ran2].flag)
                     { 
                        bom[ran1][ran2].flag = 1;
			i++;
                     } 
#ifdef __rtems__
		      else i++;		/* bad rtems random function*/
#endif
                }
 
               for (i = 0; i < sg_boxnumx; i++)         
                  for (j = 0; j < sg_boxnumy; j++)
                    {
                if (!bom[i][j].flag) {
                if(i-1>=0&&j-1>=0&&bom[i-1][j-1].flag)
                        bom[i][j].value++;
                        
                if(i-1>=0&&bom[i-1][j].flag)
                        bom[i][j].value++;
                        
                if(i-1>=0&&j+1<sg_boxnumy&&bom[i-1][j+1].flag)
                        bom[i][j].value++;
                        
                if(j-1>=0&&bom[i][j-1].flag)
                        bom[i][j].value++;
                        
                if(j+1<sg_boxnumy&&bom[i][j+1].flag)
                        bom[i][j].value++;
                        
                if(i+1<sg_boxnumx&&j+1<sg_boxnumy&&bom[i+1][j+1].flag)
                        bom[i][j].value++; 
                        
                if(i+1<sg_boxnumx&&bom[i+1][j].flag) 
                        bom[i][j].value++;
                        
                if(i+1<sg_boxnumx&&j-1>=0&&bom[i+1][j-1].flag)
                        bom[i][j].value++;
                } 
                     }    
               InvalidateRect (hWnd, NULL, TRUE);
               }
        break;
        
        case WM_TIMER:
            if (wParam == ID_CLOCK)
                if (bTimer){
                    if (second < 1000){
                        second++;
                        InvalidateRect(hWnd, &clock, FALSE);
                    }
                }
        break;        

        case WM_LBUTTONDOWN:

            oldx = LOWORD (lParam);
            oldy = HIWORD (lParam);

            adrx = (oldx-offsetx)/WIDTH_BOX;
            adry = (oldy-HEIGHT_FACE)/HEIGHT_BOX;
            
            if (hCongratulate || hHighscore)
                break;
                
            if (!PtInRect2 (&bombregion, oldx, oldy)) {
                if (PtInRect2 (&face, oldx, oldy)){
                    PostMessage (hWnd, WM_COMMAND, ID_NEW, 0);
                    break;
                }    
                else
                    break;
            }
            if (flag_bombout)
                break;
            if (flag_finished)
                break;
            if (!bTimer)
                bTimer = TRUE;
                
            if (bom[adrx][adry].hit)
                break;

#if 0
            if (GetShiftKeyStatus() & 0x00000200) {
#endif
	    if(mwCurrentButtons & MWBUTTON_R) {
                BothButtonDownProc(hWnd,adrx,adry);
                break;
            }
            
                
            if (bom[adrx][adry].test)
                break;
                
            if (bom[adrx][adry].flag) {
                BombOut(hWnd);   
                break;
            }    
            if (bom[adrx][adry].value != 0)
            {
                    NoAdr[itime].x  = adrx;
                    NoAdr[itime].y  = adry;
                    NoAdr[itime].NY = TRUE;
                    itime++;
                    bom[adrx][adry].test = TRUE;
                    
                    SetRect (&onerect, adrx*WIDTH_BOX+offsetx,
                           adry*HEIGHT_BOX+HEIGHT_FACE,
                           (adrx+1)*WIDTH_BOX+offsetx,
                           (adry+1)*HEIGHT_BOX+HEIGHT_FACE);
                    InvalidateRect (hWnd, &onerect, FALSE);
             }
             else {
                    hdc = GetDC(hWnd);
                    SearchGround (hdc, adrx, adry);
                    ReleaseDC(hWnd, hdc);
             }
             
             if (itime == (sg_boxnumx*sg_boxnumy - bombnum))
                Finished(hWnd);
             
        break;

        case WM_MBUTTONDOWN:
             oldx = LOWORD (lParam);
             oldy = HIWORD (lParam);
             adrx = (oldx-offsetx)/WIDTH_BOX;
             adry = (oldy-HEIGHT_FACE)/HEIGHT_BOX;  

            if (!PtInRect2 (&bombregion, oldx, oldy))
                break;
            if (flag_bombout)
                break;
            if (flag_finished)
                break;
                
            if (!bTimer)
                bTimer = TRUE;
                
           /*****RBUTTONDOWN & LBUTTONDOWN******/
            
                BothButtonDownProc(hWnd,adrx,adry); 
		break;

        case WM_RBUTTONDOWN:
#if 0
             SetCapture (hWnd);
#endif
             oldx = LOWORD (lParam);
             oldy = HIWORD (lParam);
             adrx = (oldx-offsetx)/WIDTH_BOX;
             adry = (oldy-HEIGHT_FACE)/HEIGHT_BOX;  

            if (!PtInRect2 (&bombregion, oldx, oldy))
                break;
            if (flag_bombout)
                break;
            if (flag_finished)
                break;
                
            if (!bTimer)
                bTimer = TRUE;
                
           /*****RBUTTONDOWN & LBUTTONDOWN******/
            
#if 0
            if (GetShiftKeyStatus() & 0x00000100){
#endif
	    if(mwCurrentButtons & MWBUTTON_L) {
                BothButtonDownProc(hWnd,adrx,adry); 
                break; 
            }    
           /*******  only  rbuttondown   *******/
                
            if (bom[adrx][adry].test)
               break;
            if (!bom[adrx][adry].hit )
               {
                     bom[adrx][adry].hit = TRUE;
                     leftbombnum--;    
               }
            else
               { bom[adrx][adry].hit = FALSE;
                 leftbombnum++;
               }
            SetRect (&onerect, adrx*WIDTH_BOX+offsetx,
                      adry*HEIGHT_BOX+HEIGHT_FACE, 
                      (adrx+1)*WIDTH_BOX+offsetx,
                      (adry+1)*HEIGHT_BOX+HEIGHT_FACE);
            InvalidateRect (hWnd, &onerect, FALSE);
            InvalidateRect (hWnd, &bombnumber, FALSE);
        break;
      
        case WM_PAINT:
            hdc = BeginPaint (hWnd, &ps);
           
            sprintf(seconds, "%03d", second);
            DrawDigit(hdc, seconds, TRUE);

            if (leftbombnum >= 0){                     
                sprintf(bomn,"%02d",leftbombnum);
                DrawDigit(hdc, bomn, FALSE);
            }
            
            if (flag_finished)
		DrawDIB(hdc, x_face+1, 1, &image_minedone);
#if 0
                DrawMyBitmap (hdc, fValidfinalface?&bmpfinalface:NULL,
#endif
            else 
                if (flag_bombout)
		    DrawDIB(hdc, x_face, 0, &image_minefacelost);
                else
		    DrawDIB(hdc, x_face, 0, &image_mineface);
            
            for (i = 0; i < sg_boxnumx; i++)
                for (j = 0; j < sg_boxnumy; j++)
            {
                if (!bom[i][j].test && !bom[i][j].bombout){
                    SetTextColor (hdc,BLACK);
		    SelectObject(hdc, GetStockObject(NULL_BRUSH));
                    Draw3DUpFrame(hdc,
                            i*WIDTH_BOX+offsetx,
                            j*HEIGHT_BOX+HEIGHT_FACE,
                            (i+1)*WIDTH_BOX+offsetx,
                            (j+1)*HEIGHT_BOX+HEIGHT_FACE,
                            LTGRAY);
                }
              
           
                if ( bom[i][j].hit)
		    DrawDIB(hdc, i*WIDTH_BOX+offsetx+3, j*HEIGHT_BOX+3+HEIGHT_FACE,
				 &image_mineflag);
            
                if (bom[i][j].error) {
                    Cancel3DFrame(hdc,i*WIDTH_BOX+offsetx,
                                 j*HEIGHT_BOX+HEIGHT_FACE,
                                 WIDTH_BOX,HEIGHT_BOX);
                    
		    DrawDIB(hdc, i*WIDTH_BOX+offsetx+1, j*HEIGHT_BOX+1+HEIGHT_FACE,
		       &image_minehitfalse);
                }
                
                if (bom[i][j].bombout) {
                    Cancel3DFrame(hdc,i*WIDTH_BOX+offsetx,
                                        j*HEIGHT_BOX+HEIGHT_FACE,
                                        WIDTH_BOX,HEIGHT_BOX);
                    
		    DrawDIB(hdc, i*WIDTH_BOX+offsetx+1, j*HEIGHT_BOX+1+HEIGHT_FACE,
		       &image_minebomb);
                }
            }
            
            for ( i = 0; i < itime; i++ )
            {
             Cancel3DFrame(hdc,(NoAdr[i].x)*WIDTH_BOX+offsetx,
                        (NoAdr[i].y)*HEIGHT_BOX+HEIGHT_FACE,
                        WIDTH_BOX, HEIGHT_BOX);
             if(NoAdr[i].NY) 
                 TextValue(hdc,(NoAdr[i].x)*WIDTH_BOX+offsetx,
                          (NoAdr[i].y)*HEIGHT_BOX+HEIGHT_FACE, 
                          bom[NoAdr[i].x][NoAdr[i].y].value);
            }
            EndPaint (hWnd, &ps);
        break;    

        case WM_CLOSE:
            KillTimer(hWnd, ID_CLOCK);  
            DestroyWindow (hWnd);
            PostQuitMessage (0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void InitMyWinCreateInfo(PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle = WS_CAPTION | WS_BORDER | WS_SYSMENU |  WS_VISIBLE;
    pCreateInfo->spCaption="Microwindows Minesweeper";
    pCreateInfo->hMenu = 0; 	/* createmenu1(); */
    pCreateInfo->hCursor = 0; 	/* GetSystemCursor(0); */
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = TestMyWinProc;
    pCreateInfo->lx = 0;
    pCreateInfo->ty = 0;
    pCreateInfo->rx = winwidth;
    pCreateInfo->by = winheight;
    pCreateInfo->iBkColor = LTGRAY; 
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting = HWND_DESKTOP;
}

void* TestMyWindow(void* data) 
{
    MSG Msg;
    MAINWINCREATE CreateInfo;
    HWND hMainWnd;

    InitMyWinCreateInfo(&CreateInfo);

    hMainWnd = CreateMainWindow(&CreateInfo);

    if (hMainWnd == 0)
        return NULL;

    ShowWindow(hMainWnd,SW_SHOWNORMAL);
    while( GetMessage(&Msg, NULL, 0, 0) ) {
        TranslateMessage (&Msg);
        DispatchMessage(&Msg);
    }

    return NULL;
}

int WINAPI 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
	int nShowCmd)
{
    TestMyWindow (NULL);
    return 0;
}

/**********   create a hosted about window****/
LRESULT AbHostedWinProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;

    switch (message) {
        case WM_CREATE:
        break;

        case WM_PAINT:
        {
            HDC hdc;

            hdc = BeginPaint (hWnd, &ps);
            SetBkColor(hdc,LTGRAY);
            TextOut (hdc, 10, 25, "Minesweeper Ver 0.9 (1999/08/27)", -1); 
            TextOut (hdc, 10, 55, 
                "Author: Mis. Zheng Xiang (xiang_zi@263.net).", -1);
            TextOut (hdc, 10, 75, 
                "        Mis. Glory (glory@263.net).", -1);
            EndPaint (hWnd, &ps);
        }
        return 0;

        case WM_CLOSE:
            DestroyWindow (hWnd);
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
void InitAbHostedCreateInfo (HWND hHosting, PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle   = WS_BORDER | WS_CAPTION | WS_VISIBLE;
    pCreateInfo->spCaption = "The about window" ;
    pCreateInfo->hMenu = 0;
    pCreateInfo->hCursor = 0; 	/* GetSystemCursor(0); */
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = AbHostedWinProc;
    pCreateInfo->lx = 100; 
    pCreateInfo->ty = 200;
    pCreateInfo->rx = 540;
    pCreateInfo->by = 390;
    pCreateInfo->iBkColor  = LTGRAY; 
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting  = hHosting;
}

/*************************** High Scores Window ******************************/
#define IDC_RESET  110
LRESULT HighScoreWinProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    static HWND hRESET, hOK;
    
    switch (message) {
        case WM_CREATE:
            hRESET = CreateWindowEx (0L, "button",
                        "重置",
                        WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE,
                        100, 145, 60, 26, 
			hWnd, (HMENU)IDC_RESET, 0, 0L);
                        
            hOK    = CreateWindowEx (0L, "button",
                        "确定",
                        WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE,
                        220, 145, 60, 26,
			hWnd, (HMENU)IDOK, 0, 0L);
        break;

        case WM_COMMAND:
        {
            int id = LOWORD(wParam);
            int i;

            if (id == IDC_RESET && HIWORD(wParam) == BN_CLICKED) {
                for (i = 0; i < 3; i++){
                    score[i].highscore = 999;
                    strcpy(score[i].name, "unknown");
                }    
                
#if 0
                FILE* pHighscore;
                if ((pHighscore = fopen("res/.highscore.bomb", "w+"))){
                    for (i = 0; i < 3; i++)
                        fprintf(pHighscore, "%d, %s\n", 
                                    score[i].highscore,score[i].name);
                    fclose(pHighscore);
                }    
#endif
                InvalidateRect(hWnd, NULL, TRUE);
            }
            if (id == IDOK && HIWORD(wParam) == BN_CLICKED)
                PostMessage(hWnd, WM_CLOSE, 0, 0);
        }
        break;
        
        case WM_PAINT:
        {
            HDC hdc;
            char buffer[50];
            int i;
            
            hdc = BeginPaint (hWnd, &ps);
            SetBkColor(hdc,LTGRAY);
            TextOut(hdc, 130, 15, "英雄榜", -1);
            TextOut(hdc, 15, 45, "SMALL" , -1);
            TextOut(hdc, 15, 75, "MIDDLE", -1 );
            TextOut(hdc, 15, 105, "LARGE", -1 );
            for (i = 0; i < 3; i++){
                sprintf(buffer, "%3d          %s", 
                            score[i].highscore, score[i].name);
                TextOut(hdc, 150, 45+i*30, buffer, -1);
            }    
            EndPaint (hWnd, &ps);
        }
        return 0;

        case WM_CLOSE:
            DestroyWindow (hRESET);
            DestroyWindow (hOK);
            DestroyWindow (hWnd);
            hHighscore = HWND_DESKTOP;
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void InitHighScoreCreateInfo (HWND hHosting, PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle   = WS_BORDER | WS_CAPTION;
    pCreateInfo->spCaption = "High Score" ;
    pCreateInfo->hMenu = 0;
    pCreateInfo->hCursor   = 0; 	/* GetSystemCursor(0); */
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = HighScoreWinProc;
    pCreateInfo->lx = 100; 
    pCreateInfo->ty = 200;
    pCreateInfo->rx = 470;
    pCreateInfo->by = 410;
    pCreateInfo->iBkColor  = LTGRAY; 
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting  = hHosting;
}

/************************* Congratulation Window *****************************/

#define IDC_CTRL_NAME       100

LRESULT CongratulationWinProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hPrompt, hName, hOK;
       
    switch (message) {
        case WM_CREATE:
#if 0
            hPrompt = CreateWindow ("static",
                          "请输入大虾尊姓大名: ",
                          SS_SIMPLE | WS_VISIBLE,
                          IDC_STATIC,
                          10, 10, 185, 24, hWnd, 0);
            hName   = CreateWindow ("edit",
                          getlogin(),
                          WS_CHILD | WS_VISIBLE | WS_BORDER,
                          IDC_CTRL_NAME,
                          10, 40, 175, 26, hWnd, 0);
#endif
            hOK     = CreateWindow ("button",
                          "确定",
                          WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE,
                          110, 75, 75, 26,
			  hWnd, (HMENU)IDOK, 0, 0L);
        break;

        case WM_COMMAND:
        {
            int id = LOWORD(wParam);

            if (id == IDOK && HIWORD(wParam) == BN_CLICKED) {
                score[flag_size].highscore = second;
                SendMessage (hName, WM_GETTEXT,
                                19, (LPARAM)score[flag_size].name);
#if 0
                FILE* pHighscore;
                int i;
                if ((pHighscore = fopen("res/.highscore.bomb", "w+"))){
                    for (i = 0; i < 3; i++)
                        fprintf(pHighscore, "%d, %s\n", 
                                    score[i].highscore,score[i].name);
                    fclose(pHighscore);
                }    
#endif
                PostMessage (hWnd, WM_CLOSE, 0, 0);
            }
        }
        break;
                                                                                        case WM_CLOSE:
            DestroyWindow (hPrompt);
            DestroyWindow (hName);
            DestroyWindow (hOK);
            DestroyWindow (hWnd);
            hCongratulate = HWND_DESKTOP;
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void InitCongratulationCreateInfo (HWND hHosting, PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle   = WS_BORDER | WS_CAPTION | WS_VISIBLE;
    pCreateInfo->spCaption = "Congratulation" ;
    pCreateInfo->hMenu = 0;
    pCreateInfo->hCursor   = 0; 	/* GetSystemCursor(0); */
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = CongratulationWinProc;
    pCreateInfo->lx = 50; 
    pCreateInfo->ty = 60;
    pCreateInfo->rx = 255;
    pCreateInfo->by = 200;
    pCreateInfo->iBkColor  = LTGRAY; 
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting  = hHosting;
}

HWND
CreateMainWindow(PMAINWINCREATE pCreateInfo)
{
	HWND		hwnd;
	RECT		rc;
	WNDCLASS	wc;

	wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)pCreateInfo->MainWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = 0;
	wc.hIcon = pCreateInfo->hIcon;
	wc.hCursor = pCreateInfo->hCursor;
	wc.hbrBackground = (HBRUSH)CreateSolidBrush(pCreateInfo->iBkColor);
	wc.lpszMenuName = 0;
	wc.lpszClassName = pCreateInfo->spCaption;
	RegisterClass(&wc);

	SetRect(&rc, pCreateInfo->lx, pCreateInfo->ty, pCreateInfo->rx,
		pCreateInfo->by);
	AdjustWindowRectEx(&rc, pCreateInfo->dwStyle, FALSE, 0L);

	hwnd = CreateWindowEx(0L, pCreateInfo->spCaption,
			pCreateInfo->spCaption,
			pCreateInfo->dwStyle,
			rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
			pCreateInfo->hHosting,
			(HMENU)0, (HINSTANCE)0,
			(PVOID)pCreateInfo->dwAddData);
	return hwnd;
}

BOOL
PtInRect2(const RECT *lprc, int x, int y)
{
	POINT	p;

	p.x = x;
	p.y = y;
	return PtInRect(lprc, p);
}

void
Draw3DUpFrame(HDC hDC, int l, int t, int r, int b, int fillc)
{
	RECT	rc;
	HBRUSH	hbr;

	SetRect(&rc, l, t, r, b);
	Draw3dBox(hDC, rc.left, rc.top,
		rc.right-rc.left, rc.bottom-rc.top,
		GetSysColor(COLOR_3DLIGHT),
		GetSysColor(COLOR_WINDOWFRAME));
	InflateRect(&rc, -1, -1);
	Draw3dBox(hDC, rc.left, rc.top,
		rc.right-rc.left, rc.bottom-rc.top,
		GetSysColor(COLOR_BTNHIGHLIGHT),
		GetSysColor(COLOR_BTNSHADOW));
	InflateRect(&rc, -1, -1);

	hbr = CreateSolidBrush(LTGRAY);
	FillRect(hDC, &rc, hbr);
	DeleteObject(hbr);
}

/* winctl.h*/
/*
 * Header file for builtin controls
 * This currently includes button, progressbar, listbox, edit
 */

/* entry points*/
int WINAPI	MwRegisterButtonControl(HINSTANCE hInstance);
int WINAPI	MwRegisterListboxControl(HINSTANCE hInstance);
int WINAPI	MwRegisterEditControl(HINSTANCE hInstance);
int WINAPI	MwRegisterStaticControl(HINSTANCE hInstance);
int WINAPI	MwRegisterProgressBarControl(HINSTANCE hInstance);
int WINAPI	MwRegisterComboboxControl(HINSTANCE hInstance);
int WINAPI	MwRegisterScrollbarControl(HINSTANCE hInstance);
int WINAPI	MwRegisterMEditControl(HINSTANCE hInstance);

/* temporarily in button.c*/
void WINAPI	CheckRadioButton(HWND hDlg, int nIDFirst,int nIDLast,
			int nIDCheckButton);

/* Dialog Codes*/
#define DLGC_WANTARROWS     0x0001      /* Control wants arrow keys         */
#define DLGC_WANTTAB        0x0002      /* Control wants tab keys           */
#define DLGC_WANTALLKEYS    0x0004      /* Control wants all keys           */
#define DLGC_WANTMESSAGE    0x0004      /* Pass message to control          */
#define DLGC_HASSETSEL      0x0008      /* Understands EM_SETSEL message    */
#define DLGC_DEFPUSHBUTTON  0x0010      /* Default pushbutton               */
#define DLGC_UNDEFPUSHBUTTON 0x0020     /* Non-default pushbutton           */
#define DLGC_RADIOBUTTON    0x0040      /* Radio button                     */
#define DLGC_WANTCHARS      0x0080      /* Want WM_CHAR messages            */
#define DLGC_STATIC         0x0100      /* Static item: don't include       */
#define DLGC_BUTTON         0x2000      /* Button item: can be checked      */

/* Button Control Styles*/
#define BS_PUSHBUTTON       0x00000000L
#define BS_DEFPUSHBUTTON    0x00000001L
#define BS_CHECKBOX         0x00000002L
#define BS_AUTOCHECKBOX     0x00000003L
#define BS_RADIOBUTTON      0x00000004L
#define BS_3STATE           0x00000005L
#define BS_AUTO3STATE       0x00000006L
#define BS_GROUPBOX         0x00000007L
#define BS_USERBUTTON       0x00000008L
#define BS_AUTORADIOBUTTON  0x00000009L
#define BS_OWNERDRAW        0x0000000BL
#define BS_LEFTTEXT         0x00000020L
#define BS_TEXT             0x00000000L
#define BS_ICON             0x00000040L
#define BS_BITMAP           0x00000080L
#define BS_LEFT             0x00000100L
#define BS_RIGHT            0x00000200L
#define BS_CENTER           0x00000300L
#define BS_TOP              0x00000400L
#define BS_BOTTOM           0x00000800L
#define BS_VCENTER          0x00000C00L
#define BS_PUSHLIKE         0x00001000L
#define BS_MULTILINE        0x00002000L
#define BS_NOTIFY           0x00004000L
#define BS_FLAT             0x00008000L
#define BS_RIGHTBUTTON      BS_LEFTTEXT

/* User Button Notification Codes*/
#define BN_CLICKED          0
#define BN_PAINT            1
#define BN_HILITE           2
#define BN_UNHILITE         3
#define BN_DISABLE          4
#define BN_DOUBLECLICKED    5
#define BN_PUSHED           BN_HILITE
#define BN_UNPUSHED         BN_UNHILITE
#define BN_DBLCLK           BN_DOUBLECLICKED
#define BN_SETFOCUS         6
#define BN_KILLFOCUS        7

/* Button Control Messages*/
#define BM_GETCHECK        0x00F0
#define BM_SETCHECK        0x00F1
#define BM_GETSTATE        0x00F2
#define BM_SETSTATE        0x00F3
#define BM_SETSTYLE        0x00F4
#define BM_CLICK           0x00F5
#define BM_GETIMAGE        0x00F6
#define BM_SETIMAGE        0x00F7

#define BST_UNCHECKED      0x0000
#define BST_CHECKED        0x0001
#define BST_INDETERMINATE  0x0002
#define BST_PUSHED         0x0004
#define BST_FOCUS          0x0008

/* Progress Bar messages*/
#define PBM_SETRANGE		0xF0A0
#define PBM_SETSTEP		0xF0A1
#define PBM_SETPOS		0xF0A2
#define PBM_DELTAPOS		0xF0A3
#define PBM_STEPIT		0xF0A4

/* Progress Bar styles */
#define PBS_NOTIFY              0x0001L
#define PBS_VERTICAL            0x0002L

/* Progress Bar notification code */
#define PBN_REACHMAX            1
#define PBN_REACHMIN            2

/* Listbox messages*/
#define LB_ADDSTRING            0xF180
#define LB_INSERTSTRING         0xF181
#define LB_DELETESTRING         0xF182
#define LB_SELITEMRANGEEX       0xF183
#define LB_RESETCONTENT         0xF184
#define LB_SETSEL               0xF185
#define LB_SETCURSEL            0xF186
#define LB_GETSEL               0xF187
#define LB_GETCURSEL            0xF188
#define LB_GETTEXT              0xF189
#define LB_GETTEXTLEN           0xF18A
#define LB_GETCOUNT             0xF18B
#define LB_SELECTSTRING         0xF18C
#define LB_DIR                  0xF18D
#define LB_GETTOPINDEX          0xF18E
#define LB_FINDSTRING           0xF18F
#define LB_GETSELCOUNT          0xF190
#define LB_GETSELITEMS          0xF191
#define LB_SETTABSTOPS          0xF192
#define LB_GETHORIZONTALEXTENT  0xF193
#define LB_SETHORIZONTALEXTENT  0xF194
#define LB_SETCOLUMNWIDTH       0xF195
#define LB_ADDFILE              0xF196
#define LB_SETTOPINDEX          0xF197
#define LB_GETITEMRECT          0xF198
#define LB_GETITEMDATA          0xF199
#define LB_SETITEMDATA          0xF19A
#define LB_SELITEMRANGE         0xF19B
#define LB_SETANCHORINDEX       0xF19C
#define LB_GETANCHORINDEX       0xF19D
#define LB_SETCARETINDEX        0xF19E
#define LB_GETCARETINDEX        0xF19F
#define LB_SETITEMHEIGHT        0xF1A0
#define LB_GETITEMHEIGHT        0xF1A1
#define LB_FINDSTRINGEXACT      0xF1A2
#define LB_SETLOCALE            0xF1A5
#define LB_GETLOCALE            0xF1A6
#define LB_SETCOUNT             0xF1A7
#define LB_INITSTORAGE          0xF1A8
#define LB_ITEMFROMPOINT        0xF1A9
#define LB_SETTEXT              0xF1AA
#define LB_GETCHECKMARK         0xF1AB
#define LB_SETCHECKMARK         0xF1AC
#define LB_GETITEMADDDATA       0xF1AD
#define LB_SETITEMADDDATA       0xF1AE
#define LB_MSGMAX               0xF1B0

/* Listbox styles */
#define LBS_NOTIFY              0x0001L
#define LBS_SORT                0x0002L
#define LBS_NOREDRAW            0x0004L		/* not supported*/
#define LBS_MULTIPLESEL         0x0008L
#define LBS_OWNERDRAWFIXED      0x0010L		/* nyi*/
#define LBS_OWNERDRAWVARIABLE   0x0020L		/* nyi*/
#define LBS_HASSTRINGS          0x0040L		/* not supported*/
#define LBS_USETABSTOPS         0x0080L		/* nyi*/
#define LBS_NOINTEGRALHEIGHT    0x0100L		/* not supported*/
#define LBS_MULTICOLUMN         0x0200L		/* nyi*/
#define LBS_WANTKEYBOARDINPUT   0x0400L		/* not supported*/
#define LBS_EXTENDEDSEL         0x0800L		/* not supported*/
#define LBS_STANDARD		(LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)
#define LBS_CHECKBOX            0x1000L		/* non std*/
#define LBS_USEICON             0x2000L		/* non std*/
#define LBS_AUTOCHECK           0x4000L		/* non std*/
#define LBS_AUTOCHECKBOX        0x5000L		/* non std*/
/* private Microwindows styles for combobox*/
#define	LBS_PRELOADED		0x4000L		/* Microwindows private*/
#define	LBS_COMBOLBOX		0x8000L		/* Microwindows private*/

#if 0
#define LBS_DISABLENOSCROLL	0x1000L
#define LBS_NODATA		0x2000L
#define LBS_NOSEL		0x4000L
#endif

/* Listbox Notification Codes */
#define LBN_ERRSPACE        (-2)
#define LBN_SELCHANGE       1
#define LBN_DBLCLK          2
#define LBN_SELCANCEL       3
#define LBN_SETFOCUS        4
#define LBN_KILLFOCUS       5
#define LBN_CLICKCHECKMARK  6		/* non std*/

/* Listbox return value */
#define LB_OKAY             0
#define LB_ERR              (-1)
#define LB_ERRSPACE         (-2)

/* Edit Control Notification Codes*/
#define EN_SETFOCUS         0x0100
#define EN_KILLFOCUS        0x0200
#define EN_CHANGE           0x0300
#define EN_UPDATE           0x0400
#define EN_ERRSPACE         0x0500
#define EN_MAXTEXT          0x0501
#define EN_HSCROLL          0x0601
#define EN_VSCROLL          0x0602

/* Edit Control Styles*/
#define ES_LEFT             0x0000L
#define ES_CENTER           0x0001L
#define ES_RIGHT            0x0002L
#define ES_MULTILINE        0x0004L
#define ES_UPPERCASE        0x0008L
#define ES_LOWERCASE        0x0010L
#define ES_PASSWORD         0x0020L
#define ES_AUTOVSCROLL      0x0040L
#define ES_AUTOHSCROLL      0x0080L
#define ES_NOHIDESEL        0x0100L
#define ES_OEMCONVERT       0x0400L
#define ES_READONLY         0x0800L
#define ES_WANTRETURN       0x1000L
#define ES_NUMBER           0x2000L

/* Edit Control Messages*/
#define EM_GETSEL               0xF0B0
#define EM_SETSEL               0xF0B1
#define EM_GETRECT              0xF0B2
#define EM_SETRECT              0xF0B3
#define EM_SETRECTNP            0xF0B4
#define EM_SCROLL               0xF0B5
#define EM_LINESCROLL           0xF0B6
#define EM_SCROLLCARET          0xF0B7
#define EM_GETMODIFY            0xF0B8
#define EM_SETMODIFY            0xF0B9
#define EM_GETLINECOUNT         0xF0BA
#define EM_LINEINDEX            0xF0BB
#define EM_SETHANDLE            0xF0BC
#define EM_GETHANDLE            0xF0BD
#define EM_GETTHUMB             0xF0BE
#define EM_LINELENGTH           0xF0C1
#define EM_REPLACESEL           0xF0C2
#define EM_GETLINE              0xF0C4
#define EM_LIMITTEXT            0xF0C5
#define EM_CANUNDO              0xF0C6
#define EM_UNDO                 0xF0C7
#define EM_FMTLINES             0xF0C8
#define EM_LINEFROMCHAR         0xF0C9
#define EM_SETTABSTOPS          0xF0CB
#define EM_SETPASSWORDCHAR      0xF0CC
#define EM_EMPTYUNDOBUFFER      0xF0CD
#define EM_GETFIRSTVISIBLELINE  0xF0CE
#define EM_SETREADONLY          0xF0CF
#define EM_SETWORDBREAKPROC     0xF0D0
#define EM_GETWORDBREAKPROC     0xF0D1
#define EM_GETPASSWORDCHAR      0xF0D2
#define EM_SETMARGINS           0xF0D3
#define EM_GETMARGINS           0xF0D4
#define EM_SETLIMITTEXT         EM_LIMITTEXT
#define EM_GETLIMITTEXT         0xF0D5
#define EM_POSFROMCHAR          0xF0D6
#define EM_CHARFROMPOS          0xF0D7
#define EM_SETIMESTATUS         0xF0D8
#define EM_GETIMESTATUS         0xF0D9

/* Static Control messages*/
#define STM_SETICON         0xF170
#define STM_GETICON         0xF171
#define STM_SETIMAGE        0xF172
#define STM_GETIMAGE        0xF173
#define STM_MSGMAX          0xF174

/* Static Control notification code*/
#define STN_CLICKED         0
#define STN_DBLCLK          1
#define STN_ENABLE          2
#define STN_DISABLE         3

/* Static Control Styles */
#define SS_LEFT             0x00000000L
#define SS_CENTER           0x00000001L
#define SS_RIGHT            0x00000002L
#define SS_ICON             0x00000003L
#define SS_BLACKRECT        0x00000004L
#define SS_GRAYRECT         0x00000005L
#define SS_WHITERECT        0x00000006L
#define SS_BLACKFRAME       0x00000007L
#define SS_GRAYFRAME        0x00000008L
#define SS_WHITEFRAME       0x00000009L
#define SS_GROUPBOX         0x0000000AL
#define SS_SIMPLE           0x0000000BL
#define SS_LEFTNOWORDWRAP   0x0000000CL
#define SS_OWNERDRAW        0x0000000DL
#define SS_BITMAP           0x0000000EL
#define SS_ENHMETAFILE      0x0000000FL
#define SS_TYPEMASK         0x0000000FL
#define SS_NOPREFIX         0x00000080L
#define SS_ETCHEDHORZ       0x00000010L
#define SS_ETCHEDVERT       0x00000011L
#define SS_ETCHEDFRAME      0x00000012L
#define SS_ETCTYPEMAKS      0x0000001FL
#define SS_NOTIFY           0x00000100L
#define SS_CENTERIMAGE      0x00000200L
#define SS_RIGHTJUST        0x00000400L
#define SS_REALSIZEIMAGE    0x00000800L
#define SS_SUNKEN           0x00001000L		/* notimp*/
#define SS_ENDELLIPSIS      0x00004000L		/* notimp*/
#define SS_PATHELLIPSIS     0x00008000L		/* notimp*/
#define SS_WORDELLIPSIS     0x0000C000L		/* notimp*/
#define SS_ELLIPSISMASK     0x0000C000L		/* notimp*/

/* Combo Box styles*/
#define CBS_SIMPLE            0x0001L
#define CBS_DROPDOWN          0x0002L
#define CBS_DROPDOWNLIST      0x0003L
#define CBS_OWNERDRAWFIXED    0x0010L
#define CBS_OWNERDRAWVARIABLE 0x0020L
#define CBS_AUTOHSCROLL       0x0040L
#define CBS_OEMCONVERT        0x0080L
#define CBS_SORT              0x0100L
#define CBS_HASSTRINGS        0x0200L
#define CBS_NOINTEGRALHEIGHT  0x0400L
#define CBS_DISABLENOSCROLL   0x0800L
#define CBS_UPPERCASE         0x2000L
#define CBS_LOWERCASE         0x4000L

/* Combo box messages */
#define CB_GETEDITSEL               0xF140
#define CB_LIMITTEXT                0xF141
#define CB_SETEDITSEL               0xF142
#define CB_ADDSTRING                0xF143
#define CB_DELETESTRING             0xF144
#define CB_DIR                      0xF145
#define CB_GETCOUNT                 0xF146
#define CB_GETCURSEL                0xF147
#define CB_GETLBTEXT                0xF148
#define CB_GETLBTEXTLEN             0xF149
#define CB_INSERTSTRING             0xF14A
#define CB_RESETCONTENT             0xF14B
#define CB_FINDSTRING               0xF14C
#define CB_SELECTSTRING             0xF14D
#define CB_SETCURSEL                0xF14E
#define CB_SHOWDROPDOWN             0xF14F
#define CB_GETITEMDATA              0xF150
#define CB_SETITEMDATA              0xF151
#define CB_GETDROPPEDCONTROLRECT    0xF152
#define CB_SETITEMHEIGHT            0xF153
#define CB_GETITEMHEIGHT            0xF154
#define CB_SETEXTENDEDUI            0xF155
#define CB_GETEXTENDEDUI            0xF156
#define CB_GETDROPPEDSTATE          0xF157
#define CB_FINDSTRINGEXACT          0xF158
#define CB_SETLOCALE                0xF159	/* notimp*/
#define CB_GETLOCALE                0xF15A	/* notimp*/
#define CB_GETTOPINDEX              0xF15b	/* notimp*/
#define CB_SETTOPINDEX              0xF15c	/* notimp*/
#define CB_GETHORIZONTALEXTENT      0xF15d	/* notimp*/
#define CB_SETHORIZONTALEXTENT      0xF15e	/* notimp*/
#define CB_GETDROPPEDWIDTH          0xF15f	/* notimp*/
#define CB_SETDROPPEDWIDTH          0xF160	/* notimp*/
#define CB_INITSTORAGE              0xF161	/* notimp*/
#define CB_MSGMAX                   0xF162

/* Combo box notification codes */
#define CBN_ERRSPACE        (-1)
#define CBN_SELCHANGE       1
#define CBN_DBLCLK          2
#define CBN_SETFOCUS        3
#define CBN_KILLFOCUS       4
#define CBN_EDITCHANGE      5
#define CBN_EDITUPDATE      6
#define CBN_DROPDOWN        7
#define CBN_CLOSEUP         8
#define CBN_SELENDOK        9
#define CBN_SELENDCANCEL    10

/* Combo box message return values */
#define CB_OKAY         0
#define CB_ERR          (-1)
#define CB_ERRSPACE     (-2)

/* scroll bar control styles*/
#define SBS_HORZ                    0x0000L
#define SBS_VERT                    0x0001L
#define SBS_TYPEMASK		    0x0001L
#define SBS_TOPALIGN                0x0002L
#define SBS_LEFTALIGN               0x0002L
#define SBS_BOTTOMALIGN             0x0004L
#define SBS_RIGHTALIGN              0x0004L
#define SBS_SIZEBOXTOPLEFTALIGN     0x0002L
#define SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L
#define SBS_SIZEBOX                 0x0008L
#define SBS_SIZEGRIP                0x0010L

/* scroll bar constants*/
#define SB_HORZ             0
#define SB_VERT             1
#define SB_CTL              2
#define SB_BOTH             3

/* scroll bar notify codes*/
#define SB_LINEUP           0
#define SB_LINELEFT         0
#define SB_LINEDOWN         1
#define SB_LINERIGHT        1
#define SB_PAGEUP           2
#define SB_PAGELEFT         2
#define SB_PAGEDOWN         3
#define SB_PAGERIGHT        3
#define SB_THUMBPOSITION    4
#define SB_THUMBTRACK       5
#define SB_TOP              6
#define SB_LEFT             6
#define SB_BOTTOM           7
#define SB_RIGHT            7
#define SB_ENDSCROLL        8

#define SIF_RANGE           0x0001
#define SIF_PAGE            0x0002
#define SIF_POS             0x0004
#define SIF_DISABLENOSCROLL 0x0008
#define SIF_TRACKPOS        0x0010
#define SIF_ALL             (SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS)

typedef struct tagSCROLLINFO {
    UINT    cbSize;
    UINT    fMask;
    int     nMin;
    int     nMax;
    UINT    nPage;
    int     nPos;
    int     nTrackPos;
} SCROLLINFO, *LPSCROLLINFO;
typedef SCROLLINFO CONST *LPCSCROLLINFO;

int     WINAPI SetScrollInfo(HWND, int, LPCSCROLLINFO, BOOL);
BOOL    WINAPI GetScrollInfo(HWND, int, LPSCROLLINFO);

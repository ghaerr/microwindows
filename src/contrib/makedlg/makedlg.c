#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>



#ifndef PSTR
typedef const char *PSTR;
#endif



#define STAT_NULL		0
#define STAT_INITDLG		1
#define STAT_DLGCTRLS		2
#define STAT_DLGCTRLSON		3


char *declarations = 
    "#define MW_DIALOGS_CORE\n"
    "#include <windows.h>\n"
    "#include <mwdialog.h>\n"
    "#include <memory.h>\n"
    "#include <resource.h>\n"
    "\n"
    "\n"
    "\n"
    "";


typedef struct tagDlgId
	{
	char *dlgId;
	struct tagDlgId *next;
	} DlgId;






#define DEFCTRLSSTYLE	"WS_CHILD | WS_VISIBLE"
#define DLGBASEUNITSX	6//0x0007
#define DLGBASEUNITSY	13//0x0010


int dlgCoordsX ( PSTR str )
{
	int i = atoi ( str );

	return (i * DLGBASEUNITSX) / 4;
}


int dlgCoordsY ( PSTR str )
{
	int i = atoi ( str );

	return (i * DLGBASEUNITSY) / 8;
}



typedef struct tagControlTypes
	{
	PSTR sName;
	PSTR sClass;
	PSTR sStyle;
	} ControlTypes;

ControlTypes controlsDef[] = 
	{
		{"CONTROL", NULL, NULL},
		{"DEFPUSHBUTTON", "\"BUTTON\"", "BS_DEFPUSHBUTTON | WS_BORDER | WS_TABSTOP"},
		{"PUSHBUTTON", "\"BUTTON\"", "WS_BORDER | WS_TABSTOP"},
		{"GROUPBOX", "\"BUTTON\"", "BS_GROUPBOX"},
		{"LISTBOX", "\"LISTBOX\"", "WS_BORDER"},
		{"COMBOBOX", "\"COMBOBOX\"", "WS_BORDER | WS_TABSTOP"},
		{"EDITTEXT", "\"EDIT\"", "WS_BORDER | WS_TABSTOP"},
		{"LTEXT", "\"STATIC\"", "SS_LEFT"},
		{"CTEXT", "\"STATIC\"", "SS_CENTER"},
		{"RTEXT", "\"STATIC\"", "SS_RIGHT"},
		{NULL, NULL, NULL}
	};




/*
LRESULT CALLBACK g_dlg_Function ( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam )
{
	switch ( Msg )
		{
		}

	return DefWindowProc ( hWnd, Msg, wParam, lParam );
}
*/


void initDlgFunction ( FILE *dst, const char *dlgName )
{
	fprintf ( dst, "\n\n" );
	fprintf ( dst, "static HWND create_dlg_%s \n\t(\n\tHINSTANCE hInst, HWND hParent, HWND *hFocus\n\t)\n", dlgName );
	fprintf ( dst, "{\n" );
}



void headerDlgFunction ( FILE *dst, PSTR strStyle, PSTR strTitle, 
						 PSTR strFont, int szFont, char* strCoords )
{
	char *szX = strtok ( strCoords, "," );
	char *szY = strtok ( NULL, "," );
	char *szSX = strtok ( NULL, "," );
	char *szSY = strtok ( NULL, "," );
	int x, y, sx, sy;

	if( !szX || !szY || !szSX || !szSY )
		{
		fprintf ( dst, "/* ERROR: bad coords on dialog %s. */\n", strTitle );
		return;
		}

	x = dlgCoordsX ( szX );
	y = dlgCoordsY ( szY );
	sx = dlgCoordsX ( szSX );
	sy = dlgCoordsY ( szSY );

	/*  Variables declaration  */
	fprintf ( dst, "    HWND hDlg;\n" );

	/*  Dialog creation  */
	fprintf ( dst, "    hDlg = CreateWindowEx (\n" );
	fprintf ( dst, "                0,\n" );
	fprintf ( dst, "                \"GDLGCLASS\",\n" );
	fprintf ( dst, "                \"%s\",\n", strTitle );
	fprintf ( dst, "                %s,\n", strStyle );
	fprintf ( dst, "                %d , %d, %d, %d,\n", x, y, sx, sy );
	fprintf ( dst, "                hParent,\n" );
	fprintf ( dst, "                NULL,\n" );
	fprintf ( dst, "                hInst,\n" );
	fprintf ( dst, "                0\n" );
	fprintf ( dst, "                );\n\n" );

	fprintf ( dst, "    if( hDlg )\n" );
	fprintf ( dst, "        {\n" );
}



void endDlgFunction ( FILE *dst )
{
	fprintf ( dst, "        }\n\n" );
	fprintf ( dst, "    return hDlg;\n" );
	fprintf ( dst, "}\n\n" );
}



/*
 *	Return the control type specified by name, or NULL if is not a control.
 */
ControlTypes* getControlType ( PSTR name )
{
	ControlTypes* obj = controlsDef;

	while ( obj->sName )
		{
		if( !strcmp(obj->sName, name) )
			return obj;

		obj++;
		}

	return NULL;
}


static int firstFocus = 0;


void dlgPutControl ( FILE *dst, char *strControl, ControlTypes *control )
{
	char *text;
	char *id;
	char *sClass;
	char *style;
	char *szX, *szY, *szSX, *szSY;
	int x, y, sx, sy;
	char negStyles [64];
	char intText [20];

	while ( *strControl == ' ' || *strControl == '\t' ) strControl++;

	negStyles[0] = 0;
	if( strControl[0] == '"' )
	    {
	    text = strControl;
	    for ( ++strControl; *strControl != '"'; strControl++ );
	    *(++strControl) = 0;
	    id = strtok ( ++strControl, "," );
	    }
	else
	    {
	    /*  If base control, the first item is always text, also if
	        don't become with " (bitmap for example).   */
	    if( control->sClass == NULL )
		{
		text = strtok ( strControl, "," );
		sprintf ( intText, "\"%d\"", atoi(text) );
		text = intText;
		id = strtok ( NULL, "," );
		}
	    else
		{
		id = strtok ( strControl, "," );
		text = "\"\"";
		}
	    }
/*
	if( id[0] == '"' )
		{
		text = id;
		id = strtok ( NULL, "," );
		}
	else
		text = "\"\"";
*/
	if( control->sClass )
		{
		sClass = (char*) control->sClass;
		szX = strtok ( NULL, "," );
		szY = strtok ( NULL, "," );
		szSX = strtok ( NULL, "," );
		szSY = strtok ( NULL, "," );
		style = strtok ( NULL, "" );
		}
	else
		{
		sClass = strtok ( NULL, "," );
		style = strtok ( NULL, "," );
		szX = strtok ( NULL, "," );
		szY = strtok ( NULL, "," );
		szSX = strtok ( NULL, "," );
		szSY = strtok ( NULL, "" );
		}

	if( id && sClass && szX && szY && szSX && szSY )
		{
		x = dlgCoordsX ( szX );
		y = dlgCoordsY ( szY );
		sx = dlgCoordsX ( szSX );
		sy = dlgCoordsY ( szSY );

		fprintf ( dst, "		/* %s(%s) */\n", sClass, text );

		if( !firstFocus && ((style && strstr(style, "WS_TABSTOP")) ||
		    (control->sStyle && strstr(control->sStyle, "WS_TABSTOP"))) )
			{
			fprintf ( dst, "        *hFocus = \\\n" );
			firstFocus = 1;
			}

		fprintf ( dst, "        CreateWindowEx ( 0, %s, %s,\n", sClass, text );
		if( control->sStyle )
			fprintf ( dst, "                (%s | %s", DEFCTRLSSTYLE, control->sStyle );
		else
			fprintf ( dst, "                (%s", DEFCTRLSSTYLE );
			
		if( style )
		    {
		    int prevNot = 0;
		    char *ss = strtok ( style, " " );
		    while ( ss )
			{
			if( !strcmp(ss, "NOT") ) prevNot = 1;
			if( !prevNot && ss[0] != '|' )
			    fprintf ( dst, " | %s", ss );
			ss = strtok ( NULL, " " );
			if( ss && prevNot )
			    {
			    if( negStyles[0] ) strcat ( negStyles, " | " );
			    strcat ( negStyles, ss );
			    prevNot = 0;
			    }
			}
		    }
		    
		fprintf ( dst, " )" );

		if( strlen(negStyles) )
		    fprintf ( dst, " & ~(%s)", negStyles );

		fprintf ( dst, ",\n" );	
		fprintf ( dst, "                %d, %d, %d, %d, hDlg, (HMENU)%s, hInst, NULL );\n", 
				  x, y, sx, sy, id );
		}
	else
		fprintf ( dst, "/*  WARNING:  error on control %s. */\n", control->sName );
}



int main ( int argc, char *argv[] )
{
	static char strControl [1024];
	static char s [350];
	static char strStyle [128];
	static char strTitle [256];
	static char strFont [40];
	static char strCoords [32];

	ControlTypes *control, *curControl;
	int szFont;
	int ln;
	char *ps;
	char *ctrlName, *ctrlDecl;

	FILE *rc, *dst;
	DlgId *firstID = NULL;

	int stat = 0;
	curControl = NULL;

	if( argc < 3 )
		{
		printf ( "Usare: makedlg <srcfile> <dstfile>\n" );
		exit ( 1 );
		}

	rc = fopen ( argv[1], "rt" );
	if( rc == NULL )
		{
		printf ( "Impossibile aprire il file %s.\n", argv[1] );
		exit ( 1 );
		}

	dst = fopen ( argv[2], "wt" );
	if( dst == NULL )
		{
		printf ( "Impossibile creare il file di destinazione %s.\n", argv[2] );
		exit ( 1 );
		}


	strStyle[0] = 0;
	strTitle[0] = 0;
	strFont[0] = 0;
	strCoords[0] = 0;

	fputs ( declarations, dst );

	while ( fgets(s, sizeof(s), rc) )
		{
		ln = strlen ( s );
		if( ln && s[ln-1] == '\n' ) s[--ln] = 0;
		if( ln && s[ln-1] == '\r' ) s[--ln] = 0;
		ps = s;
		while ( *ps == ' ' || *ps == '\t' ) ps++, ln--;

		switch ( stat )
			{
			case STAT_NULL:
				if( !strncmp(ps, "#include", 8) && strstr(ps, "resource.h") )
					{
					//fputs ( ps, dst );
					break;
					}
				if( strstr(ps, "DIALOG") )
					{
					char *name = strtok ( ps, " " );
					char *dlg = strtok ( NULL, " " );
					char *p = strtok ( NULL, "" );
					while ( p && *p && !isdigit(*p) ) p++;
					if( name && dlg && p && isdigit(*p) )
						{
						/*  Store the informations about the new dialog. */
						DlgId *newDlg = malloc ( sizeof(DlgId) );
						newDlg->dlgId = malloc ( strlen(name)+1 );
						strcpy ( newDlg->dlgId, name );
						newDlg->next = firstID;
						firstID = newDlg;

						strcpy ( strCoords, p );
						initDlgFunction ( dst, name );
						stat = STAT_INITDLG;
						}
					break;
					}
				break;

			case STAT_INITDLG:
				if( !strncmp(ps, "STYLE", 5) )
					{
					char *head = strtok ( ps, " " );
					char *style = strtok ( NULL, "" );
					if( head && style )
						strcpy ( strStyle, style );
					break;
					}
				if( !strncmp(ps, "CAPTION", 7) )
					{
					char *head = strtok ( ps, " " );
					char *caption = strtok ( NULL, "" );
					if( head && caption )
						{
						if( caption[0] == '"' ) caption++;
						ln = strlen(caption);
						if( ln && caption[ln-1] == '"' ) caption[ln-1] = 0;
						strcpy ( strTitle, caption );
						}
					break;
					}
				if( !strncmp(ps, "FONT", 4) )
					{
					char *head = strtok ( ps, " " );
					char *sz = strtok ( NULL, "," );
					char *name = strtok ( NULL, "" );
					while ( name && *name == ' ' ) name++;
					if( head && sz && name )
						{
						szFont = atoi ( sz );
						if( name[0] == '"' ) name++;
						ln = strlen(name);
						if( ln && name[ln-1] == '"' ) name[--ln] = 0;
						strcpy ( strFont, name );
						}
					break;
					}
				if( !strncmp(ps, "BEGIN", 5) )
					{
					headerDlgFunction ( dst, strStyle, strTitle, strFont, 
										szFont, strCoords );
					stat = STAT_DLGCTRLS;
					curControl = NULL;
					firstFocus = 0;
					break;
					}
				break;

			case STAT_DLGCTRLS:
				if( !strncmp(ps, "END", 3) )
					{
					if( curControl )
						dlgPutControl ( dst, strControl, curControl );
					stat = STAT_NULL;
					endDlgFunction ( dst );
					break;
					}
				ctrlName = strtok ( ps, " " );
				ctrlDecl = strtok ( NULL, "" );
				if( ctrlName )
					{
					control = getControlType ( ctrlName );
					if( control )
						{
						if( curControl )
							dlgPutControl ( dst, strControl, curControl );

						curControl = control;
						if( ctrlDecl ) 
							strcpy ( strControl, ctrlDecl );
						}
					else
						{
						if( curControl )
							{
							strcat ( strControl, ctrlName );
							strcat ( strControl, " " );
							if( ctrlDecl )
								strcat ( strControl, ctrlDecl );
							}
						else
							fprintf ( dst, "/* WARNING: Invalid control %s */\n", ctrlName );
						}
					}
				break;
			}
		}

		
	/*  Flush the ID objects  */
	fprintf ( dst, "DlgDesc dialogTemplates[] =\n{\n" );

	while ( firstID )
		{
		DlgId *next = firstID->next;
		fprintf ( dst, "    { %s,\tcreate_dlg_%s },\n", firstID->dlgId, firstID->dlgId );
		free ( firstID->dlgId );
		free ( firstID );
		firstID = next;
		}

	fprintf ( dst, "    { 0, NULL }\n" );
	fprintf ( dst, "};\n" );
	fclose ( dst );
	fclose ( rc );


	return 0;
}

%a 3000
%{
#include<tnWidgets.h>
#include<math.h>
#include<string.h>
#include "y.tab.h"
int lineno = 0;
int i;
%}

%%

widgetname	 return WIDGETNAME; 
type		 return TYPE;
parent 		 return PARENT;
xpos 		 return XPOS;
ypos		 return YPOS;
height 		 return HEIGHT;
width		 return WIDTH;
enabled		 return ENABLED;
visible		 return VISIBLE;
caption 	 return CAPTION;
callbackcount	 return CALLBACKCOUNT;
resize		 return RESIZE;
callback	 return CALLBACK;
stretch		 return STRETCH;
filename	 return FILENAME; 
haspixmap	 return HASPIXMAP;
defaulttext	 return DEFAULTTEXT;
orientation	 return ORIENTATION;
minval		 return MINVAL;
maxval		 return MAXVAL;
pagestep	 return PAGESTEP;
linestep	 return LINESTEP;
fillcolor	 return FILLCOLOR;
fgcolor		 return FGCOLOR;
discrete	 return DISCRETE;
stepsize	 return STEPSIZE;
exclusive	 return EXCLUSIVE;
label		 return LABEL;
checkable	 return CHECKABLE;
menubarypos	 return MENUBARYPOS;
\$	 	 return EOW;
[0-9]+   	{ yylval.number = atoi(yytext); return NUMBER;}
[a-zA-Z0-9_]+ 	{ strcpy(yylval.name,yytext); return NAME; }
\".*\"          { for(i=1;yytext[i]!= '"';i++)
			yylval.name[i-1] = yytext[i];
		    yylval.name[i-1] = '\0';	
		    return QSTRING; }
\n		{ lineno++; return EOL; }
[ \t]		;

%%


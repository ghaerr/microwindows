/* Example how to use the file open dialog box
 * 
 * Georg Potthast 2018
 */
#include <windows.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#define OFDEBUG 0

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  char myfilename[PATH_MAX];
  char myfile[PATH_MAX];
  sprintf(myfilename,"initial filename");
  
  //setup filter buffer
  int totallen;
  char filter[256]; 
  /* A buffer containing pairs of null-terminated filter strings. The last string in the buffer must 
   * be terminated by two NULL characters.
   * The first string in each pair is a display string that describes the filter (for example, "Text Files"), 
   * and the second string specifies the filter pattern (for example, ".TXT"). */
  sprintf(filter,"%s:%s:%s:%s:%s:%s:%s:%s:","All files",".*","Program files",".c","Text files",".txt","Header files",".h");
  totallen=strlen(filter)+1; //including two zero bytes
  filter[strlen(filter)+2]='\0'; //add second zero after string
  //replace ":" with Null in filter
  const char s[2] = ":";
  char *token;
  token = strtok(filter, s); /* get the first token */
  while( token != NULL ) { /* walk through other tokens */
      if (OFDEBUG) printf( "%s,", token );   
      token = strtok(NULL, s);
   }
   if (OFDEBUG) printf("\n");
  int i;
  //replace non-printable chars (NULL here) with exclamation mark
  if (OFDEBUG) for (i=0;i<totallen;i++) printf("%c",isprint(((unsigned char*)filter)[i])?((unsigned char*)filter)[i]:'!'); //filter[i]);
  if (OFDEBUG) printf("\n");
  //done with filter buffer
  
  OPENFILENAME mdData;
  memset(&mdData,0,sizeof(mdData));
  
  mdData.lpstrFilter=filter;
  mdData.lpstrTitle="Select file to open";
  
  mdData.lpstrFileTitle=myfilename; 
  mdData.nMaxFileTitle=PATH_MAX; //buffer size
   
  mdData.lpstrFile=myfile;
  mdData.nMaxFile=PATH_MAX; //buffer size

  MwInitializeDialogs ( hInstance );
  GetOpenFileName (&mdData);
  //GetSaveFileName (&mdData);
  printf("\nGot the filename:%s<\n",mdData.lpstrFileTitle);
  printf("Got the path including file name:%s<\n",mdData.lpstrFile);
  return 0;
}
 

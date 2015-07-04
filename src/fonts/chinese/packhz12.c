/*pack12hz.c for HAVE_BIG5_SUPPORT*/
/*convert fonts/chinese/hzx12 to fonts/chinese/big5font.c*/
#include <stdio.h>

#define SIZEHZX12 324048

char head[]=
{
"/* big5 12x12 font\n" 
" * by jauming.tseng\n"
"   genrated by pack12hz < hzx12 > /root/microwin.big5/src/fonts/big5font.c\n"
"   since hzx12 is 16x12, so low 4 bits of each even byte is 0; in follow data block,\n"
"   two even bytes compressed into one byte, so 4 bytes stored into 3 byte;\n"
"*/\n"
"\n"
"unsigned char JMT_BIG5_12X12_FONT_BITMAP[] = {\n"
};

char tail[]="};\n";

void fprintbit(FILE *fp,char *s)
{
 int i,j;
 char *b=s,t;

 for (i=0; i<2; i++)
 {
  t=*b;
  for(j=0; j<8; j++)
  {
   if (t & 0x80)
    fprintf(fp,"*");
   else
    fprintf(fp,".");
   t<<=1;
  }
  b++;
 }
}

int main(int argc, char** argv)
{
 FILE *fp,*fp2;
 char *buf,*s;
 unsigned char p3[3];
 int i;

 printf("pack12hz v0.1 2k0816 by kevin@gv.com.tw\n");

 printf("loading hzx12...");
 fp=fopen("hzx12","rb");
 if (!fp)
 {
  printf("error open hzx12\n");
  return 1;
 }
 buf=(char*)malloc(SIZEHZX12);
      if (!buf)
 {
  printf("error malloc(%d)\n",SIZEHZX12);
  return 2;
 }
 fread(buf,1,SIZEHZX12,fp);
 fclose(fp);
 printf("ok\n");

 printf("hzx12 -> big5font.c ...");
 fflush(stdout);
 fp2=fopen("big5font.c","wt+");
 fprintf(fp2,head);

 s=buf;
 for (i = 0; i < (SIZEHZX12/4); i++)
 {
  p3[0]=*s;
  p3[1]=(*(s+1)&0xf0)+((*(s+3)&0xf0)>>4);
  p3[2]=*(s+2);

  if (!(i%(24/4)) && i)
   fprintf(fp2,"\n");

  fprintf(fp2,"\t0x%02x, 0x%02x,\t/* ",p3[0],p3[1]);
  fprintbit(fp2,s);
  fprintf(fp2," */\n");
  
  fprintf(fp2,"\t0x%02x,      \t/* ",p3[2]);
  fprintbit(fp2,s+2);
  fprintf(fp2," */\n");

  s+=4;
 }

 fprintf(fp2,tail);
 fclose(fp2);
 printf("ok\n");

 return 0;
}

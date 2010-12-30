
// these 4 inlines are here to get the code clearer (it was unreadable due do the parenthesis)
static inline T8p sd(T8p d1, int s1, T8p d2, int s2) 
// sd stands for shift data
{ return((d1<<s1)|(d2>>s2)); }

static inline T8p sdm(T8p d1, int s1, T8p d2, int s2, T8p m) 
// sdm stands for shift data mask
{ return(((d1<<s1)|(d2>>s2))&m); }

static inline T8p sdml(T8p d, int s, T8p m) 
// sdml stands for shift data left mask
{ return(m&(d<<s)); }

static inline T8p sdmr(T8p d, int s, T8p m) 
// sdmr stands for shift data right mask
{ return(m&(d>>s)); }

// for all the functions bellow
// the shift state is after in the function name.
// w? stands for the number of full words-1 to copy.
// as all these functions are based on the same principles,
// the comments are located in the most complex of them: Blt_Shift_Positif

// s and d are the source and destination pointers.
// ss and ds are the skip source and skip destination values (ie: the number to add to source and
// destination pointer at the end of each line to go to the next line).
// S, M1 and M2 are the shift, start and end masks.
// w is the number of full words to write per line
// h is the number of line to work on.

static inline void Blt_Shift_Positif_w0(P8p s, P8p d, int ss, int ds, int S, T8p M1, T8p M2, int w, int h)
{
  M1= M2&~M1;
  do
  { 
    *d= (*d&~M1) | sdml(*s, S, M1);
    d+= ds; s+= ss;
  } while (--h!=0);
  return;
}

static inline void Blt_Shift_Positif_w1(P8p s, P8p d, int ss, int ds, int S, T8p M1, T8p M2, int w, int h)
{
  int S2= 32-S;
  M1= ~M1;
  do
  {
    T8p d1= *s++;
    *d++= (*d&~M1)|sdml(d1, S, M1);
    *d= (*d&~M2)| sdm(*s, S, d1, S2, M2);
    d+= ds; s+= ss;
  } while (--h!=0);
  return;
}

static inline void Blt_Shift_Positif_w2(P8p s, P8p d, int ss, int ds, int S, T8p M1, T8p M2, int w, int h)
{
  int S2= 32- S;
  M1= ~M1;
  do
  {
    T8p d2, d1= *s++; *d++= (*d&~M1)|sdml(d1, S, M1);
    d2= *s++; *d++= sd(d2, S, d1, S2);
    *d= (*d&~M2)|sdm(*s, S, d2, S2, M2);
    d+= ds; s+= ss;
  } while (--h!=0);
  return;
}

static inline void Blt_Shift_Positif(P8p s, P8p d, int ss, int ds, int S, T8p M1, T8p M2, int w, int h)
{
  int S2= 32- S;
  M1= ~M1;
  // for each line to copy (there is at least one)
  do
  {
    int i;
    // get the first set of pixels from the source
    T8p d1= *s++;
    // combine the shifted source pixels with the destination pixels (use of mask1)
    *d++= (*d&~M1)|sdml(d1, S, M1);
    // for every other set of pixel
    i= w;
    // directly write the shifted set of pixels to the dest (see how 2 set of pixels are shifted and combined together like a sliding window)
    do { T8p d2= *s++; *d++= sd(d2, S, d1, S2); d1= d2; } while (--i!=0);
    // combine the final shifted source pixels with the destination pixels (use of mask2)
    *d= (*d&~M2)|sdm(*s, S, d1, S2, M2);
    // skip what is left of the curent lines in sources and dest.
    d+= ds; s+= ss;
  } while (--h!=0);
  return;
}

static inline void Blt_Shift_Null_w0(P8p s, P8p d, int ss, int ds, int S, T8p M1, T8p M2, int w, int h)
{
  M1|= ~M2;
  do
  { 
    *d= (*d&M1) | (*s&~M1);
    d+= ds; s+= ss;
  } while (--h!=0);
}

static inline void Blt_Shift_Null_w1(P8p s, P8p d, int ss, int ds, int S, T8p M1, T8p M2, int w, int h)
{
  do
  {
    *d++= (*d&M1)|(*s++&~M1);
    *d= (*d&~M2)|(*s&M2);
    d+= ds; s+= ss;
  } while (--h!=0);
  return;
}

static inline void Blt_Shift_Null_w2(P8p s, P8p d, int ss, int ds, int S, T8p M1, T8p M2, int w, int h)
{
  do
  {
    *d++= (*d&M1)|(*s++&~M1);
    *d++= *s++;
    *d= (*d&~M2)|(*s&M2);
    d+= ds; s+= ss;
  } while (--h!=0);
  return;
}

static inline void Blt_Shift_Null_w3(P8p s, P8p d, int ss, int ds, int S, T8p M1, T8p M2, int w, int h)
{
  do
  {
    *d++= (*d&M1)|(*s++&~M1);
    *d++= *s++;
    *d++= *s++;
    *d= (*d&~M2)|(*s&M2);
    d+= ds; s+= ss;
  } while (--h!=0);
  return;
}

static inline void Blt_Shift_Null(P8p s, P8p d, int ss, int ds, int S, T8p M1, T8p M2, int w, int h)
{
  do
  {
    int i;
    *d++= (*d&M1)|(*s++&~M1);
    switch (w&7)
    {
      case 7: *d++= *s++; case 6: *d++= *s++; case 5: *d++= *s++; case 4: *d++= *s++;
      case 3: *d++= *s++; case 2: *d++= *s++; case 1: *d++= *s++; case 0: default:; 
    }
    for (i= w>>3; i!=0; i--)               
    {
      *d++= *s++; *d++= *s++; *d++= *s++; *d++= *s++;
      *d++= *s++; *d++= *s++; *d++= *s++; *d++= *s++; 
    }
    *d= (*d&~M2)|(*s&M2);
    d+= ds; s+= ss;
  } while (--h!=0);
  return;
}

static inline void Blt_Shift_Negatif_w0(P8p s, P8p d, int ss, int ds, int S, T8p M1, T8p M2, int w, int h)
{
  int S2= 32+ S;
  S= -S;
  M1= M2&~M1;
  do
  { 
    *d= (*d&~M1) | sdm(*(s+1), S2, *s, S, M1);
    d+= ds; s+= ss;
  } while (--h!=0);
  return;
}

static inline void Blt_Shift_Negatif_w1(P8p s, P8p d, int ss, int ds, int S, T8p M1, T8p M2, int w, int h)
{ 
  int S2= 32+ S;
  M1= ~M1;
  S= -S;
  do
  {
    T8p d1= *s++; T8p d2= *s;
    *d++= (*d&~M1)|sdm(d2, S2, d1, S, M1);
    *d= (*d&~M2)|sdm(*(s+1), S2, *s, S, M2);
    d+= ds; s+= ss;
  } while (--h!=0);
  return;
}

static inline void Blt_Shift_Negatif_w2(P8p s, P8p d, int ss, int ds, int S, T8p M1, T8p M2, int w, int h)
{
  int S2= 32+S;
  M1= ~M1;
  ss--; 
  S= -S;
  do
  {
    T8p d1= *s++; T8p d2= *s++;
    *d++= (*d&~M1)|sdm(d2, S2, d1, S, M1);
    d1= *s++; *d++= sd(d1, S2, d2, S);
    *d= (*d&~M2)|sdm(*s, S2, d1, S, M2);
    d+= ds; s+= ss;
  } while (--h!=0);
  return;
}

static inline void Blt_Shift_Negatif(P8p s, P8p d, int ss, int ds, int S, T8p M1, T8p M2, int w, int h)
{
  int S2= 32+S;
  M1= ~M1;
  ss--; 
  S= -S;
  do
  {
    int i;
    T8p d1= *s++; T8p d2= *s++;
    *d++= (*d&~M1)|sdm(d2, S2, d1, S, M1);
    i= w;
    do { d1= d2; d2= *s++; *d++= sd(d2, S2, d1, S); } while (--i!=0);
    *d= (*d&~M2)|sdm(*s, S2, d2, S, M2);
    d+= ds; s+= ss;
  } while (--h!=0);
  return;
}

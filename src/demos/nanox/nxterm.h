/* These contained 'pt:re=\\EC' entries.  I deleted them from MiNT and linux
 * because neither of my (linux) termcap nor terminfo manual pages listed
 * them.
 *	++eero
 */
#ifdef __MINT__

static char termcap_string[1024] =
"TERMCAP=wterm|WTerm terminal:al=\\EL:am:bc=\\ED:bl=^G:bs:cd=\\EJ:\
ce=\\EK:cl=\\EE:cm=\\EY%+ %+ :cr=^M:dl=\\EM:do=\\EB:ho=\\EH:is=\\Ev\\Eq:\
l0=F10:le=\\ED:ms:nd=\\EC:rc=\\Ek:rs=\\Ev\\Eq\\EE:sc=\\Ej:sr=\\EI:\
ti=\\Ev\\Ee\\EG:up=\\EA:ve=\\Ee:vi=\\Ef:so=\\Ep:se=\\Eq:mb=\\Ei:md=\\Eg:\
mr=\\Ep:me=\\EG:te=\\EG:us=\\Ei:ue=\\EG:\
kb=^H:kl=\\ED:kr=\\EC:ku=\\EA:kd=\\EB:kI=\\EI:kh=\\EE:kP=\\Ea:kN=\\Eb:k0=\\EY:\
k1=\\EP:k2=\\EQ:k3=\\ER:k4=\\ES:k5=\\ET:k6=\\EU:k7=\\EV:k8=\\EW:k9=\\EX:\
s0=\\Ey:s1=\\Ep:s2=\\Eq:s3=\\Er:s4=\\Es:s5=\\Et:s6=\\Eu:s7=\\Ev:s8=\\Ew:\
s9=\\Ex:";

#elif defined(linux) || defined(__FreeBSD__)

static char termcap_string[1024] =
"TERMCAP=wterm|WTerm terminal:al=\\EL:am:bc=\\ED:bl=^G:bs:cd=\\EJ:\
ce=\\EK:cl=\\EE:cm=\\EY%+ %+ :cr=^M:dl=\\EM:do=\\EB:ho=\\EH:is=\\Ev\\Eq:\
l0=F10:le=\\ED:ms:nd=\\EC:rc=\\Ek:rs=\\Ev\\Eq\\EE:sc=\\Ej:sr=\\EI:\
ti=\\Ev\\Ee\\EG:up=\\EA:ve=\\Ee:vi=\\Ef:so=\\Ep:se=\\Eq:mb=\\Ei:md=\\Eg:\
mr=\\Ep:me=\\EG:te=\\EG:us=\\Ei:ue=\\EG:\
kb=^H:kl=\\E[D:kr=\\E[C:ku=\\E[A:kd=\\E[B:kI=\\EI:kh=\\EE:kP=\\Ea:kN=\\Eb:\
k0=\\EY:k1=\\EP:k2=\\EQ:k3=\\ER:k4=\\ES:k5=\\ET:k6=\\EU:k7=\\EV:k8=\\EW:\
k9=\\EX:s0=\\Ey:s1=\\Ep:s2=\\Eq:s3=\\Er:s4=\\Es:s5=\\Et:s6=\\Eu:s7=\\Ev:\
s8=\\Ew:s9=\\Ex:";

#elif defined(sun)

  /* only very basic cursor keys so far... */

static char termcap_string[1024] =
"TERMCAP=wterm|WTerm terminal:al=\\EL:am:bc=\\ED:bl=^G:bs:cd=\\EJ:\
ce=\\EK:cl=\\EE:cm=\\EY%+ %+ :cr=^M:dl=\\EM:do=\\EB:ho=\\EH:is=\\Ev\\Eq:\
l0=F10:le=\\ED:ms:nd=\\EC:pt:re=\\EC:rc=\\Ek:rs=\\Ev\\Eq\\EE:sc=\\Ej:sr=\\EI:\
ti=\\Ev\\Ee\\EG:up=\\EA:ve=\\Ee:vi=\\Ef:so=\\Ep:se=\\Eq:mb=\\Ei:md=\\Eg:\
mr=\\Ep:me=\\EG:te=\\EG:us=\\Ei:ue=\\EG:\
kb=^H:kl=\\E[D:kr=\\E[C:ku=\\E[A:kd=\\E[B:\
k1=\\E[224z:k2=\\E[225z:k3=\\E[226z:k4=\\E[227z:k5=\\E[228z:\
k6=\\E[229z:k7=\\E[230z:k8=\\E[231z:k9=\\E[232z:k0=\\E[233z:\
kN=\\E[222z:kP=\\E[216z:kh=\\E[214z:kH=\\E220z:";

#elif defined(__NetBSD__)

static char termcap_string[1024] =
"TERMCAP=wterm|WTerm terminal:al=\\EL:am:bc=\\ED:bl=^G:bs:cd=\\EJ:\
ce=\\EK:cl=\\EE:cm=\\EY%+ %+ :cr=^M:dl=\\EM:do=\\EB:ho=\\EH:is=\\Ev\\Eq:\
l0=F10:le=\\ED:ms:nd=\\EC:pt:re=\\EC:rc=\\Ek:rs=\\Ev\\Eq\\EE:sc=\\Ej:sr=\\EI:\
ti=\\Ev\\Ee\\EG:up=\\EA:ve=\\Ee:vi=\\Ef:so=\\Ep:se=\\Eq:mb=\\Ei:md=\\Eg:\
mr=\\Ep:me=\\EG:te=\\EG:us=\\Ei:ue=\\EG:\
kb=^H:kl=\\E[D:kr=\\E[C:ku=\\E[A:kd=\\E[B:\
k1=\\E[224z:k2=\\E[225z:k3=\\E[226z:k4=\\E[227z:k5=\\E[228z:\
k6=\\E[229z:k7=\\E[230z:k8=\\E[231z:k9=\\E[232z:k0=\\E[233z:\
kN=\\E[222z:kP=\\E[216z:kh=\\E[214z:kH=\\E220z:";

#else

#error oops, a new operating system?

#endif


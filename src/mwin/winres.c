/*
 * Copyright (c) 2010 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Resource functions
 *
 * Copyright (C) 2003 - Gabriele Brugnoni
 * DVE Prog. El. - Varese, Italy
 *
 * gabrielebrugnoni@dveprojects.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MWINCLUDECOLORS
#include "windows.h"		/* windef.h, winuser.h */
#include "winres.h"
#include "windlg.h"
#include "device.h"
#include "../drivers/genmem.h"

#define MAX_MRU_RESOURCES	32

//  This is a pointer to a list of resources.
static HRSRC mruResources = NULL;
static int mruResCount = 0;

static PMWIMAGEHDR resDecodeBitmap(unsigned char *buffer, int size);

static void
mwAddResource(HRSRC hRes)
{
	hRes->next = mruResources;
	mruResources = hRes;
	mruResCount++;

	//  if we have too many resources, try to free unlocked res.
	while (mruResCount > MAX_MRU_RESOURCES) {
		HRSRC obj = hRes->next;
		HRSRC prev = hRes;
		int ndel = 0;
		while (obj) {
			HRSRC cobj = obj;
			prev = obj;
			obj = obj->next;
			if (cobj->cLock <= 0) {
				if (prev != NULL)
					prev->next = cobj->next;
				else
					mruResources = NULL;
				if (cobj->pData)
					free(cobj->pData);
				free(cobj);
				mruResCount--;
				ndel++;
			}
		}
		if (!ndel)
			break;
	}
}


//  Compare resource types
static int
mwResCompare(LPCTSTR res1, LPCTSTR res2)
{
	if ((HIWORD(res1) == 0xFFFF) || (HIWORD(res2) == 0xFFFF))	// OK: Not pointer. Checks high word of resource DWORD.
		return (res1 != res2);

	// FIXME: resource string names not handled properly here
	return strcmp(res1, res2);
}


static HRSRC
mwFindMruResource(LPCTSTR resName, LPCTSTR resType)
{
	HRSRC obj = mruResources;

	while (obj != NULL) {
		if (!mwResCompare(obj->name, resName) &&
		    !mwResCompare(obj->type, resType))
			break;
		obj = obj->next;
	}

	return obj;
}

/*
 *  Create application instance
 */
HINSTANCE
mwCreateInstance(int argc, char *argv[])
{
	char *p;
	int i, tot;
	PMWAPPINSTANCE mwAppInst = (PMWAPPINSTANCE) malloc(sizeof(MWAPPINSTANCE));
	char *defaultargv[1] = { "mw.exe" };

	if (!argv)
		argv = defaultargv;

	if (mwAppInst == NULL)
		return NULL;

	for (i = 1, tot = 1; i < argc; i++)
		tot += 1 + strlen(argv[i]);
	mwAppInst->szCmdLine = (LPCSTR) malloc(tot);
	if (mwAppInst->szCmdLine == NULL) {
		free(mwAppInst);
		return NULL;
	}

	strcpy((char *) mwAppInst->szCmdLine, "");
	p = (char *) mwAppInst->szCmdLine;
	for (i = 1; i < argc; i++) {
		if (i > 1)
			*p++ = ' ';
		strcpy(p, argv[i]);
		p += strlen(argv[i]);
	}

	mwAppInst->szExecCommand = argv[0];
	mwAppInst->argc = argc;
	mwAppInst->argv = argv;
	mwAppInst->szResFilename = (LPCSTR) malloc(5 + strlen(argv[0]));
	if (mwAppInst->szResFilename == NULL) {
		free(mwAppInst);
		return NULL;
	}

	strcpy((char *) mwAppInst->szResFilename, argv[0]);
	p = strrchr(mwAppInst->szResFilename, '.');
	if ((p != NULL) && (p[1] != '/') && (p[1] != '\\'))
		strcpy(p, ".res");
	else
		strcat((char *) mwAppInst->szResFilename, ".res");

	mwAppInst->fResources = fopen(mwAppInst->szResFilename, "rb");
	return (HINSTANCE) mwAppInst;
}


void
mwFreeInstance(HINSTANCE hInst)
{
	if (((PMWAPPINSTANCE) hInst)->fResources) fclose (((PMWAPPINSTANCE) hInst)->fResources);
	free((void *) ((PMWAPPINSTANCE) hInst)->szResFilename);
	free((void *) ((PMWAPPINSTANCE) hInst));
}


/*
 *  IMPORTANT NOTE:
 *  When reading from .res file we read one field a time.
 *  This prevent alignment problem that some architecture,
 *  like ARM, may present when reading entire structure
 *  in one shot.
 */

/*
 *  File access functions
 */
static WORD
resReadWord(FILE * f, BOOL * pEof)
{
	WORD w;

	if (!fread(&w, 2, 1, f))
		*pEof = TRUE;
	return w;
}

static DWORD
resReadDWord(FILE * f, BOOL * pEof)
{
	DWORD dw;

	if (!fread(&dw, 4, 1, f))
		*pEof = TRUE;
	return dw;
}

#if LATER
static BYTE
resReadByte(FILE * f, BOOL * pEof)
{
	int ch = fgetc(f);
	if (ch == EOF)
		*pEof = TRUE;
	return (BYTE) ch;
}

static void
resReadData(FILE * f, void *buffer, int len, BOOL * pEof)
{
	if ((fread(buffer, 1, len, f) < len))
		*pEof = TRUE;
}

/*
 *  Allocate a text buffer and read the string from file.
 */
static LPTSTR
resReadText(FILE * f, BOOL * pEof)
{
	WORD w;
	LPSTR txt;
	int i;
	int count;
	long pos = ftell(f);

	count = 1;
	do {
		if (!fread(&w, 2, 1, f))
			*pEof = TRUE;
		if (w == 0)
			break;
		//  check special cases where string init with a FFFF
		if ((count == 1) && (w == 0xFFFF)) {
			count = 2;
			break;
		}
		count++;
	} while (!*pEof);

	if (*pEof)
		return NULL;
	fseek(f, pos, SEEK_SET);
	txt = (LPTSTR) malloc(sizeof(TCHAR) * count);
	if (txt == NULL)
		return NULL;

	for (i = 0; i < count; i++)
		txt[i] = (TCHAR) resReadWord(f, pEof);
	if (*pEof) {
		free(txt);
		return NULL;
	}

	return txt;
}
#endif /* LATER*/

/*
 *  Check if type (numeric or text) are the same
 */
static BOOL
mwIsSameType(FILE * f, LPCTSTR id, BOOL * pEof)
{
	WORD w;
	int i, n;

	if (id == NULL) {
		w = resReadWord(f, pEof);
		w = resReadWord(f, pEof);
		return !(*pEof);
	}
	//  Resource may be specified in text or integer
	if (HIWORD(id) == 0xFFFF) {
		w = resReadWord(f, pEof);
		if (w != 0xFFFF)
			return FALSE;
		w = resReadWord(f, pEof);
		if (*pEof)
			return FALSE;
		if (w == LOWORD((DWORD) id))
			return TRUE;
	} else {
		LPCTSTR p = id;
		for (i = 0, n = strlen(p) + 1; (i < n) && !*pEof; i++) {
			w = resReadWord(f, pEof);
			if (w != (unsigned) *p)
				break;
			p++;
		}
		if (!*pEof && (i >= n))
			return TRUE;
	}

	return FALSE;
}

/*
 *  Search for a resource.
 *  If found, the file is positioned at the beginning of the resource.
 */
FILE *
mwFindResource(HINSTANCE hInst, LPCTSTR resType, LPCTSTR resName, PMWRESOURCEHEADER pResHead)
{
	FILE *f;
	PMWAPPINSTANCE pInst = (PMWAPPINSTANCE) hInst;
	BOOL bEof;
	BOOL bType, bName;
	long pos;

	bEof = FALSE;

	do {
		f = pInst->fResources;
		if (f == NULL) {
			EPRINTF("Error opening resource file: %s\n", pInst->szResFilename);
			break;
		}

		fseek(f, 0, SEEK_SET);

		while (!feof(f)) {
			pos = ftell(f);
			pResHead->DataSize = resReadDWord(f, &bEof);
			pResHead->HeaderSize = resReadDWord(f, &bEof);
			if (bEof)
				break;

			bType = mwIsSameType(f, resType, &bEof);
			bName = mwIsSameType(f, resName, &bEof);
			if (bType && bName) {
				pResHead->DataVersion = resReadDWord(f, &bEof);
				pResHead->MemoryFlags = resReadWord(f, &bEof);
				pResHead->LanguageId = resReadWord(f, &bEof);
				pResHead->Version = resReadDWord(f, &bEof);
				pResHead->Characteristics = resReadDWord(f, &bEof);
				if (bEof)
					break;
				fseek(f, pos + pResHead->HeaderSize, SEEK_SET);
				return f;
			}

			pos += pResHead->HeaderSize + pResHead->DataSize;
			//  align to dword
			while ((pos & 3) != 0)
				pos++;
			if (fseek(f, pos, SEEK_SET))
				break;
		}
	} while (0);

	return NULL;
}


/*
 *  Allocate a text string from a template.
 */
static LPTSTR
resAllocText(WORD ** pw)
{
	WORD *orgpw = *pw;
	WORD *ppw = *pw;
	LPSTR txt;
	int i;
	int count;

	count = 0;
	while (*ppw != 0) {
		//  check special cases where string init with a FFFF
		if ((count == 0) && (*ppw == 0xFFFF)) {
			count = 1;
			ppw++;
			break;
		}
		ppw++;
		count++;
	}

	*pw = ppw + 1;
	ppw = orgpw;
	count++;
	txt = (LPTSTR) malloc(sizeof(TCHAR) * count);
	if (txt == NULL)
		return NULL;

	for (i = 0; i < count; i++)
		txt[i] = (TCHAR) * ppw++;
	return txt;
}


/*
 *  get dynamic extra information from a dlg item template
 */
PMWDLGITEMTEMPLATE
resGetDlgItemTemplExtra(PMWDLGITEMTEMPLATE pItem, PMWDLGITEMTEMPLEXTRA pItemExtra)
{
	LPWORD pw = (LPWORD) pItem->extraData;

	pItemExtra->szClassName = resAllocText(&pw);
	pItemExtra->szCaption = resAllocText(&pw);
	if ((*pw != 0xFFFF) && (*pw != 0x0000))
		pItemExtra->lpData = (LPWORD) pw;
	else
		pItemExtra->lpData = NULL;

	pw += 1 + ((*pw) >> 1);
	//  dword align
	if ((((UINT_PTR)pw) & 2) != 0)
		pw++;
	return (PMWDLGITEMTEMPLATE) pw;
}

/*
 *  Free allocated resources of MWDLGTEMPLATE
 */
void
resDiscardDlgItemTemplate(PMWDLGITEMTEMPLEXTRA pItemExtra)
{
	if (pItemExtra->szClassName)
		free(pItemExtra->szClassName);
	if (pItemExtra->szCaption)
		free(pItemExtra->szCaption);
}

/*
 *  get dynamic extra information from a dlg template
 */
void
resGetDlgTemplExtra(PMWDLGTEMPLATE pDlg, PMWDLGTEMPLEXTRA pDlgExtra)
{
	PMWDLGITEMTEMPLATE pItem;
	LPWORD pw = (LPWORD) pDlg->extraData;
	int i;

	pDlgExtra->szIdMenu = resAllocText(&pw);
	pDlgExtra->szClassName = resAllocText(&pw);
	pDlgExtra->szDlgName = resAllocText(&pw);
	if ((pDlg->style & DS_SETFONT) != 0) {
		pDlgExtra->fontSize = *pw++;
		pDlgExtra->szFontName = resAllocText(&pw);
	} else {
		pDlgExtra->fontSize = 0;
		pDlgExtra->szFontName = NULL;
	}
	pDlgExtra->pItems = malloc(sizeof(PMWDLGITEMTEMPLATE) * pDlg->cdit);
	pDlgExtra->pItemsExtra =
		malloc(sizeof(MWDLGITEMTEMPLEXTRA) * pDlg->cdit);
	if ((pDlgExtra->pItems == NULL) || (pDlgExtra->pItemsExtra == NULL))
		return;

	pDlgExtra->nItems = pDlg->cdit;
	pItem = resFirstDlgItem(pDlg);
	for (i = 0; i < pDlg->cdit; i++) {
		pDlgExtra->pItems[i] = pItem;
		pItem = resGetDlgItemTemplExtra(pItem, pDlgExtra->pItemsExtra + i);
	}
}

/*
 *  Free allocated resources of MWDLGTEMPLATE
 */
void
resDiscardDlgTemplExtra(PMWDLGTEMPLEXTRA pDlgExtra)
{
	int i;

	for (i = 0; i < pDlgExtra->nItems; i++)
		resDiscardDlgItemTemplate(pDlgExtra->pItemsExtra + i);

	if (pDlgExtra->szIdMenu)
		free(pDlgExtra->szIdMenu);
	if (pDlgExtra->szClassName)
		free(pDlgExtra->szClassName);
	if (pDlgExtra->szDlgName)
		free(pDlgExtra->szDlgName);
	if (pDlgExtra->szFontName)
		free(pDlgExtra->szFontName);
	if (pDlgExtra->pItems)
		free(pDlgExtra->pItems);
	if (pDlgExtra->pItemsExtra)
		free(pDlgExtra->pItemsExtra);
}


#define RES_SKIP_WSTRING(w)		{\
	if( *(w) == 0xFFFF ) (w) += 2; \
	else while ( *(w)++ != 0 );\
	}


/*
 *  Return pointer to first dlgitem in dlg template
 */
PMWDLGITEMTEMPLATE
resFirstDlgItem(PMWDLGTEMPLATE pDlg)
{
	LPWORD pw = (LPWORD) pDlg->extraData;

	//  Skip idMenu, classname, dlgName
	RES_SKIP_WSTRING(pw);
	RES_SKIP_WSTRING(pw);
	RES_SKIP_WSTRING(pw);

	//  check font
	if ((pDlg->style & DS_SETFONT) != 0) {
		pw++;		// skip font size
		RES_SKIP_WSTRING(pw);	// skip font name
	}
	//  dword align
	if ((((UINT_PTR)pw) & 2) != 0)
		pw++;
	return (PMWDLGITEMTEMPLATE) pw;
}

/*
 *  Move pointer to next dlgitem in template
 */
PMWDLGITEMTEMPLATE
resNextDlgItem(PMWDLGITEMTEMPLATE pItem)
{
	LPWORD pw = (LPWORD) pItem->extraData;

	//  Skip classname, caption
	RES_SKIP_WSTRING(pw);
	RES_SKIP_WSTRING(pw);
	pw += 1 + ((*pw) >> 1);
	//  dword align
	if ((((UINT_PTR)pw) & 2) != 0)
		pw++;
	return (PMWDLGITEMTEMPLATE) pw;
}

/*
 * In-memory dialog template creating routines
 */
BYTE *
resDialogTemplate(BYTE *dest, LPCSTR caption, DWORD style, DWORD dwExtendedStyle,
	int x, int y, int cx, int cy, LPSTR menu, LPSTR classname, int cdit)
{
	DLGTEMPLATE *dialog = (DLGTEMPLATE *)dest;
	WORD *extra;

	dialog->style = style;
	dialog->dwExtendedStyle = dwExtendedStyle;
	dialog->x = (short)x;
	dialog->y = (short)y;
	dialog->cx = (short)cx;
	dialog->cy = (short)cy;
	dialog->cdit = (WORD)cdit;

	extra = (WORD *)(((BYTE *)dialog) + FIXSZ_MWDLGTEMPLATE);
	*extra++ = (WORD)menu;		// OK: No string menus yet, menu id always passed as WORD.
	*extra++ = (WORD)classname;	// OK: No string classes yet, class id always passed as WORD.
	if (caption)
		while (*caption)
			*extra++ = (WORD)*caption++;
	*extra++ = 0;			// 0 terminate caption

	dest = (BYTE *)extra;
	dest = (BYTE *)(((UINT_PTR)dest + 3) & ~3);		// DWORD align
	return dest;
}

BYTE *
resDialogItemTemplate(BYTE *dest, DWORD style, DWORD dwExtendedStyle, int id,
		int x, int y, int cx, int cy, int classname, LPCSTR data)
{
	DLGITEMTEMPLATE *item = (DLGITEMTEMPLATE *)dest;
	WORD *extra;

	item->style = style;
	item->dwExtendedStyle = dwExtendedStyle;
	item->id = (WORD)id;
	item->x = (short)x;
	item->y = (short)y;
	item->cx = (short)cx;
	item->cy = (short)cy;

	extra = (WORD *)(((BYTE *)item) + FIXSZ_MWDLGITEMTEMPLATE);
	*extra++ = 0xFFFF;
	*extra++ = (WORD)classname;	// no string classes yet
	if (data)
		while (*data)
			*extra++ = (WORD)*data++;
	*extra++ = 0;			// 0 terminate data
	*extra++ = 0;			// lpData

	dest = (BYTE *)extra;
	dest = (BYTE *)(((UINT_PTR)dest + 3) & ~3);		// DWORD align
	return dest;
}

/***********************  EXPORTED FUNCTIONS  *******************************/

/*
 *  Find a resource
 */
HRSRC WINAPI
FindResource(HMODULE hModule, LPCTSTR resName, LPCTSTR resType)
{
	HRSRC hRes;

	hRes = mwFindMruResource(resName, resType);
	if (hRes != NULL)
		return hRes;

	hRes = (HRSRC) malloc(sizeof(MWRSRC));
	if (hRes == NULL)
		return NULL;

	hRes->f = mwFindResource(hModule, resType, resName, &hRes->head);
	if (hRes->f == NULL) {
		free(hRes);
		return NULL;
	}

	hRes->fPos = ftell(hRes->f);
	hRes->type = resType;
	hRes->name = resName;
	hRes->pData = NULL;
	hRes->cLock = 0;

	//  Add resource to MRU List
	mwAddResource(hRes);
	return hRes;
}


/*
 *  Return the size of a resource
 */
DWORD
SizeofResource(HMODULE hModule, HRSRC hResInfo)
{
	return hResInfo->head.DataSize;
}


/*
 * Load a resource.
 *
 * The data will be pointed by hRes->pData.
 */
HGLOBAL WINAPI
LoadResource(HMODULE hModule, HRSRC hRes)
{
	if (hRes == NULL)
		return NULL;
	hRes->pData = malloc(hRes->head.DataSize);
	if (hRes->pData == NULL)
		return NULL;

//printf("LoadResource size %d\n", hRes->head.DataSize);
	fseek(hRes->f, hRes->fPos, SEEK_SET);
	if (fread(hRes->pData, 1, hRes->head.DataSize, hRes->f) < hRes->head.DataSize) {
		free(hRes->pData);
		return NULL;
	}
	return hRes;
}


LPVOID WINAPI
LockResource(HGLOBAL hObj)
{
	((HRSRC) hObj)->cLock++;
	return ((HRSRC) hObj)->pData;
}


int WINAPI
UnlockResource(HGLOBAL hObj)
{
	if (((HRSRC) hObj)->cLock <= 0)
		return 0;
	((HRSRC) hObj)->cLock--;
	return ((HRSRC) hObj)->cLock;
}


BOOL WINAPI
FreeResource(HGLOBAL hObj)
{
	if (((HRSRC) hObj)->pData == NULL)
		return FALSE;
	if (((HRSRC) hObj)->cLock > 0)
		return FALSE;
	free(((HRSRC) hObj)->pData);
	((HRSRC) hObj)->pData = NULL;
	return TRUE;
}

/*
 *  Loads a string from resource table.
 */
int WINAPI
LoadString(HINSTANCE hInstance, UINT uid, LPTSTR lpBuffer, int nMaxBuff)
{
	MWRESOURCEHEADER resHead;
	int blkId = (uid >> 4) + 1;
	int retV = 0;
	int i, x, ln;
	FILE *f;
	BOOL bEof;
	WORD w;
	LPTSTR ptr;

	if ((lpBuffer != NULL) && (nMaxBuff > 0))
		*lpBuffer = 0;
	else
		return 0;

	nMaxBuff--;
	ptr = lpBuffer;

	f = mwFindResource(hInstance, RT_STRING, MAKEINTRESOURCE(blkId), &resHead);
	if (f) {
		i = (blkId - 1) * 16;
		bEof = FALSE;
		while (!bEof && ((UINT) i <= uid)) {
			ln = (int) resReadWord(f, &bEof);
			for (x = 0; x < ln; x++) {
				w = resReadWord(f, &bEof);
				if (((UINT) i == uid) && (x < nMaxBuff)) {
					*ptr++ = (TCHAR) w;
					retV++;
				}
			}
			i++;
		}
		*ptr++ = (TCHAR) 0;
	}

	return retV;
}

/*
 *  Load a bitmap from resource file.
 */
PMWIMAGEHDR
resLoadBitmap(HINSTANCE hInst, LPCTSTR resName)
{
	HGLOBAL hResBmp;
	HRSRC hRes;
	PMWIMAGEHDR pimage = NULL;
	unsigned char *buffer;
	int size;

	hRes = FindResource(hInst, resName, RT_BITMAP);
	if (!hRes)
		return NULL;

	size = SizeofResource(hInst, hRes);
	hResBmp = LoadResource(hInst, hRes);
	buffer = LockResource(hResBmp);
	if (buffer) {
		pimage = resDecodeBitmap(buffer, size);
		UnlockResource(hResBmp);
	}
	FreeResource(hResBmp);
	return pimage;
}

static PMWIMAGEHDR
resDecodeBitmap(unsigned char *buffer, int size)
{
#if HAVE_BMP_SUPPORT
	PSD			pmd;
	buffer_t stream;

	GdImageBufferInit(&stream, buffer, size);
	pmd = GdDecodeBMP(&stream, FALSE);	/* don't read file hdr*/

	return (PMWIMAGEHDR)pmd;		//FIXME uses shared header for now
#else
	return NULL;
#endif
}

/*
 *  Free memory allocated with resLoadBitmap.
 */
void
resFreeBitmap(PMWIMAGEHDR pimage)
{
	GdFreePixmap((PSD)pimage);		// FIXME uses shared header
}

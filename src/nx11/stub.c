#include <stdio.h>

#if DEBUG | 0
int	GdError(const char *format, ...);
#define DPRINTF		GdError			/* debug output*/
#else
#define DPRINTF(str, ...)			/* no debug output*/
#endif

/* stubbed out calls, need implementations*/
int XScreenResourceString() { DPRINTF("XScreenResourceString called\n"); return 0; } 
int XWMGeometry() { DPRINTF("XWMGeometry called\n"); return 0; } 
int XGetIconSizes() { DPRINTF("XGetIconSizes called\n"); return 0; } 
int XQueryBestCursor() { DPRINTF("XQueryBestCursor called\n"); return 0; } 
int XSetState() { DPRINTF("XSetState called\n"); return 0; } 
//int XResourceManagerString() { DPRINTF("XResourceManagerString called\n"); return 0; } 
int XQueryKeymap() { DPRINTF("XQueryKeymap called\n"); return 0; } 
int XGetDefault(void *d, char *program, char *option) { DPRINTF("XGetDefault %s %s\n", program, option); return 0; } 
int XRecolorCursor() { DPRINTF("XRecolorCursor called\n"); return 0; } 
int XListExtensions() { DPRINTF("XListExtensions called\n"); return 0; } 
 	
/* required for FLTK 1.3.3 */
int XGetPointerControl() { DPRINTF("XGetPointerControl called\n"); return 0; }
int XChangePointerControl() { DPRINTF("XChangePointerControl called\n"); return 0; } 
int XDefaultGC() { DPRINTF("XDefaultGC called\n"); return 0; }
int XkbGetIndicatorState() { DPRINTF("XkbGetIndicatorState called\n"); return 0; }

/* required for FLTK 1.3.4 (also must #define USE_XRANDR 0 in FLTK src*/
int XFixesQueryExtension() { DPRINTF("XFixesQueryExtension called\n"); return 0; }
int XFixesSelectSelectionInput() { DPRINTF("XFixesSelectSelectionInput called\n"); return 0; }
/* following in IM.c but here until it compiles*/
int XOpenIM(void* d, int db, char* rn, char* rc) { DPRINTF("XOpenIM called\n"); return 0; }
int XCloseIM(void* d) { DPRINTF("XCloseIM called\n"); return 0; }
int XCreateIC(int i, int j) { DPRINTF("XCreateIC called\n"); return 0; }

/* required for gtk+ 1.2.7*/
int XAutoRepeatOn() { DPRINTF("XAutoRepeatOn called\n"); return 0; } 
int XAutoRepeatOff() { DPRINTF("XAutoRepeatOff called\n"); return 0; } 
int XChangeActivePointerGrab() { DPRINTF("XChangeActivePointerGrab called\n"); return 0; } 
int XShrinkRegion() { DPRINTF("XShrinkRegion called\n"); return 0; } 

/* required for gtk+ 2.0.6*/
int XShapeCombineRectangles() { DPRINTF("XShapeCombineRectangles called\n"); return 0; } 
int XShapeGetRectangles() { DPRINTF("XShapeGetRectangles called\n"); return 0; } 
int XAddConnectionWatch() { DPRINTF("XAddConnectionWatch called\n"); return 0; } 
int XProcessInternalConnection() { DPRINTF("XProcessInternalConnection called\n"); return 0;}
int XCopyGC() { DPRINTF("XCopyGC called\n"); return 0;}
//int XGetSubImage() { DPRINTF("XGetSubImage called\n"); return 0;}
int XGetMotionEvents() { DPRINTF("XGetMotionEvents called\n"); return 0;}
//int XQueryExtension() { DPRINTF("XQueryExtension called\n"); return 0; } 
int XwcDrawString() { DPRINTF("XwcDrawString called\n"); return 0;}

int XwcTextExtents() { DPRINTF("XwcTextExtents called\n"); return 0;}
int XwcTextEscapement() { DPRINTF("XwcTextEscapement called\n"); return 0;}

int XmbTextPropertyToTextList() { DPRINTF("XmbTextPropertyToTextList called\n"); return 0;}
int XmbTextEscapement() { DPRINTF("XmbTextEscapement called\n"); return 0;}
int XmbResetIC(int i) { DPRINTF("XmbResetIC called\n"); return 0; } 
int XGetICValues(int i, int j) { DPRINTF("XGetICValues called\n"); return 0; } 
int XFontsOfFontSet() { DPRINTF("XFontsOfFontSet called\n"); return 0;}
int XBaseFontNameListOfFontSet() { DPRINTF("XBaseFontNameListOfFontSet called\n"); return 0;}
int XkbLibraryVersion() { DPRINTF("XkbLibraryVersion called\n"); return 0; } 
//int XDisplayKeycodes() { DPRINTF("XDisplayKeycodes called\n"); return 0;}
//int XGetKeyboardMapping() { DPRINTF("XGetKeyboardMapping called\n"); return 0;}
int XGetKeyboardControl() { DPRINTF("XGetKeyboardControl called\n"); return 0; } 
//int XShmQueryExtension() { DPRINTF("XShmQueryExtension called\n"); return 0; } 
int XShmAttach() { DPRINTF("XShmAttach called\n"); return 0; } 
int XShmDetach() { DPRINTF("XShmDetach called\n"); return 0; }
int XShmCreateImage() { DPRINTF("XShmCreateImage called\n"); return 0; } 
int XShmCreatePixmap () {DPRINTF("XShmCreatePixmap called\n"); return 0; }
int XShmPutImage() { DPRINTF("XShmPutImage called\n"); return 0; } 
int XShmGetImage() { DPRINTF("XShmGetImage called\n"); return 0; }

/* required for Xforms toolkit */
int XGetStandardColormap() { DPRINTF("XGetStandardColormap called\n"); return 0; }
int XChangeKeyboardControl() { DPRINTF("XChangeKeyboardControl called\n"); return 0; } 
int XFetchBuffer() { DPRINTF("XFetchBuffer called\n"); return 0; } 
int XCopyColormapAndFree() { DPRINTF("XCopyColormapAndFree called\n"); return 0; }

/* other required*/
//int XAddExtension() { DPRINTF("XAddExtension called\n"); return 0; } 
int XAllocColorCells() { DPRINTF("XAllocColorCells called\n"); return 0; }
int _XAllocScratch() { DPRINTF("_XAllocScratch called\n"); return 0; } 
int XAllowEvents() { DPRINTF("XAllowEvents called\n"); return 0; } 

int _XEatData() { DPRINTF("_XEatData called\n"); return 0; } 
//int XESetCloseDisplay() { DPRINTF("XESetCloseDisplay called\n"); return 0; } 
int XESetCopyGC() { DPRINTF("XESetCopyGC called\n"); return 0; } 
int XESetCreateFont() { DPRINTF("XESetCreateFont called\n"); return 0; } 
int XESetCreateGC() { DPRINTF("XESetCreateGC called\n"); return 0; } 
int XESetError() { DPRINTF("XESetError called\n"); return 0; } 
int XESetErrorString() { DPRINTF("XESetErrorString called\n"); return 0; } 
int XESetEventToWire() { DPRINTF("XESetEventToWire called\n"); return 0; } 
int XESetFlushGC() { DPRINTF("XESetFlushGC called\n"); return 0; } 
int XESetFreeFont() { DPRINTF("XESetFreeFont called\n"); return 0; } 
int XESetFreeGC() { DPRINTF("XESetFreeGC called\n"); return 0; } 
int XESetWireToEvent() { DPRINTF("XESetWireToEvent called\n"); return 0; } 
//int XExtentsOfFontSet() { DPRINTF("XExtentsOfFontSet called\n"); return 0; } 
int XFetchName() { DPRINTF("XFetchName called\n"); return 0; }
int _XFlush() { DPRINTF("_XFlush called\n"); return 0; } 
int _XFlushGCCache() { DPRINTF("_XFlushGCCache called\n"); return 0; } 
void XFreeFontSet(int i, int j) { DPRINTF("XFreeFontSet called\n"); } 
int _XGetBitsPerPixel() { DPRINTF("_XGetBitsPerPixel called\n"); return 0; } 
//int XGetGCValues(void* d, int gc, unsigned long vm, void* vr) { DPRINTF("XGetGCVAlues called\n"); return 0; }
int XGetErrorDatabaseText(void* i, char* j, char* k, char* l, char* m, int n) { DPRINTF("XGetErrorDatabaseText called\n"); return 0; } 
int XGetErrorText(void* i, int j, char* k, int l) { DPRINTF("XGetErrorText called\n"); return 0; } 
int _XGetScanlinePad() { DPRINTF("_XGetScanlinePad called\n"); return 0; } 

//int XGetWMHints() { DPRINTF("XGetWMHints called\n"); return 0; } 
int XGetWMNormalHints() { DPRINTF("XGetWMNormalHints called\n"); return 0; } 
int XGrabKeyboard(void* i, int j, int k, int l, int m, int n) { DPRINTF("XGrabKeyboard called\n"); return 0; } 
int XGrabPointer(void* i, int j, int k, unsigned int l, int m, int n, int o, int p, int q) { DPRINTF("XGrabPointer called\n"); return 0; } 
int XGrabServer() { DPRINTF("XGrabServer called\n"); return 0; } 
int XIconifyWindow() { DPRINTF("XIconifyWindow called\n"); return 0; } 
//int XInitExtension() { DPRINTF("XInitExtension called\n"); return 0; } 
int _XInitImageFuncPtrs() { DPRINTF("_XInitImageFuncPtrs called\n"); return 0; } 
int XKillClient() { DPRINTF("XKillClient called\n"); return 0; } 
////int XMaxRequestSize() { DPRINTF("XMaxRequestSize called\n"); return 0; } 
int XmbDrawImageString() { DPRINTF("XmbDrawImageString called\n"); return 0; } 
int XmbDrawString() { DPRINTF("XmbDrawString called\n"); return 0; } 
int XmbLookupString() { DPRINTF("XmbLookupString called\n"); return 0; } 
int XmbTextExtents() { DPRINTF("XmbTextExtents called\n"); return 0; } 

int XParseGeometry(char* i, int* j, int* k, unsigned int* l, unsigned int* m) { DPRINTF("XParseGeometry called\n"); return 0; } 
int _XRead() { DPRINTF("_XRead called\n"); return 0; } 
int _XReadPad() { DPRINTF("_XReadPad called\n"); return 0; } 
int _XReply() { DPRINTF("_XReply called\n"); return 0; } 
int XRestackWindows() { DPRINTF("XRestackWindows called\n"); return 0; } 
int _XSend() { DPRINTF("_XSend called\n"); return 0; } 
int XSendEvent(void* i, int j, int k, long l, void* m) { DPRINTF("XSendEvent called\n"); return 0; } 
int XSetArcMode() { DPRINTF("XSetArcMode called\n"); return 0; } 
int XSetCloseDownMode() { DPRINTF("XSetCloseDownMode called\n"); return 0; } 
/* int XSetErrorHandler() { DPRINTF("XSetErrorHandler called\n"); return 0; }  */
int XSetFillRule() { DPRINTF("XSetFillRule called\n"); return 0; } 
int _XSetLastRequestRead() { DPRINTF("_XSetLastRequestRead called\n"); return 0; } 
//int XSetLocaleModifiers(char* i) { DPRINTF("XSetLocaleModifiers called\n"); return 0; }

int XSetStandardProperties() { DPRINTF("XSetStandardProperties called\n"); return 0; } 
//int XSetNormalHints() { DPRINTF("XSetNormalHints called\n"); return 0; }
//int XSetWMProtocols() { DPRINTF("XSetWMProtocols called\n"); return 0; } 
int XSupportsLocale() { DPRINTF("XSupportsLocale called\n"); return 0; } /* 0 = does not support locale */
int XSynchronize() { DPRINTF("XSynchronize called\n"); return 0; } 
int XUngrabKeyboard(void* i, int j) { DPRINTF("XUngrabKeyboard called\n"); return 0; } 
int XUngrabPointer(void* i, int j) { DPRINTF("XUngrabPointer called\n"); return 0; } 
int XUngrabServer() { DPRINTF("XUngrabServer called\n"); return 0; } 
int XVaCreateNestedList(int i, int j) { DPRINTF("XVaCreateNestedList called\n"); return 0; } 
int _XVIDtoVisual() { DPRINTF("_XVIDtoVisual called\n"); return 0; } 
int XInstallColormap() { DPRINTF("XInstallColormap called\n"); return 0; } 
int XReconfigureWMWindow() { DPRINTF("XReconfigureWMWindow called\n"); return 0; } 
int XSetWindowColormap(int i, int j, int k) { DPRINTF("XSetWindowColormap called\n"); return 0; }
int XUninstallColormap() { DPRINTF("XUninstallColormap called\n"); return 0; } 
//int XConfigureWindow() { DPRINTF("XConfigureWindow called\n"); return 0; } 
int XForceScreenSaver() { DPRINTF("XForceScreenSaver called\n"); return 0; } 
//int XFreeModifiermap() { DPRINTF("XFreeModifiermap called\n"); return 0; } 
//int XGetInputFocus() { DPRINTF("XGetInputFocus called\n"); return 0; } 
//int XGetModifierMapping() { DPRINTF("XGetModifierMapping called\n"); return 0; } 
int XGetWMColormapWindows() { DPRINTF("XGetWMColormapWindows called\n"); return 0; } 
//int XKeysymToString() { DPRINTF("XKeysymToString called\n"); return 0; } 
int XListHosts() { DPRINTF("XListHosts called\n"); return 0; } 
//int XSetClassHint() { DPRINTF("XSetClassHint called\n"); return 0; } 
int XSetCommand() { DPRINTF("XSetCommand called\n"); return 11; } /* 11 = BADALLOC */
int XSetWindowBorderPixmap(int i, int j, int k) { DPRINTF("XSetWindowBorderPixmap called\n"); return 0; }
//int XSetWMClientMachine() { DPRINTF("XSetWMClientMachine called\n"); return 0; } 
int XSetWMColormapWindows() { DPRINTF("XSetWMColormapWindows called\n"); return 0; } 
int XStoreColor() { DPRINTF("XStoreColor called\n"); return 0; }
int XStoreColors() { DPRINTF("XStoreColors called\n"); return 0; }
int _XUnknownNativeEvent() { DPRINTF("_XUnknownNativeEvent called\n"); return 0; }
int Xutf8LookupString(int i, int j, int k, int l, int m, int n) { DPRINTF("Xutf8LookupString called\n"); return 0; }

/* also in IM.c*/
void XDestroyIC(int i) { DPRINTF("XDestroyIC called\n"); } 
void XSetICFocus(int i) { DPRINTF("XSetICFocus called\n"); } 
int XSetICValues(int i, int j) { DPRINTF("XSetICValues called\n"); return 0; } 
void XUnsetICFocus(int i) { DPRINTF("XUnsetICFocus called\n"); } 
int XGetIMValues(int i, int j) { DPRINTF("XGetIMValues called\n"); return 0; } 
int XSetIMValues() { DPRINTF("XSetIMValues called\n"); return 0; } 
int XRegisterIMInstantiateCallback() { DPRINTF("XRegisterIMInstantiateCallback called\n"); return 0; } 
int XUnregisterIMInstantiateCallback() { DPRINTF("XUnregisterIMInstantiateCallback called\n"); return 0; } 
int XIMOfIC() { DPRINTF("XIMOfIC called\n"); return 0; }
//int XLocaleOfIM() { DPRINTF("XLocaleOfIM called\n"); return 0; }

/* required for xine*/
int XResetScreenSaver() { DPRINTF("XResetScreenSaver called\n"); return 0; }
/* required for wine*/
int XGetScreenSaver() { DPRINTF("XGetScreenSaver called\n"); return 0; }
int XSetScreenSaver() { DPRINTF("XSetScreenSaver called\n"); return 0; }
/* required for gmplayer*/
int _XDefaultError() { DPRINTF("_XDefaultError called\n"); return 0; }

/*
 * Stubs for stubbing out libXext for Microwindows
 * 2016 Georg Potthast
 */
#include "nxlib.h"

/***
#include <X11/Xlibint.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/dpmsproto.h>
#include <X11/extensions/Xext.h>
#include <X11/extensions/extutil.h>

#include <X11/extensions/MITMisc.h>
#include <X11/extensions/mitmiscproto.h>

#include <X11/extensions/Xag.h>
#include <X11/extensions/agproto.h>

#include <X11/extensions/XEVI.h>
#include <X11/extensions/EVIproto.h>

#include <X11/extensions/XLbx.h>
#include <X11/extensions/lbxproto.h>

#include <X11/extensions/multibufproto.h>
#include <X11/extensions/multibuf.h>

#include <X11/extensions/securproto.h>
#include <X11/extensions/security.h>

#include <X11/Xregion.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/shapeproto.h>

#include <X11/ImUtil.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/shmproto.h>

#include <X11/extensions/sync.h>
#include <X11/extensions/syncproto.h>

#include <X11/extensions/xtestext1.h>
#include <X11/extensions/xtestext1proto.h>

#include <X11/extensions/Xcup.h>
#include <X11/extensions/cupproto.h>

#include <X11/extensions/Xdbe.h>
#include <X11/extensions/dbeproto.h>

#include <X11/extensions/geproto.h>
#include <X11/extensions/ge.h>
#include <X11/extensions/Xge.h>

#include <X11/extensions/extutil.h>
***/

#if DEBUG
static void errmsg(void) { DPRINTF("nxlib: libXext called\n"); }
#else
static void errmsg(void) {}
#endif

#define STUB		{ errmsg(); return 0; }

//	DPMS.c
int DPMSQueryExtension() STUB
int DPMSGetVersion() STUB
int DPMSCapable() STUB
int DPMSSetTimeouts() STUB
int DPMSGetTimeouts() STUB
int DPMSEnable() STUB
int DPMSDisable() STUB
int DPMSForceLevel() STUB
int DPMSInfo() STUB
//	MITMisc.c
int XMITMiscQueryExtension() STUB
int XMITMiscSetBugMode() STUB
int XMITMiscGetBugMode() STUB
//	XAppgroup.c
int XagQueryVersion() STUB
int XagCreateEmbeddedApplicationGroup() STUB
int XagCreateNonembeddedApplicationGroup() STUB
int XagDestroyApplicationGroup() STUB
int XagGetApplicationGroupAttributes() STUB
int XagQueryApplicationGroup() STUB
int XagCreateAssociation() STUB
int XagDestroyAssociation() STUB
//	XLbx.c
int XLbxQueryExtension() STUB
int XLbxGetEventBase() STUB
int XLbxQueryVersion() STUB
//	XMultibuf.c
int XmbufQueryExtension() STUB
int XmbufGetVersion() STUB
int XmbufCreateBuffers() STUB
int XmbufDestroyBuffers() STUB
int XmbufDisplayBuffers() STUB
int XmbufGetWindowAttributes() STUB
int XmbufChangeWindowAttributes() STUB
int XmbufGetBufferAttributes() STUB
int XmbufChangeBufferAttributes() STUB
int XmbufGetScreenInfo() STUB
int XmbufCreateStereoWindow() STUB
int XmbufClearBufferArea() STUB
//	XSecurity.c
//int XSecurityQueryExtension() STUB
int XSecurityAllocXauth() STUB
int XSecurityFreeXauth() STUB
int XSecurityGenerateAuthorization() STUB
int XSecurityRevokeAuthorization() STUB
//	XShape.c
//int XShapeQueryExtension() STUB
//int XShapeQueryVersion() STUB
int XShapeCombineRegion() STUB
//int XShapeCombineRectangles() STUB
//int XShapeCombineMask() STUB
int XShapeCombineShape() STUB
int XShapeOffsetShape() STUB
int XShapeQueryExtents() STUB
int XShapeSelectInput() STUB
int XShapeInputSelected() STUB
//int XShapeGetRectangles() STUB
//	XSync.c
//int XSyncQueryExtension() STUB
int XSyncInitialize() STUB
int XSyncListSystemCounters() STUB
int XSyncFreeSystemCounterList() STUB
int XSyncCreateCounter() STUB
int XSyncSetCounter() STUB
int XSyncChangeCounter() STUB
int XSyncDestroyCounter() STUB
int XSyncQueryCounter() STUB
int XSyncAwait() STUB
int XSyncCreateAlarm() STUB
int XSyncDestroyAlarm() STUB
int XSyncQueryAlarm() STUB
int XSyncChangeAlarm() STUB
int XSyncSetPriority() STUB
int XSyncGetPriority() STUB
int XSyncIntToValue() STUB
int XSyncIntsToValue() STUB
int XSyncValueGreaterThan() STUB
int XSyncValueLessThan() STUB
int XSyncValueGreaterOrEqual() STUB
int XSyncValueLessOrEqual() STUB
int XSyncValueEqual() STUB
int XSyncValueIsNegative() STUB
int XSyncValueIsZero() STUB
int XSyncValueIsPositive() STUB
int XSyncValueLow32() STUB
int XSyncValueHigh32() STUB
int XSyncValueAdd() STUB
int XSyncValueSubtract() STUB
int XSyncMaxValue() STUB
int XSyncMinValue() STUB
//	XTestExt1.c
int XTestFakeInput() STUB
int XTestGetInput() STUB
int XTestStopInput() STUB
int XTestReset() STUB
int XTestQueryInputSize() STUB
int XTestPressKey() STUB
int XTestPressButton() STUB
int XTestMovePointer() STUB
int XTestFlush() STUB
//	Xcup.c
int XcupQueryVersion() STUB
int XcupGetReservedColormapEntries() STUB
int XcupStoreColors() STUB
//	Xdbe.c
int XdbeQueryExtension() STUB
int XdbeAllocateBackBufferName() STUB
int XdbeDeallocateBackBufferName() STUB
int XdbeSwapBuffers() STUB
int XdbeBeginIdiom() STUB
int XdbeEndIdiom() STUB
int XdbeGetVisualInfo() STUB
int XdbeFreeVisualInfo() STUB
int XdbeGetBackBufferAttributes() STUB
//	Xge.c
int XGEQueryExtension() STUB
int XGEQueryVersion() STUB
//	extutil.c
int XextCreateExtension() STUB
int XextDestroyExtension() STUB
int XextAddDisplay() STUB
int XextRemoveDisplay() STUB
//int XextFindDisplay() STUB
int XSetExtensionErrorHandler() STUB
int XMissingExtension() STUB

/*
Stubs for stubbing out libXext for Microwindows
2016 Georg Potthast
*/

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



#include <stdio.h>

#if DEBUG
#define DPRINTF(str, args...)   fprintf(stderr, str, ##args)  /* debug output*/
#else
#define DPRINTF(str, ...)									  /* no debug output*/
#endif

#if 0 //example
//int XScreenResourceString() { DPRINTF("XScreenResourceString called\n"); return 0; } 
#endif

struct xagstuff {
    int attrib_mask;
    Bool app_group_leader;
    Bool single_screen;
    Window default_root;
    VisualID root_visual;
    Colormap default_colormap;
    unsigned long black_pixel;
    unsigned long white_pixel;
};

//	DPMS.c	
Bool
DPMSQueryExtension (Display *dpy, int *event_basep, int *error_basep)
{DPRINTF("libXext called\n"); return 0;}

Status
DPMSGetVersion(Display *dpy, int *major_versionp, int *minor_versionp)
{DPRINTF("libXext called\n"); return 0;}
	
Bool
DPMSCapable(Display *dpy)
{DPRINTF("libXext called\n"); return 0;}

Status
DPMSSetTimeouts(Display *dpy, CARD16 standby, CARD16 suspend, CARD16 off)
{DPRINTF("libXext called\n"); return 0;}

Bool
DPMSGetTimeouts(Display *dpy, CARD16 *standby, CARD16 *suspend, CARD16 *off)
{DPRINTF("libXext called\n"); return 0;}

Status
DPMSEnable(Display *dpy)
{DPRINTF("libXext called\n"); return 0;}

Status
DPMSDisable(Display *dpy)
{DPRINTF("libXext called\n"); return 0;}

Status
DPMSForceLevel(Display *dpy, CARD16 level)
{DPRINTF("libXext called\n"); return 0;}

Status
DPMSInfo(Display *dpy, CARD16 *power_level, BOOL *state)
{DPRINTF("libXext called\n"); return 0;}
	
//	MITMisc.c	
Bool XMITMiscQueryExtension (Display *dpy, int *event_basep, int *error_basep)
{DPRINTF("libXext called\n"); return 0;}
	
Status XMITMiscSetBugMode(Display *dpy, Bool onOff)
{DPRINTF("libXext called\n"); return 0;}

Bool XMITMiscGetBugMode(Display *dpy)
{DPRINTF("libXext called\n"); return 0;}

	
//	XAppgroup.c	
Bool
XagQueryVersion(
    Display *dpy,
    int *major_version_return,
    int *minor_version_return)
{DPRINTF("libXext called\n"); return 0;}
	
static void
StuffToWire (Display *dpy, struct xagstuff *stuff, xXagCreateReq *req)
{DPRINTF("libXext called\n"); return ;}

Bool 
XagCreateEmbeddedApplicationGroup(
    Display* dpy,
    VisualID root_visual,
    Colormap default_colormap,
    unsigned long black_pixel,
    unsigned long white_pixel,
    XAppGroup* app_group_return)
{DPRINTF("libXext called\n"); return 0;}
	
Bool 
XagCreateNonembeddedApplicationGroup(
    Display* dpy,
    XAppGroup* app_group_return)
{DPRINTF("libXext called\n"); return 0;}

Bool XagDestroyApplicationGroup(Display* dpy, XAppGroup app_group)
{DPRINTF("libXext called\n"); return 0;}

Bool
XagGetApplicationGroupAttributes(Display* dpy, XAppGroup app_group, ...)
{DPRINTF("libXext called\n"); return 0;}

Bool
XagQueryApplicationGroup(
    Display* dpy,
    XID resource,
    XAppGroup* app_group_return)
{DPRINTF("libXext called\n"); return 0;}

Bool
XagCreateAssociation(Display* dpy, Window* window_return, void* system_window)
{DPRINTF("libXext called\n"); return ;}

Bool
XagDestroyAssociation(Display* dpy, Window window)
{DPRINTF("libXext called\n"); return 0;}

	
//	XEVI.c - no public functions	

//	XLbx.c	
Bool XLbxQueryExtension (
    Display *dpy,
    int *requestp, int *event_basep, int *error_basep)
{DPRINTF("libXext called\n"); return 0;}
	
int XLbxGetEventBase(Display *dpy)
{DPRINTF("libXext called\n"); return 0;}

Bool XLbxQueryVersion(Display *dpy, int *majorVersion, int *minorVersion)
{DPRINTF("libXext called\n"); return 0;}

	
//	XMultibuf.c
Bool XmbufQueryExtension (
    Display *dpy,
    int *event_base_return, int *error_base_return)
{DPRINTF("libXext called\n"); return 0;}
	
Status XmbufGetVersion (
    Display *dpy,
    int *major_version_return, int *minor_version_return)
{DPRINTF("libXext called\n"); return 0;}

int XmbufCreateBuffers (
    Display *dpy,
    Window w,
    int count,
    int update_action, int update_hint,
    Multibuffer *buffers)
{DPRINTF("libXext called\n"); return 0;}

void XmbufDestroyBuffers (Display *dpy, Window window)
{DPRINTF("libXext called\n"); return ;}

void XmbufDisplayBuffers (
    Display *dpy,
    int count,
    Multibuffer *buffers,
    int min_delay, int max_delay)
{DPRINTF("libXext called\n"); return ;}

Status XmbufGetWindowAttributes (
    Display *dpy,
    Window w,
    XmbufWindowAttributes *attr)
{DPRINTF("libXext called\n"); return 0;}

void XmbufChangeWindowAttributes (
    Display *dpy,
    Window w,
    unsigned long valuemask,
    XmbufSetWindowAttributes *attr)
{DPRINTF("libXext called\n"); return ;}

Status XmbufGetBufferAttributes (
    Display *dpy,
    Multibuffer b,
    XmbufBufferAttributes *attr)
{DPRINTF("libXext called\n"); return 0;}

void XmbufChangeBufferAttributes (
    Display *dpy,
    Multibuffer b,
    unsigned long valuemask,
    XmbufSetBufferAttributes *attr)
{DPRINTF("libXext called\n"); return ;}

Status XmbufGetScreenInfo (
    Display *dpy,
    Drawable d,
    int *nmono_return,
    XmbufBufferInfo **mono_info_return,
    int *nstereo_return,
    XmbufBufferInfo **stereo_info_return)
{DPRINTF("libXext called\n"); return 0;}

Window XmbufCreateStereoWindow (
    Display *dpy,
    Window parent,
    int x, int y,
    unsigned int width, unsigned int height, unsigned int border_width,
    int depth,
    unsigned int class,
    Visual *visual,
    unsigned long valuemask,
    XSetWindowAttributes *attr,
    Multibuffer *leftp, Multibuffer *rightp)
{DPRINTF("libXext called\n"); return 0;}

void XmbufClearBufferArea (
    Display *dpy,
    Multibuffer buffer,
    int x, int y,
    unsigned int width, unsigned int height,
    Bool exposures)
{DPRINTF("libXext called\n"); return ;}

	
//	XSecurity.c
Status XSecurityQueryExtension (
    Display *dpy,
    int *major_version_return,
    int *minor_version_return)
{DPRINTF("libXext called\n"); return 0;}
    
Xauth *
XSecurityAllocXauth(void)
{DPRINTF("libXext called\n"); return ;}

void
XSecurityFreeXauth(Xauth *auth)
{DPRINTF("libXext called\n"); return ;}

static int //not stubbed
Ones(Mask mask)
{
    register Mask y;

    y = (mask >> 1) &033333333333;
    y = mask - y - ((y >>1) & 033333333333);
    return (((y + (y >> 3)) & 030707070707) % 077);
}

Xauth *
XSecurityGenerateAuthorization(
    Display *dpy,
    Xauth *auth_in,
    unsigned long valuemask,
    XSecurityAuthorizationAttributes *attributes,
    XSecurityAuthorization *auth_id_return)
{DPRINTF("libXext called\n"); return 0;}
    
Status
XSecurityRevokeAuthorization(
    Display *dpy,
    XSecurityAuthorization auth_id)
{DPRINTF("libXext called\n"); return 0;}
    
        
//	XShape.c
Bool XShapeQueryExtension (Display *dpy, int *event_basep, int *error_basep)
{DPRINTF("libXext called\n"); return 0;}
     
Status XShapeQueryVersion(
    Display *dpy,
    int *major_versionp,
    int *minor_versionp)
{DPRINTF("libXext called\n"); return 0;}
    
void XShapeCombineRegion(
    register Display *dpy,
    Window dest,
    int destKind, int xOff, int yOff,
    register REGION *r,
    int op)
{DPRINTF("libXext called\n"); return ;}
    
void XShapeCombineRectangles (
    register Display *dpy,
    XID dest,
    int destKind, int xOff, int yOff,
    XRectangle *rects,
    int n_rects,
    int op, int ordering)
{DPRINTF("libXext called\n"); return ;}
    
void XShapeCombineMask (
    register Display *dpy,
    XID dest,
    int destKind,
    int xOff, int yOff,
    Pixmap src,
    int op)
{DPRINTF("libXext called\n"); return ;}
    
void XShapeCombineShape (
    register Display *dpy,
    XID dest,
    int destKind,
    int xOff, int yOff,
    XID src,
    int srcKind,
    int op)
{DPRINTF("libXext called\n"); return ;}
    
void XShapeOffsetShape (
    register Display *dpy,
    XID dest,
    int destKind,
    int xOff, int yOff)
{DPRINTF("libXext called\n"); return ;}
    
Status XShapeQueryExtents (
    register Display *dpy,
    Window window,
    int *bShaped, int *xbs, int *ybs, unsigned int *wbs, unsigned int *hbs, /* RETURN */
    int *cShaped, int *xcs, int *ycs, unsigned int *wcs, unsigned int *hcs /* RETURN */)
{DPRINTF("libXext called\n"); return 0;}
    
void XShapeSelectInput (
    register Display *dpy,
    Window window,
    unsigned long mask)
{DPRINTF("libXext called\n"); return ;}
    
unsigned long XShapeInputSelected (register Display *dpy, Window window)
{DPRINTF("libXext called\n"); return 0;}
         
XRectangle *XShapeGetRectangles (
    register Display *dpy,
    Window window,
    int kind,
    int *count, /* RETURN */
    int *ordering /* RETURN */)
{DPRINTF("libXext called\n"); return 0;}
    
                                                  
//	XShm.c	
#if 0 //these are already in nx11/stub-xhm.c
/* XShm - shared memory extensions - include/X11/extensions/XShm.h */
Bool XShmQueryExtension() { DPRINTF("XShmQueryExtension called\n"); return 0; }
int XShmGetEventBase() { DPRINTF("XShmGetEventBase called\n"); return 0; }
Bool XShmQueryVersion() { DPRINTF("XShmQueryVersion called\n"); return 0; }
int XShmPixmapFormat() { DPRINTF("XShmPixmapFormat called\n"); return 0; }

Bool XShmAttach() { DPRINTF("XShmAttach called\n"); return 0; }
Bool XShmDetach() { DPRINTF("XShmDetach called\n"); return 0; }
Bool XShmPutImage() { DPRINTF("XShmPutImage called\n"); return 0; }
Bool XShmGetImage() { DPRINTF("XShmGetImage called\n"); return 0; }

/*
XImage *XShmCreateImage
Pixmap XShmCreatePixmap
*/

//added by GP in 2016
static int _XShmDestroyImage (XImage *ximage)
{DPRINTF("_XShmDestroyImage\n"); return 0; }
       
XImage *XShmCreateImage (
    register Display *dpy,
    register Visual *visual,
    unsigned int depth,
    int format,
    char *data,
    XShmSegmentInfo *shminfo,
    unsigned int width,
    unsigned int height)
{DPRINTF("XShmCreateImage called\n"); return 0; }

Pixmap XShmCreatePixmap (
    register Display *dpy,
    Drawable d,
    char *data,
    XShmSegmentInfo *shminfo,
    unsigned int width, unsigned int height, unsigned int depth)
{DPRINTF("XShmCreatePixmap called\n"); return 0; }
#endif

//	XSync.c	
static Bool
wire_to_event(Display *dpy, XEvent *event, xEvent *wire)
{DPRINTF("libXext called\n"); return 0;}
                      
static Status
event_to_wire(Display *dpy, XEvent *event, xEvent *wire)
{DPRINTF("libXext called\n"); return 0;}
                      
Status
XSyncQueryExtension(
    Display *dpy,
    int *event_base_return, int *error_base_return)
{DPRINTF("libXext called\n"); return 0;}
    
Status
XSyncInitialize(
    Display *dpy,
    int *major_version_return, int *minor_version_return)
{DPRINTF("libXext called\n"); return 0;}
    
XSyncSystemCounter *
XSyncListSystemCounters(Display *dpy, int *n_counters_return)
{DPRINTF("libXext called\n"); return 0;}
                                
void
XSyncFreeSystemCounterList(XSyncSystemCounter *list)
{DPRINTF("libXext called\n"); return ;}
                                              
XSyncCounter 
XSyncCreateCounter(Display *dpy, XSyncValue initial_value)
{DPRINTF("libXext called\n"); return 0;}
                           
Status
XSyncSetCounter(Display *dpy, XSyncCounter counter, XSyncValue value)
{DPRINTF("libXext called\n"); return 0;}
                        
Status
XSyncChangeCounter(Display *dpy, XSyncCounter counter, XSyncValue value)
{DPRINTF("libXext called\n"); return 0;}
                           
Status
XSyncDestroyCounter(Display *dpy, XSyncCounter counter)
{DPRINTF("libXext called\n"); return 0;}
                            
Status
XSyncQueryCounter(Display *dpy, XSyncCounter counter, XSyncValue *value_return)
{DPRINTF("libXext called\n"); return 0;}
                          
Status
XSyncAwait(Display *dpy, XSyncWaitCondition *wait_list, int n_conditions)
{DPRINTF("libXext called\n"); return 0;}
                   
static void
_XProcessAlarmAttributes(Display *dpy, xSyncChangeAlarmReq *req,
			 unsigned long valuemask,
			 XSyncAlarmAttributes *attributes)
{DPRINTF("libXext called\n"); return ;}
     
XSyncAlarm
XSyncCreateAlarm(
    Display *dpy,
    unsigned long values_mask,
    XSyncAlarmAttributes *values)
{DPRINTF("libXext called\n"); return 0;}
    
Status
XSyncDestroyAlarm(Display *dpy, XSyncAlarm alarm)
{DPRINTF("libXext called\n"); return 0;}
                          
Status
XSyncQueryAlarm(
    Display *dpy,
    XSyncAlarm alarm,
    XSyncAlarmAttributes *values_return)
{DPRINTF("libXext called\n"); return 0;}
    
Status
XSyncChangeAlarm(
    Display *dpy,
    XSyncAlarm alarm,
    unsigned long values_mask,
    XSyncAlarmAttributes *values)
{DPRINTF("libXext called\n"); return 0;}
    
Status
XSyncSetPriority(
    Display *dpy,
    XID client_resource_id,
    int priority)
{DPRINTF("libXext called\n"); return 0;}
    
Status
XSyncGetPriority(Display *dpy, XID client_resource_id, int *return_priority)
{DPRINTF("libXext called\n"); return 0;}
                         
/*
 *  Functions corresponding to the macros for manipulating 64-bit values
 */

void
XSyncIntToValue(XSyncValue *pv, int i)
{DPRINTF("libXext called\n"); return ;}

void
XSyncIntsToValue(XSyncValue *pv, unsigned int l, int h)
{DPRINTF("libXext called\n"); return ;}

Bool
XSyncValueGreaterThan(XSyncValue a, XSyncValue b)
{DPRINTF("libXext called\n"); return 0;}

Bool
XSyncValueLessThan(XSyncValue a, XSyncValue b)
{DPRINTF("libXext called\n"); return 0;}

Bool
XSyncValueGreaterOrEqual(XSyncValue a, XSyncValue b)
{DPRINTF("libXext called\n"); return 0;}

Bool
XSyncValueLessOrEqual(XSyncValue a, XSyncValue b)
{DPRINTF("libXext called\n"); return 0;}

Bool
XSyncValueEqual(XSyncValue a, XSyncValue b)
{DPRINTF("libXext called\n"); return 0;}

Bool
XSyncValueIsNegative(XSyncValue v)
{DPRINTF("libXext called\n"); return 0;}

Bool
XSyncValueIsZero(XSyncValue a)
{DPRINTF("libXext called\n"); return 0;}

Bool
XSyncValueIsPositive(XSyncValue v)
{DPRINTF("libXext called\n"); return 0;}

unsigned int
XSyncValueLow32(XSyncValue v)
{DPRINTF("libXext called\n"); return 0;}

int
XSyncValueHigh32(XSyncValue v)
{DPRINTF("libXext called\n"); return 0;}

void
XSyncValueAdd(XSyncValue *presult, XSyncValue a, XSyncValue b, Bool *poverflow)
{DPRINTF("libXext called\n"); return ;}

void
XSyncValueSubtract(
    XSyncValue *presult,
    XSyncValue a, XSyncValue b,
    Bool *poverflow)
{DPRINTF("libXext called\n"); return ;}

void
XSyncMaxValue(XSyncValue *pv)
{DPRINTF("libXext called\n"); return ;}

void
XSyncMinValue(XSyncValue *pv)
{DPRINTF("libXext called\n"); return ;}

                                                                                                                                                                                                                        
//	XTestExt1.c
int
XTestFakeInput(
register Display	*dpy,
char			*action_list_addr,
int			action_list_size,
int			ack_flag)
{DPRINTF("libXext called\n"); return 0;}	

int
XTestGetInput(
register Display	*dpy,
int			action_handling)
{DPRINTF("libXext called\n"); return 0;}	

int
XTestStopInput(
register Display	*dpy)
{DPRINTF("libXext called\n"); return 0;}	

int
XTestReset(
register Display	*dpy)
{DPRINTF("libXext called\n"); return 0;}	

int
XTestQueryInputSize(
register Display	*dpy,
unsigned long		*size_return)
{DPRINTF("libXext called\n"); return 0;}	

static int
XTestCheckExtInit(
register Display	*dpy)
{DPRINTF("libXext called\n"); return 0;}

static int
XTestInitExtension(
register Display	*dpy)
{DPRINTF("libXext called\n"); return 0;}

static Bool
XTestWireToEvent(
Display	*dpy,
XEvent	*reTemp,
xEvent	*eventTemp)
{DPRINTF("libXext called\n"); return 0;}

int
XTestPressKey(
Display		*display,
int		device_id,
unsigned long	delay,
unsigned int	keycode,
unsigned int	key_action)
{DPRINTF("libXext called\n"); return 0;}
         
int
XTestPressButton(
Display		*display,
int		device_id,
unsigned long	delay,
unsigned int	button_number,
unsigned int	button_action)
{DPRINTF("libXext called\n"); return 0;}
         
static int
XTestKeyOrButton(
Display		*display,
int		device_id,
unsigned long	delay,
unsigned int	code,
unsigned int	action)
{DPRINTF("libXext called\n"); return 0;}
         
int
XTestMovePointer(
Display		*display,
int		device_id,
unsigned long	delay[],
int		x[],
int		y[],
unsigned int	count)
{DPRINTF("libXext called\n"); return 0;}
         
static int
XTestCheckDelay(
Display		*display,
unsigned long	*delay_addr)
{DPRINTF("libXext called\n"); return 0;}
         
static int
XTestPackInputAction(
Display	*display,
CARD8	*action_addr,
int	action_size)
{DPRINTF("libXext called\n"); return 0;}
    
static int
XTestWriteInputActions(
Display	*display,
char	*action_list_addr,
int	action_list_size,
int	ack_flag)
{DPRINTF("libXext called\n"); return 0;}
    
static	Bool
XTestIdentifyMyEvent(
Display	*display,
XEvent	*event_ptr,
char	*args)
{DPRINTF("libXext called\n"); return 0;}                                                     

int
XTestFlush(Display *display)
{DPRINTF("libXext called\n"); return 0;}


//	Xcup.c	
Status
XcupQueryVersion(
    Display* dpy,
    int* major_version_return,
    int* minor_version_return)
{DPRINTF("libXext called\n"); return 0;}
    
Status 
XcupGetReservedColormapEntries(
    Display* dpy,
    int screen,
    XColor** colors_out,
    int* ncolors)
{DPRINTF("libXext called\n"); return 0;}
    
Status
XcupStoreColors(
    Display* dpy,
    Colormap colormap,
    XColor* colors_in_out,
    int ncolors)
{DPRINTF("libXext called\n"); return 0;}

    
//	Xdbe.c	
Status XdbeQueryExtension (
    Display *dpy,
    int *major_version_return,
    int *minor_version_return)
{DPRINTF("libXext called\n"); return 0;}
    
XdbeBackBuffer XdbeAllocateBackBufferName(
    Display *dpy,
    Window window,
    XdbeSwapAction swap_action)
{DPRINTF("libXext called\n"); return 0;}
    
Status XdbeDeallocateBackBufferName (
    Display *dpy,
    XdbeBackBuffer buffer)
{DPRINTF("libXext called\n"); return 0;}
    
Status XdbeSwapBuffers (
    Display *dpy,
    XdbeSwapInfo *swap_info,
    int num_windows)
{DPRINTF("libXext called\n"); return 0;}
    
Status XdbeBeginIdiom (Display *dpy)
{DPRINTF("libXext called\n"); return 0;}
       
Status XdbeEndIdiom (Display *dpy)
{DPRINTF("libXext called\n"); return 0;}
       
XdbeScreenVisualInfo *XdbeGetVisualInfo (
    Display        *dpy,
    Drawable       *screen_specifiers,
    int            *num_screens)  /* SEND and RETURN */
{DPRINTF("libXext called\n"); return 0;}
    
void XdbeFreeVisualInfo(XdbeScreenVisualInfo *visual_info)
{DPRINTF("libXext called\n"); return ;}
     
XdbeBackBufferAttributes *XdbeGetBackBufferAttributes(
    Display *dpy,
    XdbeBackBuffer buffer)
{DPRINTF("libXext called\n"); return 0;}
    
                                           
//	Xge.c	
Bool
XGEQueryExtension(Display* dpy, int* event_base, int* error_base)
{DPRINTF("libXext called\n"); return 0;}
	
Bool
XGEQueryVersion(Display* dpy,
   int *major_version,
   int *minor_version)
{DPRINTF("libXext called\n"); return 0;}

	
//	extutil.c
XExtensionInfo *XextCreateExtension (void)
{DPRINTF("libXext called\n"); return ;}
               
void XextDestroyExtension (XExtensionInfo *info)
{DPRINTF("libXext called\n"); return ;}

/*  
// already declared in extutil.h:123
XExtDisplayInfo *XextAddDisplay (
    XExtensionInfo *extinfo,
    Display *dpy,
    char *ext_name,
    XExtensionHooks *hooks,
    int nevents,
    XPointer data)
{DPRINTF("libXext called\n"); return 0;}
*/    
int XextRemoveDisplay (XExtensionInfo *extinfo, Display *dpy)
{DPRINTF("libXext called\n"); return 0;}
    
XExtDisplayInfo *XextFindDisplay (XExtensionInfo *extinfo, Display *dpy)
{DPRINTF("libXext called\n"); return 0;}
                
static int _default_exterror (Display *dpy, _Xconst char *ext_name, _Xconst char *reason)
{DPRINTF("libXext called\n"); return 0;}
       
XextErrorHandler XSetExtensionErrorHandler (XextErrorHandler handler)
{DPRINTF("libXext called\n"); return 0;}
                 
int XMissingExtension (Display *dpy, _Xconst char *ext_name)
{DPRINTF("libXext called\n"); return 0;}
    
                                                                            
//	globals.c
// not done

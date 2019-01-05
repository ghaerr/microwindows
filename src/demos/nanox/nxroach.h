#include "../../images/demos/mwin/mwdialog/mwdialog.h" //defines e.g. IDD_DLGMAIN
#include "../../images/demos/nanox/nxroach/roach000.xbm"
#include "../../images/demos/nanox/nxroach/roach015.xbm"
#include "../../images/demos/nanox/nxroach/roach030.xbm"
#include "../../images/demos/nanox/nxroach/roach045.xbm"
#include "../../images/demos/nanox/nxroach/roach060.xbm"
#include "../../images/demos/nanox/nxroach/roach075.xbm"
#include "../../images/demos/nanox/nxroach/roach090.xbm"
#include "../../images/demos/nanox/nxroach/roach105.xbm"
#include "../../images/demos/nanox/nxroach/roach120.xbm"
#include "../../images/demos/nanox/nxroach/roach135.xbm"
#include "../../images/demos/nanox/nxroach/roach150.xbm"
#include "../../images/demos/nanox/nxroach/roach165.xbm"
#include "../../images/demos/nanox/nxroach/roach180.xbm"
#include "../../images/demos/nanox/nxroach/roach195.xbm"
#include "../../images/demos/nanox/nxroach/roach210.xbm"
#include "../../images/demos/nanox/nxroach/roach225.xbm"
#include "../../images/demos/nanox/nxroach/roach240.xbm"
#include "../../images/demos/nanox/nxroach/roach255.xbm"
#include "../../images/demos/nanox/nxroach/roach270.xbm"
#include "../../images/demos/nanox/nxroach/roach285.xbm"
#include "../../images/demos/nanox/nxroach/roach300.xbm"
#include "../../images/demos/nanox/nxroach/roach315.xbm"
#include "../../images/demos/nanox/nxroach/roach330.xbm"
#include "../../images/demos/nanox/nxroach/roach345.xbm"

#define ROACH_HEADINGS	24	/* number of orientations */
#define ROACH_ANGLE	15	/* angle between orientations */

typedef struct RoachMap {
	char *roachBits;
	Pixmap pixmap;
	int width;
	int height;
	float sine;
	float cosine;
} RoachMap;

RoachMap roachPix[] = {
	{roach000_bits, None, roach000_height, roach000_width, 0.0, 0.0},
	{roach015_bits, None, roach015_height, roach015_width, 0.0, 0.0},
	{roach030_bits, None, roach030_height, roach030_width, 0.0, 0.0},
	{roach045_bits, None, roach045_height, roach045_width, 0.0, 0.0},
	{roach060_bits, None, roach060_height, roach060_width, 0.0, 0.0},
	{roach075_bits, None, roach075_height, roach075_width, 0.0, 0.0},
	{roach090_bits, None, roach090_height, roach090_width, 0.0, 0.0},
	{roach105_bits, None, roach105_height, roach105_width, 0.0, 0.0},
	{roach120_bits, None, roach120_height, roach120_width, 0.0, 0.0},
	{roach135_bits, None, roach135_height, roach135_width, 0.0, 0.0},
	{roach150_bits, None, roach150_height, roach150_width, 0.0, 0.0},
	{roach165_bits, None, roach165_height, roach165_width, 0.0, 0.0},
	{roach180_bits, None, roach180_height, roach180_width, 0.0, 0.0},
	{roach195_bits, None, roach195_height, roach195_width, 0.0, 0.0},
	{roach210_bits, None, roach210_height, roach210_width, 0.0, 0.0},
	{roach225_bits, None, roach225_height, roach225_width, 0.0, 0.0},
	{roach240_bits, None, roach240_height, roach240_width, 0.0, 0.0},
	{roach255_bits, None, roach255_height, roach255_width, 0.0, 0.0},
	{roach270_bits, None, roach270_height, roach270_width, 0.0, 0.0},
	{roach285_bits, None, roach285_height, roach285_width, 0.0, 0.0},
	{roach300_bits, None, roach300_height, roach300_width, 0.0, 0.0},
	{roach315_bits, None, roach315_height, roach315_width, 0.0, 0.0},
	{roach330_bits, None, roach330_height, roach330_width, 0.0, 0.0},
	{roach345_bits, None, roach345_height, roach345_width, 0.0, 0.0},
};



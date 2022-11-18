// Classes from MFC not yet expunged from source.
/*
#include "mfc_emulation.h"
*/

#if !defined( _MFC_EMULATION_INCLUDED_ ) && !defined( _MFC_VER )
#define _MFC_EMULATION_INCLUDED_

#if !defined( DEBUG_NEW )
#define DEBUG_NEW new
#endif

struct CPoint
{
	CPoint( long px = 0, long py = 0) :
		x(px)
		,y(py)
	{
	};

	long  x;
	long  y;
};

struct CSize
{
	CSize( long pcx = 0, long pcy = 0) :
		cx(pcx)
		,cy(pcy)
	{
	};
	long  cx;
	long  cy;
};

struct CRect
{
	CRect(long l = 0, long t = 0, long r = 0, long b = 0) :
		left(l)
		,top(t)
		,right(r)
		,bottom(b)
	{};
	long    left;
	long    top;
	long    right;
	long    bottom;
};

// SERIALISATION.
// define only enough to satisfy compiler (until I can remove all this code).

#define DECLARE_SERIAL(whatever)
#define DECLARE_DYNCREATE(whatever)
#define IMPLEMENT_SERIAL(whatever,something,nothing)

class CObject {};
class CArchive;
class CDC;
class CWnd;
class CPalette;
class CBitmap;

class CDocument
{
};

#if /*!defined( _DEBUG) ||*/ !defined( _MSC_VER )
// Reporting macros
#define _CRT_WARN 0
#ifdef _DEBUG

#define _RPT0(p0,p1) fprintf(stderr, p1);
#define _RPT1(p0,p1,p2) fprintf(stderr, p1, p2);
#define _RPT2(p0,p1,p2,p3) fprintf(stderr, p1, p2, p3);
#define _RPT3(p0,p1,p2,p3,p4) fprintf(stderr, p1, p2, p3, p4);
#define _RPT4(p0,p1,p2,p3,p4,p5) fprintf(stderr, p1, p2, p3, p4, p5);

// va args don't seem to work on mac
//#define _RPT_BASE(...) () (fprintf(stderr, __VA_ARGS__))
//#define _RPTN(rptno, ...) _RPT_BASE(rptno, __VA_ARGS__)
#define _RPTN(rptno, msg, ...)
#else

#define _RPT0(p0,p1)
#define _RPT1(p0,p1,p2)
#define _RPT2(p0,p1,p2,p3)
#define _RPT3(p0,p1,p2,p3,p4)
#define _RPT4(p0,p1,p2,p3,p4,p5)
#define _RPTN(rptno, msg, ...)

#endif

#define _RPTW0(p0,p1)
#define _RPTW1(p0,p1,p2)
#define _RPTW2(p0,p1,p2,p3)
#define _RPTW3(p0,p1,p2,p3,p4)
#define _RPTW4(p0,p1,p2,p3,p4,p5)
#else
#include <crtdbg.h>
#endif

#endif

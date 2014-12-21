//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _MM_GDI_H_
#define _MM_GDI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mmprof.h"
#include <wingdi.h>

//
// Delete
//

BOOL WINAPI 
DeleteObjectEnter(
	HGDIOBJ ho
	);

typedef BOOL 
(WINAPI *DELETEOBJECT)(
	HGDIOBJ ho
	);

BOOL WINAPI
DeleteDCEnter(
	HDC hdc  
	);

typedef BOOL 
(WINAPI *DELETEDC)(
	HDC hdc  
	);

int WINAPI
ReleaseDCEnter(
	HWND hWnd,
	HDC hDC  
	);

typedef int
(WINAPI *RELEASEDC)(
	HWND hWnd,
	HDC hDC
	);

BOOL WINAPI
DeleteMetaFileEnter(
	HMETAFILE hmf   
	);

typedef BOOL 
(WINAPI *DELETEMETAFILE)(
	HMETAFILE hmf   
	);

BOOL WINAPI
DeleteEnhMetaFileEnter(
	HENHMETAFILE hemf   
	);

typedef BOOL 
(WINAPI *DELETEENHMETAFILE)(
	HENHMETAFILE hemf   
	);

HMETAFILE 
WINAPI CloseMetaFileEnter(
	HDC hdc 
	);

typedef HMETAFILE 
(WINAPI *CLOSEMETAFILE)(
	HDC hdc 
	);

HENHMETAFILE 
WINAPI CloseEnhMetaFileEnter(
	HDC hdc 
	);

typedef HENHMETAFILE 
(WINAPI *CLOSEENHMETAFILE)(
	HDC hdc 
	);

//
// DC
//

HDC WINAPI
CreateCompatibleDCEnter(
	HDC hdc   
	);

typedef HDC 
(WINAPI *CREATECOMPATIBLEDC)(
	HDC hdc   
	);

HDC WINAPI 
CreateDCAEnter(
	PSTR lpszDriver,        
	PSTR lpszDevice,        
	PSTR lpszOutput,       
	CONST DEVMODE* lpInitData  
	);

typedef HDC 
(WINAPI *CREATEDC_A)(
	PSTR lpszDriver,        
	PSTR lpszDevice,       
	PSTR lpszOutput,        
	CONST DEVMODE* lpInitData 
	);

HDC WINAPI 
CreateDCWEnter(
	PWSTR lpszDriver,        
	PWSTR lpszDevice,        
	PWSTR lpszOutput,       
	CONST DEVMODE* lpInitData  
	);

typedef HDC 
(WINAPI *CREATEDC_W)(
	PWSTR lpszDriver,       
	PWSTR lpszDevice,        
	PWSTR lpszOutput,        
	CONST DEVMODE* lpInitData 
	);


HDC WINAPI 
CreateICAEnter(
	PSTR lpszDriver,        
	PSTR lpszDevice,        
	PSTR lpszOutput,        
	CONST DEVMODE* lpdvmInit 
	);

typedef HDC 
(WINAPI *CREATEIC_A)(
	PSTR lpszDriver,        
	PSTR lpszDevice,        
	PSTR lpszOutput,        
	CONST DEVMODE* lpdvmInit  
	);

HDC WINAPI 
CreateICWEnter(
	PWSTR lpszDriver,      
	PWSTR lpszDevice,      
	PWSTR lpszOutput,        
	CONST DEVMODE* lpdvmInit  
	);

typedef HDC 
(WINAPI *CREATEIC_W)(
	PWSTR lpszDriver,        
	PWSTR lpszDevice,        
	PWSTR lpszOutput,       
	CONST DEVMODE* lpdvmInit 
	);

HDC WINAPI 
GetDCEnter(
	HWND hWnd   
	);

typedef HDC 
(WINAPI *GETDC)(
	HWND hWnd   
	);

HDC WINAPI 
GetDCExEnter(
	HWND hWnd,      
	HRGN hrgnClip, 
	DWORD flags     
	);

typedef HDC 
(WINAPI *GETDCEX)(
	HWND hWnd,    
	HRGN hrgnClip,  
	DWORD flags    
	);

HDC WINAPI 
GetWindowDCEnter(
	HWND hWnd   
	);

typedef HDC 
(WINAPI *GETWINDOWDC)(
	HWND hWnd   
	);

//
// Pen
//

HPEN WINAPI
CreatePenEnter(
	int fnPenStyle,  
	int nWidth,     
	COLORREF crColor 
	);

typedef HPEN 
(WINAPI *CREATEPEN)(
	int fnPenStyle,  
	int nWidth,     
	COLORREF crColor 
	);

HPEN WINAPI 
CreatePenIndirectEnter(
	CONST LOGPEN *lplgpn 
	);

typedef HPEN 
(WINAPI *CREATEPENINDIRECT)(
	CONST LOGPEN *lplgpn 
	);

HPEN WINAPI 
ExtCreatePenEnter(
	DWORD dwPenStyle, 
	DWORD dwWidth,   
	CONST LOGBRUSH *lplb, 
	DWORD dwStyleCount,  
	CONST DWORD *lpStyle
	);

typedef HPEN 
(WINAPI *EXTCREATEPEN)(
	DWORD dwPenStyle, 
	DWORD dwWidth,   
	CONST LOGBRUSH *lplb, 
	DWORD dwStyleCount,  
	CONST DWORD *lpStyle
	);

//
// Brush
//

HBRUSH WINAPI 
CreateBrushIndirectEnter(
	CONST LOGBRUSH *lplb 
	);

typedef HBRUSH 
(WINAPI *CREATEBRUSHINDIRECT)(
	CONST LOGBRUSH *lplb 
	);

HBRUSH WINAPI 
CreateSolidBrushEnter(
	COLORREF crColor
	);

typedef HBRUSH 
(WINAPI *CREATESOLIDBRUSH)(
	COLORREF crColor
	);

HBRUSH WINAPI
CreatePatternBrushEnter(
	HBITMAP hbmp  
	);

typedef HBRUSH 
(WINAPI *CREATEPATTERNBRUSH)(
	HBITMAP hbmp  
	);

HBRUSH WINAPI
CreateDIBPatternBrushEnter(
	HGLOBAL hglbDIBPacked, 
	UINT fuColorSpec
	);

typedef HBRUSH 
(WINAPI *CREATEDIBPATTERNBRUSH)(
	HGLOBAL hglbDIBPacked, 
	UINT fuColorSpec
	);

HBRUSH WINAPI
CreateHatchBrushEnter(
	int fnStyle,      
	COLORREF clrref  
	);

typedef HBRUSH 
(WINAPI *CREATEHATCHBRUSH)(
	int fnStyle,      
	COLORREF clrref  
	);

HPALETTE WINAPI
CreateBtrftonePaletteEnter(
	HDC hdc
	);

typedef HPALETTE 
(WINAPI *CREATEHALFTONEPALETTE)(
	HDC hdc
	);

HPALETTE WINAPI 
CreatePaletteEnter(
	CONST LOGPALETTE *lplgpl
	);

typedef HPALETTE 
(WINAPI *CREATEPALETTE)(
	CONST LOGPALETTE *lplgpl
	);

//
// Font
//

HFONT WINAPI 
CreateFontAEnter(
	int nHeight,             
	int nWidth,               
	int nEscapement,          
	int nOrientation,        
	int fnWeight,             
	DWORD fdwItalic,          
	DWORD fdwUnderline,       
	DWORD fdwStrikeOut,        
	DWORD fdwCharSet,         
	DWORD fdwOutputPrecision,  
	DWORD fdwClipPrecision,  
	DWORD fdwQuality,         
	DWORD fdwPitchAndFamily,  
	PSTR lpszFace 
	);

typedef HFONT 
(WINAPI *CREATEFONT_A)(
	int nHeight,             
	int nWidth,               
	int nEscapement,          
	int nOrientation,        
	int fnWeight,             
	DWORD fdwItalic,          
	DWORD fdwUnderline,       
	DWORD fdwStrikeOut,        
	DWORD fdwCharSet,         
	DWORD fdwOutputPrecision,  
	DWORD fdwClipPrecision,  
	DWORD fdwQuality,         
	DWORD fdwPitchAndFamily,  
	PSTR lpszFace 
	);

HFONT WINAPI 
CreateFontWEnter(
	int nHeight,             
	int nWidth,               
	int nEscapement,          
	int nOrientation,        
	int fnWeight,             
	DWORD fdwItalic,          
	DWORD fdwUnderline,       
	DWORD fdwStrikeOut,        
	DWORD fdwCharSet,         
	DWORD fdwOutputPrecision,  
	DWORD fdwClipPrecision,  
	DWORD fdwQuality,         
	DWORD fdwPitchAndFamily,  
	PWSTR lpszFace        
	);

typedef HFONT 
(WINAPI *CREATEFONT_W)(
	int nHeight,             
	int nWidth,               
	int nEscapement,          
	int nOrientation,        
	int fnWeight,             
	DWORD fdwItalic,          
	DWORD fdwUnderline,       
	DWORD fdwStrikeOut,        
	DWORD fdwCharSet,         
	DWORD fdwOutputPrecision,  
	DWORD fdwClipPrecision,  
	DWORD fdwQuality,         
	DWORD fdwPitchAndFamily,  
	PWSTR lpszFace        
	);

HFONT WINAPI 
CreateFontIndirectAEnter(
	CONST LOGFONT* lplf
	);

typedef HFONT 
(WINAPI *CREATEFONTINDIRECT_A)(
	CONST LOGFONT* lplf
	);

HFONT WINAPI CreateFontIndirectWEnter(
	CONST LOGFONT* lplf 
	);

typedef HFONT 
(WINAPI *CREATEFONTINDIRECT_W)(
	CONST LOGFONT* lplf  
	);

HFONT WINAPI 
CreateFontIndirectExAEnter(
	CONST ENUMLOGFONTEXDV *penumlfex
	);

typedef HFONT 
(WINAPI *CREATEFONTINDIRECTEX_A)(
	CONST ENUMLOGFONTEXDV *penumlfex 
	);

HFONT WINAPI 
CreateFontIndirectExWEnter(
	CONST ENUMLOGFONTEXDV *penumlfex
	);

typedef HFONT 
(WINAPI *CREATEFONTINDIRECTEX_W)(
	CONST ENUMLOGFONTEXDV *penumlfex 
	);

//
// Bitmap
//

HBITMAP WINAPI 
LoadBitmapAEnter(
	HINSTANCE hInstance,
	PSTR lpBitmapName 
	);

typedef HBITMAP 
(WINAPI *LOADBITMAP_A)(
	HINSTANCE hInstance,
	PSTR lpBitmapName 
	);

HBITMAP WINAPI 
LoadBitmapWEnter(
	HINSTANCE hInstance, 
	PWSTR lpBitmapName  
	);

typedef HBITMAP 
(WINAPI *LOADBITMAP_W)(
	HINSTANCE hInstance, 
	PWSTR lpBitmapName  
	);

HBITMAP WINAPI 
CreateBitmapEnter(
	int nWidth,         
	int nHeight,        
	UINT cPlanes,       
	UINT cBitsPerPel,   
	CONST VOID *lpvBits 
	);

typedef HBITMAP 
(WINAPI *CREATEBITMAP)(
	int nWidth,         
	int nHeight,        
	UINT cPlanes,       
	UINT cBitsPerPel,   
	CONST VOID *lpvBits 
	);

HBITMAP WINAPI 
CreateBitmapIndirectEnter(
	CONST BITMAP *lpbm 
	);

typedef HBITMAP 
(WINAPI *CREATEBITMAPINDIRECT)(
	CONST BITMAP *lpbm 
	);

HBITMAP WINAPI 
CreateCompatibleBitmapEnter(
	HDC hdc,    
	int nWidth,  
	int nHeight 
	);

typedef HBITMAP 
(WINAPI *CREATECOMPATIBLEBITMAP)(
	HDC hdc,   
	int nWidth,    
	int nHeight   
	);

HANDLE WINAPI 
LoadImageAEnter(
	HINSTANCE hinst,
    PSTR lpszName,
    UINT uType,
    int cxDesired,
    int cyDesired,
    UINT fuLoad
	);

typedef HANDLE 
(WINAPI *LOADIMAGE_A)(
	HINSTANCE hinst,
    PSTR lpszName,
    UINT uType,
    int cxDesired,
    int cyDesired,
    UINT fuLoad
	);

HANDLE WINAPI 
LoadImageWEnter(
	HINSTANCE hinst,
    PWSTR lpszName,
    UINT uType,
    int cxDesired,
    int cyDesired,
    UINT fuLoad
	);

typedef HANDLE 
(WINAPI *LOADIMAGE_W)(
	HINSTANCE hinst,
    PWSTR lpszName,
    UINT uType,
    int cxDesired,
    int cyDesired,
    UINT fuLoad
	);

//
// Region
//

HRGN WINAPI 
PathToRegionEnter(
	HDC hdc
	);

typedef HRGN 
(WINAPI *PATHTOREGION)(
	HDC hdc
	);

HRGN WINAPI
CreateEllipticRgnEnter(
	int nLeftRect,   
	int nTopRect,   
	int nRightRect,  
	int nBottomRect  
	);

typedef HRGN 
(WINAPI *CREATEELLIPTICRGN)(
	int nLeftRect,   
	int nTopRect,   
	int nRightRect,  
	int nBottomRect  
	);

HRGN WINAPI 
CreateEllipticRgnIndirectEnter(
	CONST RECT *lprc
	);
    
typedef HRGN 
(WINAPI *CREATEELLIPTICRGNINDIRECT)(
	CONST RECT *lprc
	);

HRGN WINAPI 
CreatePolygonRgnEnter(
	CONST POINT *lppt,  
	int cPoints,        
	int fnPolyFillMode  
	);

typedef HRGN 
(WINAPI *CREATEPOLYGONRGN)(
	CONST POINT *lppt,  
	int cPoints,        
	int fnPolyFillMode  
	);
    
HRGN WINAPI 
CreatePolyPolygonRgnEnter(
	CONST POINT *lppt,       
	CONST INT *lpPolyCounts,  
	int nCount,               
	int fnPolyFillMode      
	);

typedef HRGN 
(WINAPI *CREATEPOLYPOLYGONRGN)(
	CONST POINT *lppt,       
	CONST INT *lpPolyCounts,  
	int nCount,               
	int fnPolyFillMode      
	);

HRGN WINAPI 
CreateRectRgnEnter(
	int nLeftRect,   
	int nTopRect,    
	int nRightRect, 
	int nBottomRect
	);

typedef HRGN 
(WINAPI *CREATERECTRGN)(
	int nLeftRect,   
	int nTopRect,    
	int nRightRect, 
	int nBottomRect
	);

HRGN WINAPI 
CreateRectRgnIndirectEnter(
	CONST RECT *lprc 
	);
    
typedef HRGN 
(WINAPI *CREATERECTRGNINDIRECT)(
	CONST RECT *lprc 
	);

HRGN WINAPI 
CreateRoundRectRgnEnter(
	int nLeftRect,      
	int nTopRect,       
	int nRightRect,    
	int nBottomRect,    
	int nWidthEllipse,  
	int nHeightEllipse  
	);
    
typedef HRGN 
(WINAPI *CREATEROUNDRECTRGN)(
	int nLeftRect,      
	int nTopRect,       
	int nRightRect,    
	int nBottomRect,    
	int nWidthEllipse,  
	int nHeightEllipse  
	);

HRGN WINAPI 
ExtCreateRegionEnter(
	CONST XFORM *lpXform,     
	DWORD nCount,             
	CONST RGNDATA *lpRgnData 
	);

typedef HRGN 
(WINAPI *EXTCREATEREGION)(
	CONST XFORM *lpXform,     
	DWORD nCount,             
	CONST RGNDATA *lpRgnData  
	);

//
// Meta File
//

HDC WINAPI 
CreateEnhMetaFileAEnter(
	HDC hdcRef, 
	PSTR lpFilename, 
	CONST RECT* lpRect, 
	PSTR lpDescription  
	);

typedef HDC 
(WINAPI *CREATEENHMETAFILE_A)(
	HDC hdcRef, 
	PSTR lpFilename, 
	CONST RECT* lpRect, 
	PSTR lpDescription  
	);

HDC WINAPI 
CreateEnhMetaFileWEnter(
	HDC hdcRef,         
	PWSTR lpFilename,  
	CONST RECT* lpRect, 
	PWSTR lpDescription 
	);

typedef HDC 
(WINAPI *CREATEENHMETAFILE_W)(
	HDC hdcRef,         
	PWSTR lpFilename,  
	CONST RECT* lpRect,
	PWSTR lpDescription 
	);
   
HENHMETAFILE WINAPI 
GetEnhMetaFileAEnter(
	PSTR lpszMetaFile
	);
  
typedef HENHMETAFILE 
(WINAPI *GETENHMETAFILE_A)(
	PSTR lpszMetaFile
	);
    
HENHMETAFILE WINAPI
GetEnhMetaFileWEnter(
	PWSTR lpszMetaFile 
	);

typedef HENHMETAFILE 
(WINAPI *GETENHMETAFILE_W)(
	LPCTSTR lpszMetaFile 
	);
  
//
// External Declaration
//

extern BTR_CALLBACK MmGdiCallback[];
extern ULONG MmGdiCallbackCount;

#ifdef __cplusplus
}
#endif
#endif


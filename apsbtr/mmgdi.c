//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2016
//

#include "apsbtr.h"
#include "btrsdk.h"
#include "callback.h"
#include "heap.h"
#include "lock.h"
#include "callback.h"
#include "trap.h"
#include "heap.h"
#include "thread.h"
#include "hal.h"
#include "cache.h"
#include "stacktrace.h"
#include "util.h"
#include "mmprof.h"
#include "mmheap.h"
#include "mmgdi.h"

//
// N.B. Callback orders must be same with its enum definition
//

BTR_CALLBACK MmGdiCallback[] = {
	
	{ GdiCallbackType, 0,0,0,0, DeleteObjectEnter, "gdi32.dll", "DeleteObject" ,0 },
	{ GdiCallbackType, 0,0,0,0, DeleteDCEnter, "gdi32.dll", "DeleteDC" ,0 },
	{ GdiCallbackType, 0,0,0,0, ReleaseDCEnter, "user32.dll", "ReleaseDC" ,0 },
	{ GdiCallbackType, 0,0,0,0, DeleteEnhMetaFileEnter, "gdi32.dll", "DeleteEhnMetaFile" ,0 },
	{ GdiCallbackType, 0,0,0,0, CloseEnhMetaFileEnter, "gdi32.dll", "CloseEhnMetaFile" ,0 },

	{ GdiCallbackType, 0,0,0,0, CreateCompatibleDCEnter, "gdi32.dll", "CreateCompatibleDC" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateDCAEnter, "gdi32.dll", "CreateDCA" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateDCWEnter, "gdi32.dll", "CreateDCW" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateICAEnter, "gdi32.dll", "CreateICA" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateICWEnter, "gdi32.dll", "CreateICW" ,0 },
	{ GdiCallbackType, 0,0,0,0, GetDCEnter, "user32.dll", "GetDC" ,0 },
	{ GdiCallbackType, 0,0,0,0, GetDCExEnter, "user32.dll", "GetDCEx" ,0 },
	{ GdiCallbackType, 0,0,0,0, GetWindowDCEnter, "user32.dll", "GetWindowDC" ,0 },


	{ GdiCallbackType, 0,0,0,0, CreatePenEnter, "gdi32.dll", "CreatePen" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreatePenIndirectEnter, "gdi32.dll", "CreatePenIndirect" ,0 },
	{ GdiCallbackType, 0,0,0,0, ExtCreatePenEnter, "gdi32.dll", "ExtCreatePen" ,0 },

	{ GdiCallbackType, 0,0,0,0, CreateBrushIndirectEnter, "gdi32.dll", "CreateBrushIndirect" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateSolidBrushEnter, "gdi32.dll", "CreateSolidBrush" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreatePatternBrushEnter, "gdi32.dll", "CreatePatternBrush" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateDIBPatternBrushEnter, "gdi32.dll", "CreateDIBPatternBrush" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateHatchBrushEnter, "gdi32.dll", "CreateHatchBrush" ,0 },

	{ GdiCallbackType, 0,0,0,0, CreateBtrftonePaletteEnter, "gdi32.dll", "CreateBtrftonePalette" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreatePaletteEnter, "gdi32.dll", "CreatePalette" ,0 },

	{ GdiCallbackType, 0,0,0,0, CreateFontAEnter, "gdi32.dll", "CreateFontA" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateFontWEnter, "gdi32.dll", "CreateFontW" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateFontIndirectAEnter, "gdi32.dll", "CreateFontIndirectA" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateFontIndirectWEnter, "gdi32.dll", "CreateFontIndirectW" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateFontIndirectExAEnter, "gdi32.dll", "CreateFontIndirectExA" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateFontIndirectExWEnter, "gdi32.dll", "CreateFontIndirectExW" ,0 },

	{ GdiCallbackType, 0,0,0,0, LoadBitmapAEnter, "user32.dll", "LoadBitmapA" ,0 },
	{ GdiCallbackType, 0,0,0,0, LoadBitmapWEnter, "user32.dll", "LoadBitmapW" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateBitmapEnter, "gdi32.dll", "CreateBitmap" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateBitmapIndirectEnter, "gdi32.dll", "CreateBitmapIndirect" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateCompatibleBitmapEnter, "gdi32.dll", "CreateCompatibleBitmap" ,0 },
	{ GdiCallbackType, 0,0,0,0, LoadImageAEnter, "user32.dll", "LoadImageA" ,0 },
	{ GdiCallbackType, 0,0,0,0, LoadImageWEnter, "user32.dll", "LoadImageW" ,0 },

	{ GdiCallbackType, 0,0,0,0, PathToRegionEnter, "gdi32.dll", "PathToRegion" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateEllipticRgnEnter, "gdi32.dll", "CreateEllipticRgn" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateEllipticRgnIndirectEnter, "gdi32.dll", "CreateEllipticRgnIndirect" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreatePolygonRgnEnter, "gdi32.dll", "CreatePolygonRgn" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreatePolyPolygonRgnEnter, "gdi32.dll", "CreatePolyPolygonRgn" ,0 },

	{ GdiCallbackType, 0,0,0,0, CreateRectRgnEnter, "gdi32.dll", "CreateRectRgn" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateRectRgnIndirectEnter, "gdi32.dll", "CreateRectRgnIndirect" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateRoundRectRgnEnter, "gdi32.dll", "CreateRoundRectRgn" ,0 },
	{ GdiCallbackType, 0,0,0,0, ExtCreateRegionEnter, "gdi32.dll", "ExtCreateRegion" ,0 },

	{ GdiCallbackType, 0,0,0,0, CreateEnhMetaFileAEnter, "gdi32.dll", "CreateEnhMetaFileA" ,0 },
	{ GdiCallbackType, 0,0,0,0, CreateEnhMetaFileWEnter, "gdi32.dll", "CreateEnhMetaFileW" ,0 },
	{ GdiCallbackType, 0,0,0,0, GetEnhMetaFileAEnter, "gdi32.dll", "GetEnhMetaFileA" ,0 },
	{ GdiCallbackType, 0,0,0,0, GetEnhMetaFileWEnter, "gdi32.dll", "GetEnhMetaFileW" ,0 },

};

ULONG MmGdiCallbackCount = ARRAYSIZE(MmGdiCallback);

PBTR_CALLBACK FORCEINLINE 
MmGetGdiCallback(
	IN ULONG Ordinal
	)
{
	return &MmGdiCallback[Ordinal];
}

BOOL WINAPI 
DeleteObjectEnter(
	HGDIOBJ ho
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	DELETEOBJECT CallbackPtr;
	PVOID Caller;
	BOOL Status;

	Caller = _ReturnAddress();
	Callback = MmGetGdiCallback(_DeleteObject);
	CallbackPtr = (DELETEOBJECT)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(ho);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(ho);
	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	MmRemoveGdiRecord(ho);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

BOOL WINAPI
DeleteDCEnter(
	HDC hdc  
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	DELETEDC CallbackPtr;
	PVOID Caller;
	BOOL Status;

	Caller = _ReturnAddress();
	Callback = MmGetGdiCallback(_DeleteDC);
	CallbackPtr = (DELETEDC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hdc);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(hdc);
	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	MmRemoveGdiRecord(hdc);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

int WINAPI
ReleaseDCEnter(
	HWND hWnd,
	HDC hDC  
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	RELEASEDC CallbackPtr;
	PVOID Caller;
	int Status;

	Caller = _ReturnAddress();
	Callback = MmGetGdiCallback(_ReleaseDC);
	CallbackPtr = (RELEASEDC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hWnd, hDC);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(hWnd, hDC);
	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	MmRemoveGdiRecord(hDC);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

BOOL WINAPI
DeleteEnhMetaFileEnter(
	HENHMETAFILE hemf   
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	DELETEENHMETAFILE CallbackPtr;
	PVOID Caller;
	BOOL Status;

	Caller = _ReturnAddress();
	Callback = MmGetGdiCallback(_DeleteEnhMetaFile);
	CallbackPtr = (DELETEENHMETAFILE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Status = (*CallbackPtr)(hemf);
		InterlockedDecrement(&Callback->References);
		return Status;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	Status = (*CallbackPtr)(hemf);
	if (!Status) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Status;
	}

	MmRemoveGdiRecord(hemf);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Status;
}

HENHMETAFILE WINAPI 
CloseEnhMetaFileEnter(
	HDC hdc 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CLOSEENHMETAFILE CallbackPtr;
	PVOID Frame;
	PVOID Caller;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	HANDLE Handle;
	ULONG Duration;

	Caller = _ReturnAddress();
	Callback = MmGetGdiCallback(_CloseEnhMetaFile);
	CallbackPtr = (CLOSEENHMETAFILE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall((ULONG_PTR)Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hdc);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(hdc);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return Handle;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;

	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	if (hdc) {
		MmRemoveGdiRecord(hdc);
	}

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_ENHMETAFILE, Duration, Hash, 
		               (USHORT)Depth, _CloseEnhMetaFile);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HDC WINAPI
CreateCompatibleDCEnter(
	HDC hdc   
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATECOMPATIBLEDC CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HDC Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateCompatibleDC);
	CallbackPtr = (CREATECOMPATIBLEDC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hdc);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);

	Handle = (*CallbackPtr)(hdc);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_MEMDC, Duration, Hash, 
		               (USHORT)Depth, _CreateCompatibleDC);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HDC WINAPI 
CreateDCAEnter(
	PSTR lpszDriver,        
	PSTR lpszDevice,        
	PSTR lpszOutput,       
	CONST DEVMODE* lpInitData  
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEDC_A CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HDC Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateDCA);
	CallbackPtr = (CREATEDC_A)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpszDriver, lpszDevice, lpszOutput, lpInitData);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lpszDriver, lpszDevice, lpszOutput, lpInitData);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_DC, Duration, Hash, 
		               (USHORT)Depth, _CreateDCA);


	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HDC WINAPI 
CreateDCWEnter(
	PWSTR lpszDriver,        
	PWSTR lpszDevice,        
	PWSTR lpszOutput,       
	CONST DEVMODE* lpInitData  
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEDC_W CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HDC Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateDCW);
	CallbackPtr = (CREATEDC_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpszDriver, lpszDevice, lpszOutput, lpInitData);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lpszDriver, lpszDevice, lpszOutput, lpInitData);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_DC, Duration, Hash, 
		               (USHORT)Depth, _CreateDCW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HDC WINAPI 
CreateICAEnter(
	PSTR lpszDriver,        
	PSTR lpszDevice,        
	PSTR lpszOutput,        
	CONST DEVMODE* lpdvmInit 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEIC_A CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HDC Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateICA);
	CallbackPtr = (CREATEIC_A)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpszDriver, lpszDevice, lpszOutput, lpdvmInit);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lpszDriver, lpszDevice, lpszOutput, lpdvmInit);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_DC, Duration, Hash, 
		               (USHORT)Depth, _CreateICA);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HDC WINAPI 
CreateICWEnter(
	PWSTR lpszDriver,      
	PWSTR lpszDevice,      
	PWSTR lpszOutput,        
	CONST DEVMODE* lpdvmInit  
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEIC_W CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HDC Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateICW);
	CallbackPtr = (CREATEIC_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpszDriver, lpszDevice, lpszOutput, lpdvmInit);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lpszDriver, lpszDevice, lpszOutput, lpdvmInit);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_DC, Duration, Hash, 
		               (USHORT)Depth, _CreateICW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HDC WINAPI 
GetDCEnter(
	HWND hWnd   
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	GETDC CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HDC Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;
	
	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_GetDC);
	CallbackPtr = (GETDC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hWnd);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(hWnd);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_DC, Duration, Hash, 
		               (USHORT)Depth, _GetDC);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HDC WINAPI 
GetDCExEnter(
	HWND hWnd,      
	HRGN hrgnClip, 
	DWORD flags     
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	GETDCEX CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HDC Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_GetDCEx);
	CallbackPtr = (GETDCEX)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hWnd, hrgnClip, flags);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(hWnd, hrgnClip, flags);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_DC, Duration, Hash, 
		               (USHORT)Depth, _GetDCEx);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HDC WINAPI 
GetWindowDCEnter(
	HWND hWnd   
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	GETWINDOWDC CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HDC Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_GetWindowDC);
	CallbackPtr = (GETDC)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hWnd);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(hWnd);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_DC, Duration, Hash, 
		               (USHORT)Depth, _GetWindowDC);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HPEN WINAPI
CreatePenEnter(
	int fnPenStyle,  
	int nWidth,     
	COLORREF crColor 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEPEN CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HPEN Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreatePen);
	CallbackPtr = (CREATEPEN)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(fnPenStyle, nWidth, crColor);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(fnPenStyle, nWidth, crColor);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_PEN, Duration, Hash, 
		               (USHORT)Depth, _CreatePen);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HPEN WINAPI 
CreatePenIndirectEnter(
	CONST LOGPEN *lplgpn 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEPENINDIRECT CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HPEN Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreatePenIndirect);
	CallbackPtr = (CREATEPENINDIRECT)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lplgpn);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lplgpn);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_PEN, Duration, Hash, 
		               (USHORT)Depth, _CreatePenIndirect);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HPEN WINAPI 
ExtCreatePenEnter(
	DWORD dwPenStyle, 
	DWORD dwWidth,   
	CONST LOGBRUSH *lplb, 
	DWORD dwStyleCount,  
	CONST DWORD *lpStyle
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	EXTCREATEPEN CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HPEN Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_ExtCreatePen);
	CallbackPtr = (EXTCREATEPEN)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(dwPenStyle, dwWidth, lplb, dwStyleCount, lpStyle);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(dwPenStyle, dwWidth, lplb, dwStyleCount, lpStyle);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 
	
	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_PEN, Duration, Hash, 
		               (USHORT)Depth, _ExtCreatePen);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HBRUSH WINAPI 
CreateBrushIndirectEnter(
	CONST LOGBRUSH *lplb 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEBRUSHINDIRECT CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HBRUSH Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateBrushIndirect);
	CallbackPtr = (CREATEBRUSHINDIRECT)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lplb);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lplb);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 
	
	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_BRUSH, Duration, Hash, 
		               (USHORT)Depth, _CreateBrushIndirect);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HBRUSH WINAPI 
CreateSolidBrushEnter(
	COLORREF crColor
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATESOLIDBRUSH CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HBRUSH Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateSolidBrush);
	CallbackPtr = (CREATESOLIDBRUSH)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(crColor);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(crColor);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_BRUSH, Duration, Hash, 
		               (USHORT)Depth, _CreateSolidBrush);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HBRUSH WINAPI
CreatePatternBrushEnter(
	HBITMAP hbmp  
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEPATTERNBRUSH CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HBRUSH Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreatePatternBrush);
	CallbackPtr = (CREATEPATTERNBRUSH)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hbmp);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(hbmp);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_BRUSH, Duration, Hash, 
		               (USHORT)Depth, _CreatePatternBrush);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HBRUSH WINAPI
CreateDIBPatternBrushEnter(
	HGLOBAL hglbDIBPacked, 
	UINT fuColorSpec
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEDIBPATTERNBRUSH CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HBRUSH Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateDIBPatternBrush);
	CallbackPtr = (CREATEDIBPATTERNBRUSH)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hglbDIBPacked, fuColorSpec);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(hglbDIBPacked, fuColorSpec);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_BRUSH, Duration, Hash, 
		               (USHORT)Depth, _CreateDIBPatternBrush);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HBRUSH WINAPI
CreateHatchBrushEnter(
	int fnStyle,      
	COLORREF clrref  
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEHATCHBRUSH CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HBRUSH Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateHatchBrush);
	CallbackPtr = (CREATEHATCHBRUSH)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(fnStyle, clrref);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(fnStyle, clrref);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_BRUSH, Duration, Hash, 
		               (USHORT)Depth, _CreateHatchBrush);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HPALETTE WINAPI
CreateBtrftonePaletteEnter(
	HDC hdc
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEHALFTONEPALETTE CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HPALETTE Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateBtrftonePalette);
	CallbackPtr = (CREATEHALFTONEPALETTE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hdc);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(hdc);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_PAL, Duration, Hash, 
		               (USHORT)Depth, _CreateBtrftonePalette);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HPALETTE WINAPI 
CreatePaletteEnter(
	CONST LOGPALETTE *lplgpl
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEPALETTE CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HPALETTE Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreatePalette);
	CallbackPtr = (CREATEPALETTE)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lplgpl);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lplgpl);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_PAL, Duration, Hash, 
		               (USHORT)Depth, _CreatePalette);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

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
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEFONT_A CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HFONT Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateFontA);
	CallbackPtr = (CREATEFONT_A)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(nHeight, nWidth, nEscapement, nOrientation,
			                    fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut,
								fdwCharSet, fdwOutputPrecision, fdwClipPrecision,
								fdwQuality, fdwPitchAndFamily, lpszFace);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(nHeight, nWidth, nEscapement, nOrientation,
		                    fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut,
							fdwCharSet, fdwOutputPrecision, fdwClipPrecision,
							fdwQuality, fdwPitchAndFamily, lpszFace);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_FONT, Duration, Hash, 
		               (USHORT)Depth, _CreateFontA);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

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
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEFONT_W CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HFONT Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateFontW);
	CallbackPtr = (CREATEFONT_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(nHeight, nWidth, nEscapement, nOrientation,
			                    fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut,
								fdwCharSet, fdwOutputPrecision, fdwClipPrecision,
								fdwQuality, fdwPitchAndFamily, lpszFace);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(nHeight, nWidth, nEscapement, nOrientation,
		                    fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut,
							fdwCharSet, fdwOutputPrecision, fdwClipPrecision,
							fdwQuality, fdwPitchAndFamily, lpszFace);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_FONT, Duration, Hash, 
		               (USHORT)Depth, _CreateFontW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HFONT WINAPI 
CreateFontIndirectAEnter(
	CONST LOGFONT* lplf 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEFONTINDIRECT_A CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HFONT Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateFontIndirectA);
	CallbackPtr = (CREATEFONTINDIRECT_A)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lplf);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lplf);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_FONT, Duration, Hash, 
		               (USHORT)Depth, _CreateFontIndirectA);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HFONT WINAPI 
CreateFontIndirectWEnter(
	CONST LOGFONT* lplf 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEFONTINDIRECT_W CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HFONT Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateFontIndirectW);
	CallbackPtr = (CREATEFONTINDIRECT_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lplf);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lplf);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_FONT, Duration, Hash, 
		               (USHORT)Depth, _CreateFontIndirectW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HFONT WINAPI 
CreateFontIndirectExAEnter(
	CONST ENUMLOGFONTEXDV *penumlfex
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEFONTINDIRECTEX_A CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HFONT Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateFontIndirectExA);
	CallbackPtr = (CREATEFONTINDIRECTEX_A)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(penumlfex);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(penumlfex);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_FONT, Duration, Hash, 
		               (USHORT)Depth, _CreateFontIndirectExA);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HFONT WINAPI 
CreateFontIndirectExWEnter(
	CONST ENUMLOGFONTEXDV *penumlfex  
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEFONTINDIRECTEX_W CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HFONT Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateFontIndirectExW);
	CallbackPtr = (CREATEFONTINDIRECTEX_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(penumlfex);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(penumlfex);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_FONT, Duration, Hash, 
		               (USHORT)Depth, _CreateFontIndirectExW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HBITMAP WINAPI 
LoadBitmapAEnter(
	HINSTANCE hInstance,
	PSTR lpBitmapName 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	LOADBITMAP_A CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HBITMAP Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_LoadBitmapA);
	CallbackPtr = (LOADBITMAP_A)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hInstance, lpBitmapName);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(hInstance, lpBitmapName);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_BITMAP, Duration, Hash, 
		               (USHORT)Depth, _LoadBitmapA);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HBITMAP WINAPI 
LoadBitmapWEnter(
	HINSTANCE hInstance, 
	PWSTR lpBitmapName  
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	LOADBITMAP_W CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HBITMAP Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_LoadBitmapW);
	CallbackPtr = (LOADBITMAP_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hInstance, lpBitmapName);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(hInstance, lpBitmapName);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 
	
	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_BITMAP, Duration, Hash, 
		               (USHORT)Depth, _LoadBitmapW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HBITMAP WINAPI 
CreateBitmapEnter(
	int nWidth,         
	int nHeight,        
	UINT cPlanes,       
	UINT cBitsPerPel,   
	CONST VOID *lpvBits 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEBITMAP CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HBITMAP Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateBitmap);
	CallbackPtr = (CREATEBITMAP)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(nWidth, nHeight, cPlanes, cBitsPerPel, lpvBits);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(nWidth, nHeight, cPlanes, cBitsPerPel, lpvBits);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_BITMAP, Duration, Hash, 
		               (USHORT)Depth, _CreateBitmap);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HBITMAP WINAPI 
CreateBitmapIndirectEnter(
	CONST BITMAP *lpbm 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEBITMAPINDIRECT CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HBITMAP Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateBitmapIndirect);
	CallbackPtr = (CREATEBITMAPINDIRECT)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpbm);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lpbm);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_BITMAP, Duration, Hash, 
		               (USHORT)Depth, _CreateBitmapIndirect);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HBITMAP WINAPI 
CreateCompatibleBitmapEnter(
	HDC hdc,    
	int nWidth,  
	int nHeight 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATECOMPATIBLEBITMAP CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HBITMAP Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateCompatibleBitmap);
	CallbackPtr = (CREATECOMPATIBLEBITMAP)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hdc, nWidth, nHeight);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(hdc, nWidth, nHeight);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_BITMAP, Duration, Hash, 
		               (USHORT)Depth, _CreateCompatibleBitmap);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
LoadImageWEnter(
	HINSTANCE hinst,
    PWSTR lpszName,
    UINT uType,
    int cxDesired,
    int cyDesired,
    UINT fuLoad
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	LOADIMAGE_W CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HBITMAP Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_LoadImageW);
	CallbackPtr = (LOADIMAGE_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hinst, lpszName, uType, cxDesired, cyDesired, fuLoad);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(hinst, lpszName, uType, cxDesired, cyDesired, fuLoad);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_BITMAP, Duration, Hash, 
		               (USHORT)Depth, _LoadImageW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HANDLE WINAPI 
LoadImageAEnter(
	HINSTANCE hinst,
    PSTR lpszName,
    UINT uType,
    int cxDesired,
    int cyDesired,
    UINT fuLoad
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	LOADIMAGE_A CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HBITMAP Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_LoadImageA);
	CallbackPtr = (LOADIMAGE_A)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hinst, lpszName, uType, cxDesired, cyDesired, fuLoad);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(hinst, lpszName, uType, cxDesired, cyDesired, fuLoad);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_BITMAP, Duration, Hash, 
		               (USHORT)Depth, _LoadImageA);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HRGN WINAPI 
PathToRegionEnter(
	HDC hdc
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	PATHTOREGION CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HRGN Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_PathToRegion);
	CallbackPtr = (PATHTOREGION)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hdc);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(hdc);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_REGION, Duration, Hash, 
		               (USHORT)Depth, _PathToRegion);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HRGN WINAPI
CreateEllipticRgnEnter(
	int nLeftRect,   
	int nTopRect,   
	int nRightRect,  
	int nBottomRect  
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEELLIPTICRGN CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HRGN Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateEllipticRgn);
	CallbackPtr = (CREATEELLIPTICRGN)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(nLeftRect, nTopRect, nRightRect, nBottomRect);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(nLeftRect, nTopRect, nRightRect, nBottomRect);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_REGION, Duration, Hash, 
		               (USHORT)Depth, _CreateEllipticRgn);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HRGN WINAPI 
CreateEllipticRgnIndirectEnter(
	CONST RECT *lprc
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEELLIPTICRGNINDIRECT CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HRGN Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateEllipticRgnIndirect);
	CallbackPtr = (CREATEELLIPTICRGNINDIRECT)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lprc);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lprc);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_REGION, Duration, Hash, 
		               (USHORT)Depth, _CreateEllipticRgnIndirect);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HRGN WINAPI 
CreatePolygonRgnEnter(
	CONST POINT *lppt,  
	int cPoints,        
	int fnPolyFillMode  
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEPOLYGONRGN CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HRGN Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreatePolygonRgn);
	CallbackPtr = (CREATEPOLYGONRGN)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lppt, cPoints, fnPolyFillMode);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lppt, cPoints, fnPolyFillMode);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_REGION, Duration, Hash, 
		               (USHORT)Depth, _CreatePolygonRgn);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}
    
HRGN WINAPI 
CreatePolyPolygonRgnEnter(
	CONST POINT *lppt,       
	CONST INT *lpPolyCounts,  
	int nCount,               
	int fnPolyFillMode      
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEPOLYPOLYGONRGN CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HRGN Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreatePolyPolygonRgn);
	CallbackPtr = (CREATEPOLYPOLYGONRGN)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lppt, lpPolyCounts, nCount, fnPolyFillMode);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lppt, lpPolyCounts, nCount, fnPolyFillMode);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_REGION, Duration, Hash, 
		               (USHORT)Depth, _CreatePolyPolygonRgn);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HRGN WINAPI 
CreateRectRgnEnter(
	int nLeftRect,   
	int nTopRect,    
	int nRightRect, 
	int nBottomRect
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATERECTRGN CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HRGN Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateRectRgn);
	CallbackPtr = (CREATERECTRGN)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(nLeftRect, nTopRect, nRightRect, nBottomRect);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(nLeftRect, nTopRect, nRightRect, nBottomRect);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 
	
	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_REGION, Duration, Hash, 
		               (USHORT)Depth, _CreateRectRgn);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HRGN WINAPI 
CreateRectRgnIndirectEnter(
	CONST RECT *lprc 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATERECTRGNINDIRECT CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HRGN Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateRectRgnIndirect);
	CallbackPtr = (CREATERECTRGNINDIRECT)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lprc);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lprc);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_REGION, Duration, Hash, 
		               (USHORT)Depth, _CreateRectRgnIndirect);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HRGN WINAPI 
CreateRoundRectRgnEnter(
	int nLeftRect,      
	int nTopRect,       
	int nRightRect,    
	int nBottomRect,    
	int nWidthEllipse,  
	int nHeightEllipse  
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEROUNDRECTRGN CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HRGN Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateRoundRectRgn);
	CallbackPtr = (CREATEROUNDRECTRGN)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(nLeftRect, nTopRect, nRightRect, nBottomRect, 
			                    nWidthEllipse, nHeightEllipse);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(nLeftRect, nTopRect, nRightRect, nBottomRect, 
	                        nWidthEllipse, nHeightEllipse);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_REGION, Duration, Hash, 
		               (USHORT)Depth, _CreateRoundRectRgn);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HRGN WINAPI 
ExtCreateRegionEnter(
	CONST XFORM *lpXform,     
	DWORD nCount,             
	CONST RGNDATA *lpRgnData 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	EXTCREATEREGION CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HRGN Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_ExtCreateRegion);
	CallbackPtr = (EXTCREATEREGION)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpXform, nCount, lpRgnData);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lpXform, nCount, lpRgnData);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_REGION, Duration, Hash, 
		               (USHORT)Depth, _ExtCreateRegion);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}
   
HDC WINAPI 
CreateEnhMetaFileAEnter(
	HDC hdcRef, 
	PSTR lpFilename, 
	CONST RECT* lpRect, 
	PSTR lpDescription  
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEENHMETAFILE_A CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HDC Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateEnhMetaFileA);
	CallbackPtr = (CREATEENHMETAFILE_A)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hdcRef, lpFilename, lpRect, lpDescription);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(hdcRef, lpFilename, lpRect, lpDescription);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_ENHMETADC, Duration, Hash, 
		               (USHORT)Depth, _CreateEnhMetaFileA);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}

HDC WINAPI 
CreateEnhMetaFileWEnter(
	HDC hdcRef,       
	PWSTR lpFilename, 
	CONST RECT* lpRect, 
	PWSTR lpDescription 
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	CREATEENHMETAFILE_W CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HDC Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_CreateEnhMetaFileW);
	CallbackPtr = (CREATEENHMETAFILE_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(hdcRef, lpFilename, lpRect, lpDescription);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(hdcRef, lpFilename, lpRect, lpDescription);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_ENHMETADC, Duration, Hash, 
		               (USHORT)Depth, _CreateEnhMetaFileW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}
   
HENHMETAFILE WINAPI 
GetEnhMetaFileAEnter(
	PSTR lpszMetaFile
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	GETENHMETAFILE_A CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HENHMETAFILE Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_GetEnhMetaFileA);
	CallbackPtr = (GETENHMETAFILE_A)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpszMetaFile);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lpszMetaFile);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_ENHMETAFILE, Duration, Hash, 
		               (USHORT)Depth, _GetEnhMetaFileA);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}
    
HENHMETAFILE WINAPI
GetEnhMetaFileWEnter(
	PWSTR lpszMetaFile
	)
{
	PBTR_THREAD_OBJECT Thread;
	PBTR_CALLBACK Callback;
	ULONG Hash, Depth;
	PVOID Callers[MAX_STACK_DEPTH];
	GETENHMETAFILE_W CallbackPtr;
	PVOID Frame;
	ULONG_PTR Caller;
	HENHMETAFILE Handle;
	LARGE_INTEGER Enter;
	LARGE_INTEGER Exit;
	ULONG Duration;

	Caller = (ULONG_PTR)_ReturnAddress();
	Callback = MmGetGdiCallback(_GetEnhMetaFileW);
	CallbackPtr = (GETENHMETAFILE_W)BtrGetCallbackDestine(Callback);

	Thread = BtrIsExemptedCall(Caller);
	if (!Thread) {
		InterlockedIncrement(&Callback->References);
		Handle = (*CallbackPtr)(lpszMetaFile);
		InterlockedDecrement(&Callback->References);
		return Handle;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Callback->References);

	QueryPerformanceCounter(&Enter);
	
	Handle = (*CallbackPtr)(lpszMetaFile);
	if (!Handle) {
		InterlockedDecrement(&Callback->References);
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		return NULL;
	}

	QueryPerformanceCounter(&Exit);

	Callers[0] = (PVOID)Caller;
	Callers[1] = 0;
	Callers[2] = 0;
	Frame = BtrGetFramePointer();
	BtrCaptureStackTraceEx(Callers, MAX_STACK_DEPTH, Frame, 
		                   Callback->Address, &Hash, &Depth); 

	Duration = (ULONG)(Exit.QuadPart - Enter.QuadPart);
	MmInsertGdiRecord(Handle, OBJ_ENHMETAFILE, Duration, Hash, 
		               (USHORT)Depth, _GetEnhMetaFileW);

	InterlockedDecrement(&Callback->References);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	return Handle;
}
  
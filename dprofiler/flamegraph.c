//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2013
// 

#include "flamegraph.h"
#include "calltree.h"
#include <math.h>

#define SDK_FLAME_CLASS   L"SdkFlame"

BOOLEAN FlameRegistered = FALSE;

BOOLEAN
FlameInitialize(
	VOID
	)
{
   WNDCLASSEX wc;
	ATOM Atom;

	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(wc);

	SdkMainIcon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_SMALL));
    
	wc.hbrBackground  = GetSysColorBrush(COLOR_BTNFACE);
    wc.hCursor        = LoadCursor(0, IDC_ARROW);
    wc.hIcon          = SdkMainIcon;
    wc.hIconSm        = SdkMainIcon;
    wc.hInstance      = SdkInstance;
    wc.lpfnWndProc    = FlameProcedure;
    wc.lpszClassName  = SDK_FLAME_CLASS;

    Atom = RegisterClassEx(&wc);
	ASSERT(Atom != 0);

	if (Atom) {
		FlameRegistered = TRUE;
	}

	return Atom ? TRUE : FALSE;
}

PFLAME_NODE FORCEINLINE 
FlameAllocateNode(VOID)
{
	return (PFLAME_NODE)ApsMalloc(sizeof(FLAME_NODE));
}

VOID
FlameInitializeStack(
	__in PFLAME_CONTROL Object
	)
{
	ULONG Number;
	PFLAME_STACK Stack;

	Stack = (PFLAME_STACK)ApsMalloc(sizeof(FLAME_STACK));
	for(Number = 0; Number < MAX_STACK_DEPTH; Number += 1) {
		InitializeListHead(&Stack->Level[Number].ListHead);
		Stack->Level[Number].Count = 0;
	}

	Stack->Depth = 0;
	Object->Flame = Stack;
}

VOID
FlameFreeStack(
	__in PFLAME_CONTROL Object
	)
{
	ULONG Number;
	PFLAME_STACK Stack;
	PLIST_ENTRY ListEntry;
	PFLAME_NODE FlameNode;
	PCALL_NODE CallNode;

	Stack = Object->Flame;
	ASSERT(Stack != NULL);

	for(Number = 0; Number < Stack->Depth; Number += 1) {

		while (IsListEmpty(&Stack->Level[Number].ListHead)) {

			ListEntry = RemoveHeadList(&Stack->Level[Number].ListHead);
			FlameNode = CONTAINING_RECORD(ListEntry, FLAME_NODE, ListEntry);

			//
			// Detach from call node
			//

			CallNode = (PCALL_NODE)FlameNode->Node;
			ASSERT(CallNode != NULL);
			CallNode->Context = NULL;

			ApsFree(FlameNode);
		}

	}

	ApsFree(Stack);
	Object->Flame = NULL;
}

VOID
FlameBuildFlameNode(
    __in PFLAME_CONTROL Object,
    __in BTR_PROFILE_TYPE Type,
    __in PFLAME_NODE ParentFlame,
	__in ULONG Level
    )
{
    PCALL_NODE ParentNode;
    PCALL_NODE ChildNode;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
	PFLAME_NODE FlameNode;
	PFLAME_STACK Stack;
	double Inclusive;
	double Value;
	LONG Left, Width, NodeWidth;

	ParentNode = ParentFlame->Node;
	Stack = Object->Flame;

    //
    // Compute the bounding rect of the flame node
    //

	Left = ParentFlame->Rect.left;
	Width = ParentFlame->Rect.right - ParentFlame->Rect.left;
	ASSERT(Width > 0);

	if (Width < (LONG)Object->MinimumWidth)
		return;

    if (Type == PROFILE_CPU_TYPE) {
		Inclusive = ParentNode->Cpu.Inclusive * 1.0;
    }
    else if (Type == PROFILE_MM_TYPE) {
		Inclusive = ParentNode->Mm.InclusiveBytes * 1.0;
    }

	ListHead = &ParentNode->ChildListHead;
    ListEntry = ListHead->Flink;
    
    while (ListEntry != ListHead) {

        ChildNode = CONTAINING_RECORD(ListEntry, CALL_NODE, ListEntry);

		//
		// Compute flame node width by percent
		//

        if (Type == PROFILE_CPU_TYPE) {
			Value = ChildNode->Cpu.Inclusive * 1.0;
        }
        else if (Type == PROFILE_MM_TYPE) {
            Value = ChildNode->Mm.InclusiveBytes * 1.0;
        }
        else {
            ASSERT(0);
        }

        NodeWidth = (ULONG)floor((Value / Inclusive) * Width);
		NodeWidth = max(NodeWidth, (LONG)Object->NodeHoriEdge);
         
        //
        // Attach the flame node to call node
        //

		FlameNode = FlameAllocateNode();
		FlameGenerateFillColor(&FlameNode->Color);
		FlameNode->Level = Level;
		FlameNode->Rect.left = Left;
		FlameNode->Rect.right = Left + NodeWidth;
		FlameNode->Node = ChildNode;
        ChildNode->Context = FlameNode;

		//
		// Insert the flame node into flame stack
		//

		InsertTailList(&Stack->Level[Level].ListHead, &FlameNode->ListEntry);
		Stack->Level[Level].Count += 1;

		//
		// Adjust computation base and x position of next sibiling node
		//

        //
        // Recursively build flame nodes
        //

		Stack->Depth = max(Stack->Depth, Level + 1);
		if (!IsListEmpty(&ChildNode->ChildListHead)) {
	        FlameBuildFlameNode(Object, Type, FlameNode, Level + 1);
		}

        ListEntry = ListEntry->Flink;
		
		//
		// Ensure the left with is enough to draw a rect
		//

		ASSERT(Inclusive >= Value);
		Inclusive = Inclusive - Value;
		Width = Width - NodeWidth - Object->NodeHoriEdge; 

		if (Width < (LONG)Object->MinimumWidth) {
			return;
		}

		Left = FlameNode->Rect.right + Object->NodeHoriEdge;

    }
}

VOID
FlameBuildGraph(
	__in PFLAME_CONTROL Object
	)
{
	PCALL_GRAPH Graph;
    PCALL_NODE Node;
    PCALL_TREE Tree;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
	HDC hdc;
	PFLAME_NODE FlameNode;
	double Inclusive;
	double Value;
	PFLAME_STACK Stack;
	ULONG Level;
	ULONG Left, Width, NodeWidth;

    Graph = Object->Graph;
	ASSERT(Graph != NULL);

	//
	// Compute number of rows to insert listview
	// NumberOfRows = MaximumStackDepth + TopSpaceRows + OneBottomTipRow
	//

	//
	// Compute DrawText() limit, any rect width smaller than EmptyLimit is ignored
	//

	hdc = GetDC(Object->hWnd);
	GetTextExtentPoint32(hdc, L"MM", sizeof(L"MM"), &Object->EmptyLimit);
	ReleaseDC(Object->hWnd, hdc);

    //
    // Compute the coordination attach to each node 
    //

	Left = 0;
	Width = Object->ViewHoriWidth;

    if (Graph->Type == PROFILE_CPU_TYPE) {
        Inclusive = Graph->Inclusive * 1.0;
    }
    else if (Graph->Type == PROFILE_MM_TYPE) {
        Inclusive = Graph->InclusiveBytes * 1.0;
    }

	//
	// N.B. The stack's count may be smaller than maximum depth 
	// of call graph, we will adjust it later
	//

	FlameInitializeStack(Object);

	Stack = Object->Flame;
	Stack->Depth = 1;
	Level = 0;

    ListHead = &Graph->TreeListHead;
    ListEntry = ListHead->Flink;
    
    while (ListEntry != ListHead) {

        Tree = CONTAINING_RECORD(ListEntry, CALL_TREE, ListEntry);
        Node = Tree->RootNode;

		//
		// Compute flame node width by percent
		//

        if (Graph->Type == PROFILE_CPU_TYPE) {
			Value = Node->Cpu.Inclusive * 1.0;
        }
        else if (Graph->Type == PROFILE_MM_TYPE) {
            Value = Node->Mm.InclusiveBytes * 1.0;
        }
        else {
            ASSERT(0);
        }

        NodeWidth = (ULONG)floor((Value / Inclusive) * Width);
		NodeWidth = max(NodeWidth, (ULONG)Object->NodeHoriEdge);
         
        //
        // Attach the flame node to call node
        //

		FlameNode = FlameAllocateNode();
		FlameGenerateFillColor(&FlameNode->Color);
		FlameNode->Rect.left = Left;
		FlameNode->Rect.right = Left + NodeWidth;
		FlameNode->Node = Node;
        Node->Context = FlameNode;

		//
		// Insert the flame node into flame stack
		//

		InsertTailList(&Stack->Level[Level].ListHead, &FlameNode->ListEntry);
		Stack->Level[Level].Count += 1;

		//
		// Adjust computution base and x position of next sibiling node
		//

        //
        // Recursively build flame nodes
        //

		if (!IsListEmpty(&Node->ChildListHead)) {
			Stack->Depth = max(Stack->Depth, Level + 1);
	        FlameBuildFlameNode(Object, Graph->Type, FlameNode, Level + 1);
		}

        ListEntry = ListEntry->Flink;
		
		//
		// Ensure the left with is enough to draw a rect
		//
		
		ASSERT(Inclusive >= Value);
		Inclusive = Inclusive - Value;


		Width = Width - NodeWidth - Object->NodeHoriEdge; 
		if (Width < (ULONG)Object->MinimumWidth) {
			break;
		}

		Left = FlameNode->Rect.right + Object->NodeHoriEdge;
    }

	FlameDrawNodes(Object);
	FlameCreateTooltip(Object);
}

HWND
FlameCreateTooltip(
	__in PFLAME_CONTROL Object
	)
{
    HWND hWnd;
    TOOLINFO ToolInfo;
	   
    hWnd = CreateWindow(TOOLTIPS_CLASS, TEXT(""),
                        WS_POPUP,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL, (HMENU)NULL, SdkInstance,
                        NULL);

    ToolInfo.cbSize = sizeof(TOOLINFO);
    ToolInfo.uFlags = TTF_TRACK|TTF_ABSOLUTE;
	ToolInfo.hwnd = Object->hWnd;
    ToolInfo.uId = 0;
    ToolInfo.hinst = SdkInstance;
    ToolInfo.lpszText = LPSTR_TEXTCALLBACK;

    ToolInfo.rect.left = 0;
    ToolInfo.rect.top = 0;
    ToolInfo.rect.right = 0;
    ToolInfo.rect.bottom = 0;

    SendMessage(hWnd, TTM_ADDTOOL, 0, (LPARAM)&ToolInfo);

	Object->IsTooltipActivated = FALSE;
	Object->hWndTooltip = hWnd;
    return hWnd;    
}

BOOLEAN
FlameActivateTooltip(
	__in PFLAME_CONTROL Object
	)
{
    TOOLINFO ToolInfo;
	   
    ToolInfo.cbSize = sizeof(TOOLINFO);
    ToolInfo.uFlags = TTF_TRACK|TTF_ABSOLUTE;
	ToolInfo.hwnd = Object->hWnd;
    ToolInfo.uId = 0;
    ToolInfo.hinst = SdkInstance;
    ToolInfo.lpszText = LPSTR_TEXTCALLBACK;

    ToolInfo.rect.left = 0;
    ToolInfo.rect.top = 0;
    ToolInfo.rect.right = 0;
    ToolInfo.rect.bottom = 0;

    SendMessage(Object->hWndTooltip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ToolInfo);
	Object->IsTooltipActivated = TRUE;
    return TRUE;    
}

BOOLEAN
FlameDeactivateTooltip(
	__in PFLAME_CONTROL Object
	)
{
    TOOLINFO ToolInfo;

    ToolInfo.cbSize = sizeof(TOOLINFO);
    ToolInfo.uFlags = TTF_TRACK|TTF_ABSOLUTE;
	ToolInfo.hwnd = Object->hWnd;
    ToolInfo.uId = 0;
    ToolInfo.hinst = SdkInstance;
    ToolInfo.lpszText = LPSTR_TEXTCALLBACK;
    ToolInfo.rect.left = 0;
    ToolInfo.rect.top = 0;
    ToolInfo.rect.right = 0;
    ToolInfo.rect.bottom = 0;

    SendMessage(Object->hWndTooltip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&ToolInfo);
    Object->IsTooltipActivated = FALSE;
    return TRUE;
}

PFLAME_CONTROL
FlameCreateControl(
    __in HWND hWndParent,
    __in INT_PTR CtrlId,
	__in LPRECT Rect,
	__in COLORREF crFirst,
	__in COLORREF crLast,
	__in COLORREF crBack,
	__in COLORREF crEdge
    )
{
    HWND hWnd;
    PFLAME_CONTROL Object;

    Object = (PFLAME_CONTROL)SdkMalloc(sizeof(FLAME_CONTROL));
    ZeroMemory(Object, sizeof(FLAME_CONTROL));

    Object->CtrlId = CtrlId;
    Object->Width = 0;
    Object->Height = 0;
    Object->Linedx = 0;
    Object->Linedy = 0;
    Object->BackColor = crBack;
    Object->FirstColor = crFirst;
    Object->LastColor = crLast;
    Object->EdgeColor = crEdge;
    Object->HoriMargin = 1;
    Object->VertMargin = 1;
    Object->bmpRect.top = 0;
    Object->bmpRect.left = 0;
    Object->bmpRect.right = 1024;
    Object->bmpRect.bottom = 1024;

	Object->ViewHoriWidth = 1024;
	Object->ViewHoriEdge = 2;
	Object->ViewVertEdge = 2;
	Object->NodeHoriEdge = 1;
	Object->MinimumWidth = 3; // minimum width to draw a rect
	Object->TitleHeight = 0;

    hWnd = CreateWindowEx(0, SDK_FLAME_CLASS, L"", 
                          WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL, 
                          Rect->left, Rect->top, 
                          Rect->right - Rect->left, 
                          Rect->bottom - Rect->top, 
                          hWndParent, (HMENU)CtrlId, 
                          SdkInstance, Object);
    return Object;
}

PFLAME_CONTROL
FlameInitializeControl(
    __in HWND hWnd,
    __in INT_PTR CtrlId,
    __in COLORREF crFirst,
	__in COLORREF crLast,
	__in COLORREF crBack,
	__in COLORREF crEdge
    )
{
    PFLAME_CONTROL Object;

    Object = (PFLAME_CONTROL)SdkMalloc(sizeof(FLAME_CONTROL));
    ZeroMemory(Object, sizeof(FLAME_CONTROL));

    Object->CtrlId = CtrlId;
    Object->Width = 0;
    Object->Height = 0;
    Object->Linedx = 0;
    Object->Linedy = 0;
    Object->BackColor = crBack;
    Object->FirstColor = crFirst;
    Object->LastColor = crLast;
    Object->EdgeColor = crEdge;
    Object->HoriMargin = 1;
    Object->VertMargin = 1;
    Object->bmpRect.top = 0;
    Object->bmpRect.left = 0;
    Object->bmpRect.right = 1024;
    Object->bmpRect.bottom = 1024;

	Object->ViewHoriWidth = 1024;
	Object->ViewHoriEdge = 2;
	Object->ViewVertEdge = 2;
	Object->NodeHoriEdge = 1;
	Object->MinimumWidth = 3; // minimum width to draw a rect
	Object->TitleHeight = 0;

    SdkModifyStyle(hWnd, 0, WS_CLIPSIBLINGS|WS_VSCROLL|WS_HSCROLL|WS_BORDER, FALSE);

    Object->hWnd = hWnd;
    SdkSetObject(hWnd, Object);

    return Object;
}

LRESULT CALLBACK 
FlameProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
    switch (uMsg) {

    case WM_CREATE:
        FlameOnCreate(hWnd, uMsg, wp, lp);
        break;

    case WM_DESTROY:
        FlameOnDestroy(hWnd, uMsg, wp, lp);
        break;

    case WM_SIZE:
        FlameOnSize(hWnd, uMsg, wp, lp);
        break;

	case WM_MOUSEMOVE:
		FlameOnMouseMove(hWnd, uMsg, wp, lp);
		break;

	case WM_NOTIFY:
		return FlameOnNotify(hWnd, uMsg, wp, lp);

    case WM_PAINT:
        FlameOnPaint(hWnd, uMsg, wp, lp); 
		break;

    case WM_HSCROLL:
    case WM_VSCROLL:
        FlameOnScroll(hWnd, uMsg, wp, lp);
        break;

    case WM_ERASEBKGND:
        return FALSE;
    }

    return DefWindowProc(hWnd, uMsg, wp, lp);
}

LRESULT
FlameOnCreate(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
    PFLAME_CONTROL Object;
    LPCREATESTRUCT lpCreate;

    lpCreate = (LPCREATESTRUCT)lp;
    Object = (PFLAME_CONTROL)lpCreate->lpCreateParams;

    if (Object != NULL) {
        Object->hWnd = hWnd;
        SdkSetObject(hWnd, Object);
    }

    return 0L;
}

LRESULT
FlameOnDestroy(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
    PFLAME_CONTROL Object;

    Object = (PFLAME_CONTROL)SdkGetObject(hWnd);
    ASSERT(Object != NULL);

    if (Object->hbmpFlame) {
        DeleteObject(Object->hbmpFlame);
    }

    if (Object->hdcFlame) {
        DeleteDC(Object->hdcFlame);
    }

	if (Object->hNormalFont) {
		DeleteObject(Object->hNormalFont);
	}

	if (Object->hBoldFont) {
		DeleteObject(Object->hBoldFont);
	}

	FlameFreeStack(Object);

    SdkFree(Object);
    SdkSetObject(hWnd, NULL);
    return 0L;
}

VOID
FlameDrawFocusRect(
	__in PFLAME_CONTROL Object,
	__in PFLAME_NODE FlameNode
    )
{
    int x, y;
    RECT rc;
    HDC hdc;

    x = GetScrollPos(Object->hWnd, SB_HORZ);
    y = GetScrollPos(Object->hWnd, SB_VERT);

    rc = FlameNode->Rect;
    OffsetRect(&rc, -x, -y);

    hdc = GetDC(Object->hWnd);
    DrawEdge(hdc, &rc, EDGE_BUMP, BF_RECT);
    ReleaseDC(Object->hWnd, hdc);
}

VOID
FlameInvalidateRect(
	__in PFLAME_CONTROL Object,
	__in PFLAME_NODE FlameNode
    )
{
    int x, y;
    RECT rc;
    HDC hdc;

    x = GetScrollPos(Object->hWnd, SB_HORZ);
    y = GetScrollPos(Object->hWnd, SB_VERT);

    rc = FlameNode->Rect;
    OffsetRect(&rc, -x, -y);

    hdc = GetDC(Object->hWnd);
    InvalidateRect(Object->hWnd, &rc, FALSE);
	ReleaseDC(Object->hWnd, hdc);
}

VOID
FlameFillSolidRect(
    __in HWND hWnd,
    __in HDC hdc,
	__in COLORREF rgb,
	__in LPRECT rc 
    )
{
    HBRUSH hBrush;

    hBrush = CreateSolidBrush(rgb);
    FillRect(hdc, rc, hBrush);
    DeleteObject(hBrush);
}

LONG
FlameComputeLevel(
	__in PFLAME_CONTROL Object,
	__in LONG y
	)
{
	LONG Depth;
	LONG Step;

	if (y < Object->ValidRect.top || y > Object->ValidRect.bottom)
		return -1;

	Step = Object->FrameHeight + 1;
	y = Object->ValidRect.bottom - y;
	Depth = y / Step;
	return Depth;
}

PFLAME_NODE
FlameScanNode(
	__in PFLAME_LEVEL Level,
	__in LONG x 
	)
{
	PLIST_ENTRY ListEntry;
	PFLAME_NODE FlameNode;

	ListEntry = Level->ListHead.Flink;
	while (ListEntry != &Level->ListHead) {

		FlameNode = CONTAINING_RECORD(ListEntry, FLAME_NODE, ListEntry);
		if (FlameNode->Rect.left <= x && x <= FlameNode->Rect.right) {
			return FlameNode;
		}

		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

PFLAME_NODE
FlameHitTest(
	__in PFLAME_CONTROL Object,
	__in LPPOINT Point
	)
{
	PFLAME_STACK Stack;
	LONG Depth;
	PFLAME_NODE FlameNode;
	LONG x, y;

	//
	// Get the scroll delta
	//

	x = GetScrollPos(Object->hWnd, SB_HORZ);
    y = GetScrollPos(Object->hWnd, SB_VERT);

	//
	// Locate the veritical frame level
	//

	Stack = Object->Flame;
	Depth = FlameComputeLevel(Object, Point->y + y);
	if (Depth == -1 || Depth >= MAX_STACK_DEPTH) {
		return NULL;
	}

	//
	// Locate the horizonal rect
	//

	FlameNode = FlameScanNode(&Stack->Level[Depth], Point->x + x);
	return FlameNode;
}

LRESULT
FlameOnMouseMove(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
	PFLAME_CONTROL Object;
	PFLAME_NODE FlameNode;
	PFLAME_NODE OldNode;
	POINT Point;
	int x, y;

	Object = (PFLAME_CONTROL)SdkGetObject(hWnd);
	if (!Object) {
		return 0;
	}

	x = GET_X_LPARAM(lp);
	y = GET_Y_LPARAM(lp);

	Point.x = x;
	Point.y = y;
	FlameNode = FlameHitTest(Object, &Point);

	if (FlameNode != NULL) {

        OldNode = Object->MouseNode;
		Object->MouseNode = FlameNode;

        if (OldNode) {
            if (OldNode != FlameNode) {
                FlameInvalidateRect(Object, OldNode);
            }
        }
        else {
            FlameDrawFocusRect(Object, FlameNode);
        }

        if (!Object->IsTooltipActivated) {
            FlameActivateTooltip(Object);
        }

        //
        // WM_MOUSEMOVE fill us client coordinates we need map it screen coordinates
        // before send tooltip TTM_TRACKPOSITION
        //

        Point.x = x;
        Point.y = y;
        ClientToScreen(hWnd, &Point);

        x = Point.x;
        y = Point.y - 20;  // move up 1 row
		SendMessage(Object->hWndTooltip, TTM_TRACKPOSITION, 0, MAKELPARAM(x, y));
    }
	else {

        OldNode = Object->MouseNode;
		Object->MouseNode = NULL;
        
        if (OldNode) {
            FlameInvalidateRect(Object, OldNode);
        }

        if (Object->IsTooltipActivated) {
            FlameDeactivateTooltip(Object);
        }

	}

	return 0;
}


LRESULT
FlameOnPaint(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
    PAINTSTRUCT Paint;
    PFLAME_CONTROL Object;
    HDC hdc;
    RECT Rect;
    int x, y;

    Object = (PFLAME_CONTROL)SdkGetObject(hWnd);
    if (!Object) {
        return 0;
    }

    //
    // Draw the memory DC
    //

    hdc = BeginPaint(hWnd, &Paint);
    
    GetClientRect(hWnd, &Rect);
    FlameFillSolidRect(hWnd, hdc, RGB(0xFF, 0xFF, 0xFF), &Rect);

	x = GetScrollPos(hWnd, SB_HORZ);
    y = GetScrollPos(hWnd, SB_VERT);

    BitBlt(hdc, 0, 0, Rect.right, Rect.bottom, Object->hdcFlame, 
           x, y, SRCCOPY);
	
	if (Object->MouseNode) {
        FlameDrawFocusRect(Object, Object->MouseNode);
	}

    EndPaint(hWnd, &Paint);
    return 0;
}

VOID
FlameDebugDrawGraph(
	__in PFLAME_CONTROL Object,
	__in HDC hdc,
	__in LPRECT Rect
	)
{
	if (Object->hbmpFlame) {
		BitBlt(hdc, 0, 0, Rect->right, Rect->bottom, Object->hdcFlame, 
			   Object->ValidRect.left, Object->ValidRect.top, SRCCOPY);
	}
}

BOOLEAN
FlameInvalidate(
	__in HWND hWnd
	)
{
    InvalidateRect(hWnd, NULL, TRUE);
    return 0L;
}

LRESULT
FlameOnSize(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
	PFLAME_CONTROL Object;

    Object = (PFLAME_CONTROL)SdkGetObject(hWnd);
    if (!Object) {
        return 0;
    }

    FlameSetScrollBar(hWnd, SB_HORZ, Object->Width,  LOWORD(lp));
    FlameSetScrollBar(hWnd, SB_VERT, Object->Height, HIWORD(lp));

    FlameInvalidate(hWnd);
	return 0;
}

LRESULT
FlameOnNotify(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
	LRESULT Status;
	LPNMHDR pNmhdr = (LPNMHDR)lp;

	Status = 0;

	switch (pNmhdr->code) {
		case TTN_GETDISPINFO:
			return FlameOnTtnGetDispInfo(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
FlameOnTtnGetDispInfo(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
	PFLAME_CONTROL Object;
	static WCHAR Buffer[MAX_PATH];
	LPNMTTDISPINFO lpnmtdi = (LPNMTTDISPINFO)lp;

    Object = (PFLAME_CONTROL)SdkGetObject(hWnd);
    if (!Object) {
        return 0;
    }
	
	if (Object->MouseNode) {
		FlameQueryNode(Object, Object->MouseNode, FLAME_QUERY_ALL, Buffer, MAX_PATH);
		if (Buffer[0] != 0) {
			lpnmtdi->lpszText = Buffer;
		}
	}

	return 0;
}

VOID
FlameGenerateFillColor(
    __in COLORREF *Color
    )
{
    int r, g, b;

    r = 205 + ApsCreateRandomNumber(0, 50);
    g = ApsCreateRandomNumber(0, 230);
    b = ApsCreateRandomNumber(0, 55);

    *Color = RGB(r, g, b);
}

VOID 
FlameSetScrollBar(
    __in HWND hWnd,
    __in int Side, 
    __in int MaximumSize, 
    __in int PageSize 
    )
{
    SCROLLINFO si;

	if (PageSize < MaximumSize) {

		si.cbSize = sizeof(SCROLLINFO);
		si.fMask  = SIF_RANGE | SIF_PAGE;
		si.nMin   = 0;
		si.nMax   = MaximumSize - 1;
		si.nPage  = PageSize;
		si.nPos = 0;

		EnableScrollBar(hWnd, Side, ESB_ENABLE_BOTH);
		SetScrollInfo(hWnd, Side, &si, TRUE);
	}
	else {
		SetScrollPos(hWnd, Side, 0, FALSE);
		EnableScrollBar(hWnd, Side, ESB_DISABLE_BOTH);
	}
}

VOID 
FlameSetSize(
    __in HWND hWnd,
    __in int Width, 
    __in int Height, 
    __in int Linedx, 
    __in int Linedy, 
    __in BOOLEAN Resize 
    )
{
    PFLAME_CONTROL Control;
    RECT rect;

    Control = (PFLAME_CONTROL)SdkGetObject(hWnd);
    ASSERT(Control != NULL);

	ASSERT(Width == 1024);
	ASSERT(Height == 1024);

	Control->Width = Width;
	Control->Height = Height;
	Control->Linedx = Linedx;
	Control->Linedy = Linedy;

	if (Resize) {
		GetClientRect(hWnd, &rect);
		FlameSetScrollBar(hWnd, SB_HORZ, Width,  rect.right);
		FlameSetScrollBar(hWnd, SB_VERT, Height, rect.bottom);
	}
}

LRESULT
FlameOnScroll(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
    )
{
    int Bar; 
    int ScrollCode;
    int Position;
    int PageSize;
    int Distance;
    int Minimum;
    int Maximum;
	SCROLLINFO si;
    PFLAME_CONTROL Control;
	
    Control = (PFLAME_CONTROL)SdkGetObject(hWnd);
    ASSERT(Control != NULL);

    if (uMsg == WM_HSCROLL) {
        Bar = SB_HORZ;
    }
    else if (uMsg == WM_VSCROLL) {
        Bar = SB_VERT;
    }
    else {
        ASSERT(0);
    }

    ScrollCode = LOWORD(wp);
    Position = HIWORD(wp);

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_PAGE;
	GetScrollInfo(hWnd, Bar, &si);
	PageSize = si.nPage;

	switch (ScrollCode) {

		case SB_LINEDOWN:
            Distance = (Bar == SB_VERT) ? Control->Linedy : Control->Linedx;
			break;

		case SB_LINEUP:
            Distance = (Bar == SB_VERT) ? -Control->Linedy : -Control->Linedx;
			break;

		case SB_PAGEDOWN:
			Distance = PageSize;
			break;

		case SB_PAGEUP:
			Distance = - PageSize;
			break;

		case SB_THUMBPOSITION:
			Distance = Position - GetScrollPos(hWnd, Bar);
			break;

		default:
			Distance = 0;
			break;
    }

	if (Distance) {

		GetScrollRange(hWnd, Bar, &Minimum, &Maximum);

		Position = GetScrollPos(hWnd, Bar) + Distance;
		Position = max(Minimum, Position);
		Position = min(Maximum - PageSize, Position);

		Distance = Position - GetScrollPos(hWnd, Bar);

		if (Distance) {
			SetScrollPos(hWnd, Bar, Position, TRUE);

			if (Bar == SB_HORZ) {
				ScrollWindow(hWnd, -Distance, 0, NULL, NULL);
            }
			else {
				ScrollWindow(hWnd, 0, -Distance, NULL, NULL);
            }
        }
	}

    return 0L;
}

BOOLEAN
FlameSetFonts(
    __in PFLAME_CONTROL Object
    )
{
    HFONT hFont;
    LOGFONT LogFont = {0};

    LogFont.lfHeight = -11;
    LogFont.lfWeight = FW_NORMAL;
    wcscpy_s(LogFont.lfFaceName, LF_FACESIZE, L"MS Shell Dlg 2");

    hFont = CreateFontIndirect(&LogFont);
    Object->hNormalFont = hFont;

    LogFont.lfWeight = FW_BOLD;
	hFont = CreateFontIndirect(&LogFont);
    Object->hBoldFont = hFont;

    return TRUE;
}

VOID
FlameCreateBitmap(
	__in PFLAME_CONTROL Object
	)
{
	HWND hWnd;
    HDC hdc;
    HDC hdcFlame;
    HBITMAP hbmp;
    HBITMAP hbmpOld;
    HBRUSH hBrush;
    HGDIOBJ hOldBrush;
    RECT Rect;
	int Size;
    
    hWnd = Object->hWnd;
    hdc = GetDC(hWnd);
	Size = Object->ViewHoriWidth;

	//
	// Create compatible dc and bitmap, fill the background
	//

	hdcFlame = CreateCompatibleDC(hdc);
	ASSERT(hdcFlame != NULL);

	hbmp = CreateCompatibleBitmap(hdc, Size, Size);
	ASSERT(hbmp != NULL);
	ReleaseDC(hWnd, hdc);

	hbmpOld = (HBITMAP)SelectObject(hdcFlame, hbmp);
	ASSERT(hbmpOld != NULL);

	Rect.top = 0;
	Rect.left = 0;
	Rect.right = Size;
	Rect.bottom = Size;

	hBrush = CreateSolidBrush(Object->BackColor);
	hOldBrush = SelectObject(hdcFlame, hBrush);

	FillRect(hdcFlame, &Rect, hBrush);

	SelectObject(hdcFlame, hOldBrush);
	DeleteObject(hBrush);

    FlameSetFonts(Object);
    Object->hOldFont = (HFONT)SelectObject(hdcFlame, Object->hNormalFont);
	Object->hdcFlame = hdcFlame;
	Object->hbmpFlame = hbmp;
	Object->hbmpFlameOld = hbmpOld;
	Object->bmpRect = Rect;
	Object->FrameHeight = 16;

}

VOID
FlameDebugPrintNode(
	__in PFLAME_CONTROL Object,
	__in PFLAME_NODE FlameNode
	)
{
	WCHAR Buffer[MAX_PATH];
	FlameQueryNode(Object, FlameNode, FLAME_QUERY_SYMBOL, Buffer, MAX_PATH);
	DebugTrace("node:%S, ptr:%p, (%d,%d,%d,%d)", Buffer, FlameNode, FlameNode->Rect.left,
		FlameNode->Rect.top, FlameNode->Rect.right,FlameNode->Rect.bottom);
}

BOOLEAN
FlameQueryNode(
    __in PFLAME_CONTROL Object,
    __in PFLAME_NODE Node,
    __in ULONG Flags,
    __out PWCHAR Buffer,
    __in SIZE_T Length
    )
{
    LRESULT Result;
    PNM_FLAME_QUERYNODE QueryNode;

    QueryNode = (PNM_FLAME_QUERYNODE)SdkMalloc(sizeof(NM_FLAME_QUERYNODE));
    QueryNode->Node = (PCALL_NODE)Node->Node;
    QueryNode->Flags = Flags;
    QueryNode->Text[0] = 0;

    Result = SendMessage(GetParent(Object->hWnd), WM_FLAME_QUERYNODE, 0, (LPARAM)QueryNode);
    if (QueryNode->Text[0] != 0) {
        wcscpy_s(Buffer, Length, &QueryNode->Text[0]);
    }

    SdkFree(QueryNode);
    return (BOOLEAN)Result;
}

VOID
FlameDrawNodes(
	__in PFLAME_CONTROL Object
	)
{
	ULONG Number;
	PFLAME_STACK Stack;
	PFLAME_NODE Node;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	HBRUSH hBrush;
	HGDIOBJ hOldBrush;
	RECT Rect;
	int top, bottom;
    WCHAR Buffer[MAX_PATH];

	FlameCreateBitmap(Object);
	ASSERT(Object->hbmpFlame != NULL);

	//
	// Draw the flames from bottom to top
	//

	top = Object->bmpRect.bottom - Object->FrameHeight;
    bottom = Object->bmpRect.bottom;

	Stack = Object->Flame;
	for(Number = 0; Number < Stack->Depth; Number += 1) {
		
		ListHead = &Stack->Level[Number].ListHead;
		ListEntry = ListHead->Flink;
		while (ListEntry != ListHead) {

			Node = CONTAINING_RECORD(ListEntry, FLAME_NODE, ListEntry);

			//
	        // Draw the flame rect
			//

	        hBrush = CreateSolidBrush(Node->Color);
	        hOldBrush = SelectObject(Object->hdcFlame, hBrush);

			//
			// track the top/bottom
			//

			Node->Rect.top = top;
			Node->Rect.bottom = bottom;

			Rect = Node->Rect;
			Rect.top = top;
			Rect.bottom = bottom;
	        FillRect(Object->hdcFlame, &Rect, hBrush);

	        SelectObject(Object->hdcFlame, hOldBrush);
	        DeleteObject(hBrush);	

            //
            // Draw the text if the rect is big enough
            //

            if (Rect.right - Rect.left > Object->EmptyLimit.cx) {

                FlameQueryNode(Object, Node, FLAME_QUERY_SYMBOL, Buffer, MAX_PATH);

                if (Buffer[0] != 0) {

                    HFONT hOldFont;
                    RECT rc;

                    SetBkMode(Object->hdcFlame, TRANSPARENT);
                    hOldFont = (HFONT)SelectObject(Object->hdcFlame, Object->hNormalFont);

                    rc = Rect;
                    rc.left += Object->NodeHoriEdge;
                    rc.top += 1;

                    DrawText(Object->hdcFlame, Buffer, -1, &rc, DT_SINGLELINE|DT_WORD_ELLIPSIS);
                    SelectObject(Object->hdcFlame, hOldFont);
                }
            }

#ifdef _DEBUG
			FlameDebugPrintNode(Object, Node);
#endif

			ListEntry = ListEntry->Flink;
		}

		top = top - Object->FrameHeight - 1;
		bottom = top + Object->FrameHeight;
	}

	Object->ValidRect.left = 0;
	Object->ValidRect.right = Object->ViewHoriWidth;
	Object->ValidRect.top = top;
	Object->ValidRect.bottom = Object->bmpRect.bottom;

#if defined(_DEBUG)
	SdkSaveBitmap(Object->hbmpFlame, L"flame.bmp", 32);
#endif

}

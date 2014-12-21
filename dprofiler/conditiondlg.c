//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#include "conditiondlg.h"
#include "progressdlg.h"
#include "dprobe.h"
#include "mspdtl.h"
#include "mspprocess.h"

typedef enum _ConditionColumnType {
    ConditionColumnColumn,
    ConditionColumnRelation,
    ConditionColumnValue,
    ConditionColumnAction,
    ConditionColumnNumber,
} ConditionColumnType;

LISTVIEW_COLUMN ConditionColumn[ConditionColumnNumber] = {
	{ 120,  L"Column", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,  L"Relation", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 120,  L"Value", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 360,  L"Action", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

PWSTR ConditionComboObject[ConditionObjectNumber] = {
    L"Process",
    L"PID",
    L"TID",
    L"Time",
    L"Probe",
	L"Provider",
    L"Duration",
    L"Return",
    L"Detail",
};

PWSTR ConditionComboRelation[ConditionRelationNumber] = {
    L"Equal",
    L"Not Equal",
    L"Less Than",
    L"More Than",
    L"Contain",
    L"Not Contain",
	L"Early Than",
	L"Later Than",
};

PWSTR ConditionComboAction[ConditionActionNumber] = {
    L"Include",
    L"Exclude",
};

INT_PTR
ConditionDialog(
	IN HWND hWndParent,
    IN PCONDITION_OBJECT Condition,
	OUT PCONDITION_OBJECT *New,
	OUT PBOOLEAN Updated
    )
{
    #define CHILD_NUMBER 12
	DIALOG_OBJECT Object = {0};
	CONDITION_CONTEXT Context = {0};
	INT_PTR Return;
	
	static DIALOG_SCALER_CHILD Children[CHILD_NUMBER] = {
		{ IDC_STATIC_TITLE,   AlignNone, AlignNone },
		{ IDC_COMBO_OBJECT,   AlignNone, AlignNone },
		{ IDC_COMBO_RELATION, AlignNone, AlignNone },
		{ IDC_COMBO_VALUE,    AlignRight, AlignNone },
		{ IDC_STATIC_THEN,    AlignBoth, AlignNone },
		{ IDC_COMBO_ACTION,   AlignBoth,AlignNone },
		{ IDC_BUTTON_RESET,   AlignNone, AlignNone},
		{ IDC_BUTTON_ADD,     AlignBoth, AlignNone},
		{ IDC_BUTTON_REMOVE,  AlignBoth, AlignNone },
		{ IDC_LIST_CONDITION, AlignRight, AlignBottom },
		{ IDOK,     AlignBoth, AlignBoth },
		{ IDCANCEL, AlignBoth, AlignBoth },
	};

	static DIALOG_SCALER Scaler = {
		{0,0}, {0,0}, {0,0}, CHILD_NUMBER, Children
	};

	*New = NULL;
	*Updated = FALSE;

	Context.ConditionObject = Condition;

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_QUERY_FILTER;
	Object.Procedure = ConditionProcedure;
	Object.Scaler = &Scaler;

	Return = DialogCreate(&Object);

	if (Context.Updated) {
		*Updated = TRUE;
		*New = Context.UpdatedObject;
	}

	return Return;
}

ULONG
ConditionLoad(
	IN PDIALOG_OBJECT Object
	)
{
	PCONDITION_CONTEXT Context;
	PCONDITION_OBJECT Condition;
	PLIST_ENTRY ListEntry;
	PCONDITION_ENTRY Entry;
	LVITEM Item = {0};
	ULONG Number;
	HWND hWndList;
	ULONG Param;

	Context = SdkGetContext(Object, CONDITION_CONTEXT);
	Condition = Context->ConditionObject;

	hWndList = GetDlgItem(Object->hWnd, IDC_LIST_CONDITION);
	ListEntry = Condition->ConditionListHead.Flink;
	Number = 0;

	while (ListEntry != &Condition->ConditionListHead) {

		Entry = CONTAINING_RECORD(ListEntry, CONDITION_ENTRY, ListEntry);

		Param = ConditionEncodeParam((ULONG)Entry->Object, 
									 (ULONG)Entry->Relation, 
									 (ULONG)Entry->Action);

		Item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		Item.iItem = Number;
		Item.iSubItem = 0;
		Item.iImage = Entry->Action;
		Item.lParam = (LPARAM)Param;
		Item.pszText = ConditionComboObject[Entry->Object];
		ListView_InsertItem(hWndList, &Item);

		Item.mask = LVIF_TEXT;
		Item.iItem = Number;
		Item.iSubItem = 1;
		Item.pszText = ConditionComboRelation[Entry->Relation];
		ListView_SetItem(hWndList, &Item);

		Item.mask = LVIF_TEXT;
		Item.iItem = Number;
		Item.iSubItem = 2;
		Item.pszText = Entry->StringValue; 
		ListView_SetItem(hWndList, &Item);

		Item.mask = LVIF_TEXT;
		Item.iItem = Number;
		Item.iSubItem = 3;
		Item.pszText = ConditionComboAction[Entry->Action]; 
		ListView_SetItem(hWndList, &Item);

		Number += 1;
		ListEntry = ListEntry->Flink;
	}

	return S_OK;
}

LRESULT
ConditionOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	//
	// Source       HIWORD(wParam)  LOWORD(wParam)  lParam 
	// Menu         0               MenuId          0   
	// Accelerator  1               AcceleratorId	0
	// Control      NotifyCode      ControlId       hWndCtrl
	//

	switch (LOWORD(wp)) {

	case IDOK:
		ConditionOnOk(hWnd, uMsg, wp, lp);
	    break;

	case IDCANCEL:
		ConditionOnCancel(hWnd, uMsg, wp, lp);
	    break;

    case IDC_BUTTON_ADD:
        ConditionOnAdd(hWnd, uMsg, wp, lp);
        break;

    case IDC_BUTTON_REMOVE:
        ConditionOnRemove(hWnd, uMsg, wp, lp);
        break;

    case IDC_BUTTON_RESET:
        ConditionOnReset(hWnd, uMsg, wp, lp);
        break;

	case IDC_COMBO_OBJECT:
		if (HIWORD(wp) == CBN_SELCHANGE) {
			ConditionOnSelChange(hWnd, uMsg, wp, lp);	
		}
		break;
	}

	return 0;
}

INT_PTR CALLBACK
ConditionProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	INT_PTR Status;
	Status = FALSE;

	switch (uMsg) {
		
		case WM_COMMAND:
			ConditionOnCommand(hWnd, uMsg, wp, lp);
			Status = TRUE;
			break;
		
		case WM_INITDIALOG:
			ConditionOnInitDialog(hWnd, uMsg, wp, lp);
			SdkCenterWindow(hWnd);
			Status = TRUE;
			break;
	}

	return Status;
}

LRESULT
ConditionOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndCtrl;
	LVCOLUMN lvc = {0}; 
	PDIALOG_OBJECT Object;
	PCONDITION_CONTEXT Context;
	HIMAGELIST himlCondition;
    HICON hicon;
	int i;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCONDITION_CONTEXT)Object->Context;

	//
	// Initialize condition list
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CONDITION);
    ListView_SetUnicodeFormat(hWndCtrl, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndCtrl, 
		                                LVS_EX_FULLROWSELECT,  
		                                LVS_EX_FULLROWSELECT);

    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 

    for (i = 0; i < ConditionColumnNumber; i++) { 

        lvc.iSubItem = i;
		lvc.pszText = ConditionColumn[i].Title;	
		lvc.cx = ConditionColumn[i].Width;     
		lvc.fmt = ConditionColumn[i].Align;

		ListView_InsertColumn(hWndCtrl, i, &lvc);
    } 

	//
	// Create image list indicates include or exclude
	//

	himlCondition = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 0);

	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_SUCCESS));
	ImageList_AddIcon(himlCondition, hicon);
	DestroyIcon(hicon);

	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_ERROR));
	ImageList_AddIcon(himlCondition, hicon);
	DestroyIcon(hicon);

    ListView_SetImageList(hWndCtrl, himlCondition, LVSIL_SMALL);
	Context->himlCondition = himlCondition;

	//
	// Load condition from condition object
	//

	ConditionLoad(Object);
    
    hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_OBJECT);
    for(i = 0; i < ConditionObjectNumber; i++) {
        SendMessage(hWndCtrl, CB_ADDSTRING, 0, (LPARAM)ConditionComboObject[i]);
    }
	SendMessage(hWndCtrl, CB_SETCURSEL, 0, 0);

    hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_RELATION);
    SendMessage(hWndCtrl, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationEqual]);
    SendMessage(hWndCtrl, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationNotEqual]);
    SendMessage(hWndCtrl, CB_SETITEMDATA, 0, (LPARAM)ConditionRelationEqual);
    SendMessage(hWndCtrl, CB_SETITEMDATA, 1, (LPARAM)ConditionRelationNotEqual);
    SendMessage(hWndCtrl, CB_SETCURSEL, (WPARAM)0, 0);

    hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_ACTION);
    for(i = 0; i < ConditionActionNumber; i++) {
        SendMessage(hWndCtrl, CB_ADDSTRING, 0, (LPARAM)ConditionComboAction[i]);
        SendMessage(hWndCtrl, CB_SETCURSEL, (WPARAM)0, 0);
    }
    
    //
    // Limit the user input maximum to be MAX_PATH - 1 characters 
    //

    hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_ACTION);
    SendMessage(hWndCtrl, CB_LIMITTEXT, MAX_PATH - 1, 0);
	
	SdkSetMainIcon(hWnd);
	return TRUE;
}

LRESULT
ConditionOnSelChange(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndObject;
    HWND hWndRelation;
	ConditionObjectType Object;
	LRESULT index;

	hWndObject = GetDlgItem(hWnd, IDC_COMBO_OBJECT);
	hWndRelation = GetDlgItem(hWnd, IDC_COMBO_RELATION);

	Object = SendMessage(hWndObject, CB_GETCURSEL, 0, 0);
	SendMessage(hWndRelation, CB_RESETCONTENT, 0, 0);

	switch (Object) {
		case ConditionObjectProcess:

			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationEqual]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationEqual);
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationNotEqual]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationNotEqual);

			SendMessage(hWndRelation, CB_SETCURSEL, 0, 0);
			break;

		case ConditionObjectPID:
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationEqual]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationEqual);
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationNotEqual]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationNotEqual);

			SendMessage(hWndRelation, CB_SETCURSEL, 0, 0);
			break;

		case ConditionObjectTID:
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationEqual]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationEqual);
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationNotEqual]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationNotEqual);

			SendMessage(hWndRelation, CB_SETCURSEL, 0, 0);
			break;

		case ConditionObjectTime:
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationEqual]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationEqual);
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationNotEqual]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationNotEqual);

			SendMessage(hWndRelation, CB_SETCURSEL, 0, 0);
			break;

		case ConditionObjectProbe:
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationEqual]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationEqual);
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationNotEqual]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationNotEqual);
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationContain]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationContain);
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationNotContain]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationNotContain);

			SendMessage(hWndRelation, CB_SETCURSEL, 0, 0);
			break;

		case ConditionObjectProvider:
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationEqual]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationEqual);
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationNotEqual]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationNotEqual);

			SendMessage(hWndRelation, CB_SETCURSEL, 0, 0);
			break;

		case ConditionObjectDuration:
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationMoreThan]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationMoreThan);

			SendMessage(hWndRelation, CB_SETCURSEL, 0, 0);
			break;

		case ConditionObjectReturn:
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationEqual]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationEqual);
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationNotEqual]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationNotEqual);

			SendMessage(hWndRelation, CB_SETCURSEL, 0, 0);
			break;

		case ConditionObjectDetail:
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationContain]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationContain);
			index = SendMessage(hWndRelation, CB_ADDSTRING, 0, (LPARAM)ConditionComboRelation[ConditionRelationNotContain]);
			SendMessage(hWndRelation, CB_SETITEMDATA, index, (LPARAM)ConditionRelationNotContain);

			SendMessage(hWndRelation, CB_SETCURSEL, 0, 0);
			break;

		default:
			ASSERT(0);
	}

	return 0;
}

LRESULT
ConditionOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCONDITION_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CONDITION_CONTEXT);

	if (Context->himlCondition) {
		ImageList_Destroy(Context->himlCondition);
	}

	EndDialog(hWnd, IDCANCEL);
	return 0;
}

LRESULT
ConditionOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
    PDIALOG_OBJECT Object;
    PCONDITION_CONTEXT Context;
	PCONDITION_OBJECT Current;
	PCONDITION_ENTRY Entry;
    HWND hWndCtrl;
	ULONG Param;
	ULONG Count;
	ULONG i;
	ULONG Column;
	ULONG Relation;
	ULONG Action; 
	WCHAR Value[MAX_PATH];
	LVITEM lvi = {0};
	BOOLEAN IsIdentical;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PCONDITION_CONTEXT)Object->Context;

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CONDITION);
	Count = ListView_GetItemCount(hWndCtrl);

	Current = (PCONDITION_OBJECT)SdkMalloc(sizeof(CONDITION_OBJECT));
	Current->DtlObject = NULL;
	Current->NumberOfConditions = 0;
	InitializeListHead(&Current->ConditionListHead);
	
	for(i = 0; i < Count; i++) {
		
		lvi.iItem = i;
		lvi.iSubItem = ConditionColumnColumn;
		lvi.mask = LVIF_PARAM;
		ListView_GetItem(hWndCtrl, &lvi);

		Param = (ULONG)lvi.lParam;
		ConditionDecodeParam(Param, &Column, &Relation, &Action);

		lvi.mask = LVIF_TEXT;
		lvi.iSubItem = ConditionColumnValue;
		lvi.pszText = Value;
		lvi.cchTextMax = MAX_PATH;
		ListView_GetItem(hWndCtrl, &lvi);

		//
		// Currently, we treat all data as string 
		//

		Entry = (PCONDITION_ENTRY)SdkMalloc(sizeof(CONDITION_ENTRY));
		Entry->Action = Action;
		Entry->Object = Column;
		Entry->Relation = Relation;
		StringCchCopy(Entry->StringValue, MAX_PATH, Value);
		
		InsertTailList(&Current->ConditionListHead, &Entry->ListEntry);
		Current->NumberOfConditions += 1;
	}

	//
	// Compare whether condition changed
	//

	IsIdentical = ConditionIsIdentical(Current, Context->ConditionObject);
	if (!IsIdentical) {
		Context->Updated = TRUE;
		Context->UpdatedObject = Current;
	} 

	if (Context->himlCondition) {
		ImageList_Destroy(Context->himlCondition);
	}

	EndDialog(hWnd, IDOK);
	return 0;
}

ULONG
ConditionEncodeParam(
	IN ULONG Object,
	IN ULONG Relation,
	IN ULONG Action
	)
{
	ULONG Bits = 0;

	Bits = Object | (Relation << 8) | (Action << 16);
	return Bits;
}

VOID
ConditionDecodeParam(
	IN ULONG Bits,
	OUT PULONG Object,
	OUT PULONG Relation,
	OUT PULONG Action
	)
{
	*Object   = Bits & 0x000000ffUL;
	*Relation = Bits & 0x0000ff00UL;
	*Action   = Bits & 0x00ff0000UL;
}

LRESULT
ConditionOnAdd(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    )
{
    HWND hWndCtrl;
    LRESULT Current;
	LRESULT Object;
	LRESULT Relation;
	LRESULT Action;
    LVITEM Item = {0};
    WCHAR Value[MAX_PATH];
	ULONG Param;

    hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_VALUE);
    GetWindowText(hWndCtrl, Value, MAX_PATH - 1);
    if (Value[0] == 0) {
        MessageBox(hWnd, L"Value can not be empty.", L"D Probe", MB_OK|MB_ICONERROR);
        return 0;
    } 

    hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_OBJECT);
    Object = SendMessage(hWndCtrl, CB_GETCURSEL, 0, 0);

	hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_RELATION);
    Current = SendMessage(hWndCtrl, CB_GETCURSEL, 0, 0);
	Relation = SendMessage(hWndCtrl, CB_GETITEMDATA, Current, 0);

    hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_ACTION);
    Action = SendMessage(hWndCtrl, CB_GETCURSEL, 0, 0);

	//
	// Encode Object, Relation and Action bits
	//

	Param = ConditionEncodeParam((ULONG)Object, (ULONG)Relation, (ULONG)Action);

	//
	// Insert new conditon entry
	//

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CONDITION);
    Item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;

	Item.iItem = ListView_GetItemCount(hWndCtrl);
	Item.iSubItem = 0;
    Item.pszText = ConditionComboObject[Object];
	Item.lParam = (LPARAM)Param;
    Item.iImage = (int)Action;
	ListView_InsertItem(hWndCtrl, &Item);

	Item.mask = LVIF_TEXT;
	Item.iSubItem = 1;
    Item.pszText = ConditionComboRelation[Relation];
	ListView_SetItem(hWndCtrl, &Item);

	Item.mask = LVIF_TEXT;
	Item.iSubItem = 2;
    Item.pszText = Value;
	ListView_SetItem(hWndCtrl, &Item);

	Item.mask = LVIF_TEXT;
	Item.iSubItem = 3;
    Item.pszText = ConditionComboAction[Action];
	ListView_SetItem(hWndCtrl, &Item);

    return 0;
}

LRESULT
ConditionOnRemove(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    )
{
    HWND hWndCtrl;
    int Current;

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CONDITION);
    Current = ListViewGetFirstSelected(hWndCtrl); 
    if (Current == -1) {
        return 0;
    }

    ListView_DeleteItem(hWndCtrl, Current);
    return 0;
}

LRESULT
ConditionOnReset(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    )
{
    HWND hWndCtrl;

    hWndCtrl = GetDlgItem(hWnd, IDC_LIST_CONDITION);
	ListView_DeleteAllItems(hWndCtrl);
    return 0;
}

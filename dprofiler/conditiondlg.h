//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#ifndef _CONDITIONDLG_H_
#define _CONDITIONDLG_H_

#include "condition.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CONDITION_CONTEXT {
	struct _CONDITION_OBJECT *ConditionObject;
	struct _CONDITION_OBJECT *UpdatedObject;
	HIMAGELIST himlCondition;
	BOOLEAN Updated;
} CONDITION_CONTEXT, *PCONDITION_CONTEXT;

INT_PTR
ConditionDialog(
	IN HWND hWndParent,
    IN PCONDITION_OBJECT Condition,
	OUT PCONDITION_OBJECT *New,
	OUT PBOOLEAN Updated
    );

INT_PTR CALLBACK
ConditionProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ConditionOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ConditionOnAdd(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    );

LRESULT
ConditionOnRemove(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    );

LRESULT
ConditionOnReset(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    );

LRESULT
ConditionOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ConditionOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ConditionOnSelChange(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

ULONG
ConditionLoad(
	IN PDIALOG_OBJECT Object
	);

ULONG
ConditionEncodeParam(
	IN ULONG Object,
	IN ULONG Relation,
	IN ULONG Action
	);

VOID
ConditionDecodeParam(
	IN ULONG Bits,
	OUT PULONG Object,
	OUT PULONG Relation,
	OUT PULONG Action
	);

#ifdef __cplusplus
}
#endif

#endif
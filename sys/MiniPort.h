#pragma once
#ifndef _MINIPORT_H

#include "VidPn_common.h"

typedef struct _DISPLAY_ADAPTER_INFO
{
	ULONG NumberOfVideoPresentSources;
	ULONG NumberOfChildren;
}
DISPLAY_ADAPTER_INFO, *PDISPLAY_ADAPTER_INFO;


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
NTSTATUS MyDxgkDdiAddDevice(
	IN CONST PDEVICE_OBJECT pDevObj,
	OUT PVOID *pContext
);

NTSTATUS MyDxgkDdiStartDevice(
	IN CONST PVOID  pDevContxt,
	IN PDXGK_START_INFO  pDxgkStartInfo,
	IN PDXGKRNL_INTERFACE  pDxgkInterface,
	OUT PULONG  pNumberOfVideoPresentSources,
	OUT PULONG  pNumberOfChildren
);

NTSTATUS MyDxgkDdiQueryChildRelations(
	IN CONST PVOID pDevContxt,
	IN OUT PDXGK_CHILD_DESCRIPTOR pChildRelations,
	IN ULONG ChildRelationsSize
);

NTSTATUS MyDxgkDdiQueryChildStatus(
	IN CONST PVOID  pDevContxt,
	IN PDXGK_CHILD_STATUS  pChildStatus,
	IN BOOLEAN  NonDestructiveOnly
);

NTSTATUS MyDxgkDdiQueryDeviceDescriptor(
	IN CONST PVOID pDevContxt,
	IN ULONG ChildUid,
	IN OUT PDXGK_DEVICE_DESCRIPTOR pDxgkDevDesc
);

NTSTATUS
APIENTRY
MyDxgkDdiRecommendFunctionalVidPn(
	CONST HANDLE  hAdapter,
	CONST DXGKARG_RECOMMENDFUNCTIONALVIDPN* CONST  pRecommendFunctionalVidPnArg
);

NTSTATUS
APIENTRY
MyDxgkDdiIsSupportedVidPn(
	CONST HANDLE  hAdapter,
	OUT DXGKARG_ISSUPPORTEDVIDPN*  pIsSupportedVidPnArg
);

NTSTATUS
APIENTRY
MyDxgkDdiEnumVidPnCofuncModality(
	CONST HANDLE  hAdapter,
	CONST DXGKARG_ENUMVIDPNCOFUNCMODALITY* CONST  pEnumCofuncModalityArg
);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !_MINIPORT_H

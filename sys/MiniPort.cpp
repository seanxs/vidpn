#include "MiniPort.h"

extern PVIDPN_SEAN_DVC_EXT pMyDevExt;
DISPLAY_ADAPTER_INFO AdapterInfo;

NTSTATUS MyDxgkDdiAddDevice(
	IN CONST PDEVICE_OBJECT pDevObj,
	OUT PVOID *pContext
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	FUNC_ENTER();

	//INFO_MSG_VIDPN("DxgkDdiAddDevice is called!\n");
	status = pMyDevExt->pOrigenal_Init_Data->DxgkDdiAddDevice(pDevObj, pContext);
	if (!NT_SUCCESS(status))
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiAddDevice failed! return status : 0x%X\n", status);
	}

	FUNC_EXIT();

	return status;
}

NTSTATUS MyDxgkDdiStartDevice(
	IN CONST PVOID  pDevContxt,
	IN PDXGK_START_INFO  pDxgkStartInfo,
	IN PDXGKRNL_INTERFACE  pDxgkInterface,
	OUT PULONG  pNumberOfVideoPresentSources,
	OUT PULONG  pNumberOfChildren
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	FUNC_ENTER();

	status = pMyDevExt->pOrigenal_Init_Data->DxgkDdiStartDevice(pDevContxt, pDxgkStartInfo, pDxgkInterface, pNumberOfVideoPresentSources, pNumberOfChildren);
	
	if (!NT_SUCCESS(status))
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiStartDevice failed! return status : 0x%X\n", status);
	}
	else
	{
		AdapterInfo.NumberOfChildren = *pNumberOfChildren;
		AdapterInfo.NumberOfVideoPresentSources = *pNumberOfVideoPresentSources;
		if (AdapterInfo.NumberOfChildren > 1)
		{
			pMyDevExt->IsMonitorBlocked = TRUE;
		}

		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiStartDevice succeed! get %d Sources, %d children\n", *pNumberOfVideoPresentSources, *pNumberOfChildren);
	}

	FUNC_EXIT();

	return status;
}

NTSTATUS MyDxgkDdiQueryChildRelations(
	IN CONST PVOID pDevContxt,
	IN OUT PDXGK_CHILD_DESCRIPTOR pChildRelations,
	IN ULONG ChildRelationsSize
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	FUNC_ENTER();

	status = pMyDevExt->pOrigenal_Init_Data->DxgkDdiQueryChildRelations(pDevContxt, pChildRelations, ChildRelationsSize);

	if (!NT_SUCCESS(status))
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiQueryChildRelations failed! return status : 0x%X\n", status);
	}
	else
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiQueryChildRelations succeed!\n");
	}

	FUNC_EXIT();

	return status;
}

NTSTATUS MyDxgkDdiQueryChildStatus(
	IN CONST PVOID  pDevContxt,
	IN PDXGK_CHILD_STATUS  pChildStatus,
	IN BOOLEAN  NonDestructiveOnly
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	FUNC_ENTER();

	status = pMyDevExt->pOrigenal_Init_Data->DxgkDdiQueryChildStatus(pDevContxt, pChildStatus, NonDestructiveOnly);

	if (!NT_SUCCESS(status))
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiQueryChildStatus failed! return status : 0x%X\n", status);
	}
	else
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiQueryChildStatus succeed!\n");
		if (pChildStatus->Type == StatusConnection)
		{
			if (pMyDevExt->IsMonitorBlocked && pChildStatus->ChildUid == AdapterInfo.NumberOfChildren-1)
			{
				pChildStatus->HotPlug.Connected = FALSE;
			}
			DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "Child uid : %d, status type : StatusConnection, connected : %d\n", pChildStatus->ChildUid, pChildStatus->HotPlug.Connected);
		}
		else if (pChildStatus->Type == StatusRotation)
		{
			DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "Child uid : %d, status type : StatusRotation, angle : %c\n", pChildStatus->ChildUid, pChildStatus->Rotation.Angle);
		}
	}

	FUNC_EXIT();

	return status;
}

NTSTATUS MyDxgkDdiQueryDeviceDescriptor(
	IN CONST PVOID pDevContxt,
	IN ULONG ChildUid,
	IN OUT PDXGK_DEVICE_DESCRIPTOR pDxgkDevDesc
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	FUNC_ENTER();

	status = pMyDevExt->pOrigenal_Init_Data->DxgkDdiQueryDeviceDescriptor(pDevContxt, ChildUid, pDxgkDevDesc);

	if (!NT_SUCCESS(status))
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiQueryDeviceDescriptor failed! ChildUid : 0x%d, return status : 0x%X\n", ChildUid, status);
	}
	else
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiQueryDeviceDescriptor succeed!\n");
	}

	FUNC_EXIT();

	return status;
}

NTSTATUS
APIENTRY
MyDxgkDdiRecommendFunctionalVidPn(
	CONST HANDLE  hAdapter,
	CONST DXGKARG_RECOMMENDFUNCTIONALVIDPN* CONST  pRecommendFunctionalVidPnArg
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	FUNC_ENTER();

	status = pMyDevExt->pOrigenal_Init_Data->DxgkDdiRecommendFunctionalVidPn(hAdapter, pRecommendFunctionalVidPnArg);

	if (!NT_SUCCESS(status))
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiRecommendFunctionalVidPn failed! return status : 0x%X\n", status);
	}
	else
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiRecommendFunctionalVidPn succeed!\n");
	}

	FUNC_EXIT();

	return status;
}

NTSTATUS
APIENTRY
MyDxgkDdiIsSupportedVidPn(
	CONST HANDLE  hAdapter,
	OUT DXGKARG_ISSUPPORTEDVIDPN*  pIsSupportedVidPnArg
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	FUNC_ENTER();

	status = pMyDevExt->pOrigenal_Init_Data->DxgkDdiIsSupportedVidPn(hAdapter, pIsSupportedVidPnArg);

	if (!NT_SUCCESS(status))
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiIsSupportedVidPn failed! return status : 0x%X\n", status);
	}
	else
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiIsSupportedVidPn succeed! hDesiredVidPn : 0x%x, is supported\n",pIsSupportedVidPnArg->hDesiredVidPn, pIsSupportedVidPnArg->IsVidPnSupported);
	}

	FUNC_EXIT();

	return status;
}

NTSTATUS
APIENTRY
MyDxgkDdiEnumVidPnCofuncModality(
	CONST HANDLE  hAdapter,
	CONST DXGKARG_ENUMVIDPNCOFUNCMODALITY* CONST  pEnumCofuncModalityArg
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	FUNC_ENTER();

	status = pMyDevExt->pOrigenal_Init_Data->DxgkDdiEnumVidPnCofuncModality(hAdapter, pEnumCofuncModalityArg);

	if (!NT_SUCCESS(status))
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiEnumVidPnCofuncModality failed! return status : 0x%X\n", status);
	}
	else
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "DxgkDdiEnumVidPnCofuncModality succeed!\n");
	}

	FUNC_EXIT();

	return status;
}
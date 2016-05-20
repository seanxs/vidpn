#include "VidPn_sean.h"
#include "MiniPort.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, DrvUnload)
#pragma alloc_text (PAGE, Dispatch_StartDvc)
#pragma alloc_text (PAGE, Dispatch_RemoveDvc)
#endif

UNICODE_STRING RegPath;
//UNICODE_STRING TargetRegPath;
PVIDPN_SEAN_DVC_EXT pMyDevExt = NULL;

NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath )
{
    NTSTATUS status = STATUS_SUCCESS;
	ULONG               ulIndex;
    PDRIVER_DISPATCH  * dispatch;

	FUNC_ENTER();

	RegPath.MaximumLength = RegistryPath->Length + sizeof(UNICODE_NULL);
    RegPath.Length = RegistryPath->Length;
    RegPath.Buffer = (PWSTR)ExAllocatePoolWithTag(
		PagedPool,
		RegPath.MaximumLength,
		VIDPN_SEAN_POOL_TAG );
	if( RegPath.Buffer == NULL )
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyUnicodeString( &RegPath, RegistryPath );

	//
    // Create dispatch points
    //
    for( ulIndex = 0, dispatch = DriverObject->MajorFunction;
         ulIndex <= IRP_MJ_MAXIMUM_FUNCTION;
         ulIndex++, dispatch++)
	{

        *dispatch = Dispatch_FilterPassThrough;
    }
	
    DriverObject->DriverUnload = DrvUnload;

	status = AddDvc( DriverObject, NULL );

	FUNC_EXIT();
	
	return status;
}

NTSTATUS AddDvc(
    IN PDRIVER_OBJECT pDrvObj,
    IN PDEVICE_OBJECT pPhyDvcObj )
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT pDvcObj = NULL;
	PVIDPN_SEAN_DVC_EXT pDvcExt = NULL;
	UNICODE_STRING DvcName;
	UNICODE_STRING DosName;
	UNICODE_STRING Dxgkrnl_Reg_Path;
	UNICODE_STRING Dxgkrnl_DevName;
	PIRP pIrp = NULL;
	IO_STATUS_BLOCK  Io_Sts_Blk;

	UNREFERENCED_PARAMETER( pPhyDvcObj );

	FUNC_ENTER();

	do
	{
		status = IoCreateDevice(
			pDrvObj,
			sizeof( VIDPN_SEAN_DVC_EXT ),
			NULL, //WDM filter and function drivers do not name their device objects
			FILE_DEVICE_VIDEO,
			FILE_DEVICE_SECURE_OPEN,
			FALSE,
			&pDvcObj );
		if( !NT_SUCCESS (status) )
		{
			INFO_MSG_VIDPN( "IoCreateDevice() failed.\n" );
			break;
		}

		RtlInitUnicodeString( &DvcName, VIDPN_SEAN_DVC_NAME );
		RtlInitUnicodeString( &DosName, VIDPN_SEAN_DOS_NAME );

		pDvcExt = (PVIDPN_SEAN_DVC_EXT)pDvcObj->DeviceExtension;
		RtlZeroMemory( pDvcExt, sizeof(VIDPN_SEAN_DVC_EXT) );
		KeInitializeEvent( &pDvcExt->NextDrv_Event, NotificationEvent, FALSE );

		pMyDevExt = pDvcExt;

		//pDvcExt->IsMonitorBlocked = TRUE;
		RtlInitUnicodeString( &Dxgkrnl_Reg_Path, DXGKRNL_REG_PATH );		
		status = ZwLoadDriver( &Dxgkrnl_Reg_Path );
		if( !NT_SUCCESS (status) && status != STATUS_IMAGE_ALREADY_LOADED )
		{
			INFO_MSG_VIDPN( "ZwLoadDriver() failed, Can not load Dxgkrnl driver! " );
			//INFO_MSG_VIDPN( "Error code : 0x%X\n", status );
			break;
		}

		RtlInitUnicodeString( &Dxgkrnl_DevName, DXGKRNL_DEV_NAME );
		status = IoGetDeviceObjectPointer( &Dxgkrnl_DevName, FILE_ALL_ACCESS, &pDvcExt->pDxgkrnlFileObj, &pDvcExt->pDxgkrnlDevObj );
		if( !NT_SUCCESS (status) )
		{
			INFO_MSG_VIDPN( "IoGetDeviceObjectPointer() failed, Can not get Dxgkrnl device pointer! " );
			//INFO_MSG_VIDPN( "Error code : 0x%X\n", status );
			break;
		}
		
		pDvcExt->pDvcObjNext = IoAttachDeviceToDeviceStack( pDvcObj, pDvcExt->pDxgkrnlDevObj );
		if( pDvcExt->pDvcObjNext == NULL )
		{
	        //IoDeleteDevice( pDvcObj );
			status = STATUS_NO_SUCH_DEVICE;
			INFO_MSG_VIDPN( "IoAttachDeviceToDeviceStack() failed.\n" );
			//INFO_MSG_VIDPN( "Return code : 0x%X\n", status );
			break;
		}
		
		pIrp = IoBuildDeviceIoControlRequest(
			0x23003F, // IOCTL_VIDEO_CREATE_CHILD
			pDvcExt->pDxgkrnlDevObj, 
			NULL, 
			0, 
			&pDvcExt->pDxgKrnl_DpiInit, 
			sizeof( PDXGKRNL_DPIINITIALIZE ), 
			TRUE, // IRP_MJ_INTERNAL_DEVICE_CONTROL
			&pDvcExt->NextDrv_Event, 
			&Io_Sts_Blk ); 
		if( pIrp == NULL )
		{
			status = STATUS_UNSUCCESSFUL; 
			INFO_MSG_VIDPN( "IoBuildDeviceIoControlRequest() failed.\n" );
			break;
		}
		status = IoCallDriver( pDvcExt->pDxgkrnlDevObj, pIrp ); 
        if( status == STATUS_PENDING )
        { 
            KeWaitForSingleObject( &pDvcExt->NextDrv_Event, Executive, KernelMode, FALSE, NULL );
            status = Io_Sts_Blk.Status;
        }

		if( pDvcExt->pDxgKrnl_DpiInit == NULL )
		{
			status = STATUS_UNSUCCESSFUL; 
			INFO_MSG_VIDPN( "Can not get Dxkgrnl!DpiInitialize.\n" );
			break;

		}

		pDvcObj->Flags |= DO_POWER_PAGABLE | DO_BUFFERED_IO | DO_DIRECT_IO;
		pDvcExt->pDvcObjSelf = pDvcObj;
		pDvcObj->Flags &= ~DO_DEVICE_INITIALIZING;
		pDvcExt->IsAttachedToDxgkrnl = TRUE;

		IoInitializeRemoveLock(
			&pDvcExt->RemoveLock,
			VIDPN_SEAN_LOCK_TAG,
			1,
			5 );

		pDvcExt->SymbName.MaximumLength = DosName.MaximumLength;    	
		pDvcExt->SymbName.Length = DosName.Length;
    	pDvcExt->SymbName.Buffer = (PWSTR)ExAllocatePoolWithTag(
			PagedPool,
			pDvcExt->SymbName.MaximumLength,
			VIDPN_SEAN_POOL_TAG );
		if( pDvcExt->SymbName.Buffer == NULL )
        {
        	status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
	    RtlCopyUnicodeString( &pDvcExt->SymbName, &DosName );

		KeInitializeEvent(
			&pDvcExt->Thread_Event,
			NotificationEvent,
			FALSE );
		pDvcExt->StopThread = FALSE;

		InitializeListHead( &pDvcExt->IoCtrl_List );
		KeInitializeSpinLock( &pDvcExt->Queue_Lock );
	}
	while( FALSE );

	if( !NT_SUCCESS(status) )
	{
		if( pDvcObj != NULL )
			IoDeleteDevice( pDvcObj );
		if( pDvcExt->pDvcObjNext != NULL )
			IoDetachDevice( pDvcExt->pDvcObjNext );
		//IoDeleteSymbolicLink( &DosName );
	}

	FUNC_EXIT();
	
	return status;
}

VOID DrvUnload (
    IN PDRIVER_OBJECT pDrvObj )
{
	UNREFERENCED_PARAMETER( pDrvObj );

	//NTSTATUS status = STATUS_SUCCESS;

	FUNC_ENTER();

	ExFreePoolWithTag( RegPath.Buffer, VIDPN_SEAN_POOL_TAG );
	
	FUNC_EXIT();
	
	return;
}

NTSTATUS Dispatch_StartDvc(
	IN PDEVICE_OBJECT  pDvcObj,
    IN PIRP pIrp )
{
	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION pIrpStack = NULL;
	PVIDPN_SEAN_DVC_EXT pDvcExt = NULL;

	pDvcExt = (PVIDPN_SEAN_DVC_EXT)pDvcObj->DeviceExtension;
	pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

	FUNC_ENTER();

	FUNC_EXIT();

	return status;
}

NTSTATUS Dispatch_RemoveDvc(
	IN PDEVICE_OBJECT  pDvcObj,
    IN PIRP pIrp )
{
	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION pIrpStack = NULL;
	PVIDPN_SEAN_DVC_EXT pDvcExt = NULL;

	pDvcExt = (PVIDPN_SEAN_DVC_EXT)pDvcObj->DeviceExtension;
	pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

	FUNC_ENTER();

	FUNC_EXIT();

	return status;
}

NTSTATUS Dispatch_FilterPassThrough(
	IN PDEVICE_OBJECT  pDvcObj,
    IN PIRP pIrp )
{
	PVIDPN_SEAN_DVC_EXT pDvcExt = NULL;
	PIO_STACK_LOCATION pIrpStack = NULL;
    NTSTATUS    status;

	FUNC_ENTER();
	
    pDvcExt = (PVIDPN_SEAN_DVC_EXT) pDvcObj->DeviceExtension;
	pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

	DbgPrintEx( VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_TRACE_LEVEL,
	"%s,Line %d,PNP Major Function code : 0x%X\n", __FILE__, __LINE__, pIrpStack->MajorFunction );

    //status = IoAcquireRemoveLock( &pDvcExt->RemoveLock, pIrp );
    if( pIrpStack->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL )
	{
        DbgPrintEx( VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_TRACE_LEVEL,
			"%s,Line %d, receive IRP_MJ_INTERNAL_DEVICE_CONTROL with Minor Function code : 0x%X\n",
			__FILE__,
			__LINE__,
		pIrpStack->Parameters.DeviceIoControl.IoControlCode );
		if( pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTRL_VIDPN_DXGKRNL_DPIINIT )
		{
			ObDereferenceObject( pDvcExt->pDxgkrnlFileObj );
			IoDetachDevice( pDvcExt->pDvcObjNext );
			DbgPrintEx( VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_TRACE_LEVEL,
				"the real wddm driver has initialize itself to dxgkrnl, so we detach it.\n",
				__FILE__,
				__LINE__ );

			if (pIrp->UserBuffer != NULL)
			{
#ifdef _WIN64
				*((PDWORD64)pIrp->UserBuffer) = (DWORD64)MyDpiInitialize;
				pIrp->IoStatus.Information = 8;
#else
				*((PDWORD)pIrp->UserBuffer) = (DWORD)MyDpiInitialize;
				pIrp->IoStatus.Information = 4;
#endif			
			}

            pIrp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest( pIrp, IO_NO_INCREMENT );
			
			FUNC_EXIT();
			return STATUS_SUCCESS;
		}
    }

   IoSkipCurrentIrpStackLocation (pIrp);
   status = IoCallDriver(pDvcExt->pDvcObjNext, pIrp);
   //IoReleaseRemoveLock(&pDvcExt->RemoveLock, pIrp); 

   FUNC_EXIT();
   
   return status;
}

NTSTATUS MyDpiInitialize(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath,
	IN PDRIVER_INITIALIZATION_DATA DriverInitializationData
)
{
	ASSERT(DriverObject);
	ASSERT(RegistryPath);
	ASSERT(DriverInitializationData);

	NTSTATUS status = STATUS_UNSUCCESSFUL;
	ANSI_STRING AdapterRegPath;

	DbgPrint("Someone need u!");

	pMyDevExt->pDisplayAdapterDrvObj = DriverObject;
	
	status = RtlUnicodeStringToAnsiString(&AdapterRegPath, RegistryPath, TRUE);
	if (NT_SUCCESS(status))
	{
		DbgPrintEx(VIDPN_SEAN_DBG_COMPONENT_ID, DPFLTR_INFO_LEVEL, "Adapter Registry Path:%s\n", AdapterRegPath.Buffer);
		RtlFreeAnsiString(&AdapterRegPath);
	}

	if (pMyDevExt->pDxgKrnl_DpiInit != NULL)
	{
		pMyDevExt->pMyWddm_Init_Data = (PDRIVER_INITIALIZATION_DATA)ExAllocatePoolWithTag(
			PagedPool, sizeof(DRIVER_INITIALIZATION_DATA), VIDPN_SEAN_POOL_TAG);
		if (pMyDevExt->pMyWddm_Init_Data == NULL)
		{
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		RtlCopyMemory(pMyDevExt->pMyWddm_Init_Data, DriverInitializationData, sizeof(DRIVER_INITIALIZATION_DATA));

		pMyDevExt->pOrigenal_Init_Data = (PDRIVER_INITIALIZATION_DATA)ExAllocatePoolWithTag(
			PagedPool, sizeof(DRIVER_INITIALIZATION_DATA), VIDPN_SEAN_POOL_TAG);
		if (pMyDevExt->pOrigenal_Init_Data == NULL)
		{
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		RtlCopyMemory(pMyDevExt->pOrigenal_Init_Data, DriverInitializationData, sizeof(DRIVER_INITIALIZATION_DATA));

		pMyDevExt->pMyWddm_Init_Data->DxgkDdiAddDevice = MyDxgkDdiAddDevice;
		pMyDevExt->pMyWddm_Init_Data->DxgkDdiStartDevice = MyDxgkDdiStartDevice;
		pMyDevExt->pMyWddm_Init_Data->DxgkDdiQueryChildRelations = MyDxgkDdiQueryChildRelations;
		pMyDevExt->pMyWddm_Init_Data->DxgkDdiQueryChildStatus = MyDxgkDdiQueryChildStatus;
		pMyDevExt->pMyWddm_Init_Data->DxgkDdiQueryDeviceDescriptor = MyDxgkDdiQueryDeviceDescriptor;
		pMyDevExt->pMyWddm_Init_Data->DxgkDdiRecommendFunctionalVidPn = MyDxgkDdiRecommendFunctionalVidPn;
		pMyDevExt->pMyWddm_Init_Data->DxgkDdiIsSupportedVidPn = MyDxgkDdiIsSupportedVidPn;
		pMyDevExt->pMyWddm_Init_Data->DxgkDdiEnumVidPnCofuncModality = MyDxgkDdiEnumVidPnCofuncModality;

		status = pMyDevExt->pDxgKrnl_DpiInit(DriverObject, RegistryPath, pMyDevExt->pMyWddm_Init_Data);
	}

	return status;
}
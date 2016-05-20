#ifndef _VIDPN_SEAN_H
#define _VIDPN_SEAN_H

#include "VidPn_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath );

NTSTATUS Dispatch_FilterPassThrough(
	IN PDEVICE_OBJECT  pDvcObj,
    IN PIRP pIrp );

NTSTATUS Dispatch_StartDvc(
	IN PDEVICE_OBJECT  pDvcObj,
    IN PIRP pIrp );

NTSTATUS Dispatch_RemoveDvc(
	IN PDEVICE_OBJECT  pDvcObj,
    IN PIRP pIrp );

DRIVER_UNLOAD DrvUnload;

NTSTATUS AddDvc(
    IN PDRIVER_OBJECT pDrvObj,
    IN PDEVICE_OBJECT pPhyDvcObj );

NTSTATUS MyDpiInitialize(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath,
	IN PDRIVER_INITIALIZATION_DATA DriverInitializationData
);

#ifdef __cplusplus
}
#endif

#endif

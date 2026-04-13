// Works on Windows 10 1607 and later..
#define NDIS660 1
#define NDIS_PROTOCOL_DRIVER 1

#include <ntifs.h>
#include <ntddk.h>
#include <ndis.h>

#include "ndis.hpp"

NDIS_HANDLE g_NdisProtocolHandle = NULL; // _NDIS_PROTOCOL_BLOCK structure

void DrvUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	if (g_NdisProtocolHandle) {
		NdisDeregisterProtocolDriver(g_NdisProtocolHandle);
		g_NdisProtocolHandle = NULL;
	}

}

extern "C" NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	NDIS_PROTOCOL_DRIVER_CHARACTERISTICS protocolChar = { 0 };

	protocolChar.Header.Type			= NDIS_OBJECT_TYPE_PROTOCOL_DRIVER_CHARACTERISTICS;
	protocolChar.Header.Revision		= NDIS_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_2;
	protocolChar.Header.Size			= NDIS_SIZEOF_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_2;
	
	protocolChar.MajorNdisVersion = 6;
	protocolChar.MinorNdisVersion = 60;

	protocolChar.MajorDriverVersion = 1;
	protocolChar.MinorDriverVersion = 0;

	NdisInitUnicodeString(&protocolChar.Name, L"SAHAPROT");

	protocolChar.BindAdapterHandlerEx = ProtoBindAdapterEx;
	protocolChar.UnbindAdapterHandlerEx = ProtoUnbindAdapterEx;
	protocolChar.OpenAdapterCompleteHandlerEx = ProtoOpenAdapterCompleteEx;
	protocolChar.CloseAdapterCompleteHandlerEx = ProtoCloseAdapterCompleteEx;
	protocolChar.OidRequestCompleteHandler = ProtoOidRequestComplete;
	protocolChar.SendNetBufferListsCompleteHandler = ProtoSendNetBufferListsComplete;
	protocolChar.ReceiveNetBufferListsHandler = ProtoReceiveNetBufferLists;
	protocolChar.StatusHandlerEx = ProtoStatusHandlerEx;
	protocolChar.NetPnPEventHandler = ProtoNetPnPEvent;
	protocolChar.UninstallHandler = NULL;

	NDIS_STATUS status = NdisRegisterProtocolDriver(NULL, &protocolChar, &g_NdisProtocolHandle);

	if (status != NDIS_STATUS_SUCCESS)
		return (NTSTATUS)status;

	KdPrint(("[NdisHook] Dumping g_NdisProtocolHandle\n"));
	KdPrint(("\tg_NdisProtocolHandle: %p\n", g_NdisProtocolHandle));

	DriverObject->DriverUnload = DrvUnload;

	return STATUS_SUCCESS;
}
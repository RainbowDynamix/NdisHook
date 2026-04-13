#ifndef ndis_hpp
#define ndis_hpp

#include <ntifs.h>
#include <ntddk.h>
#include <cstdint>

#define NDIS660 1
#define NDIS_PROTOCOL_DRIVER 1

#include <ndis.h>

extern NDIS_HANDLE g_ndis_protocol_handle;

/*
*     [+0x000] Type             : 0x3 [Type: unsigned char]
    [+0x001] Revision         : 0x1 [Type: unsigned char]
    [+0x002] Size             : 0x386 [Type: unsigned short]

    [+0x000] Header           [Type: _NDIS_OBJECT_HEADER]
    [+0x008] ProtocolDriverContext : 0x0 [Type: void *]
    [+0x010] NextProtocol     : 0xffffb1865e2e4af0 [Type: _NDIS_PROTOCOL_BLOCK *]
    [+0x018] OpenQueue        : 0xffffb1865e274b30 [Type: _NDIS_OPEN_BLOCK *]
    [+0x020] Ref              [Type: _REFERENCE_EX]
    [+0x038] MajorNdisVersion : 0x6 [Type: unsigned char]
    [+0x039] MinorNdisVersion : 0x1e [Type: unsigned char]
    [+0x03a] MajorDriverVersion : 0x0 [Type: unsigned char]
    [+0x03b] MinorDriverVersion : 0x0 [Type: unsigned char]
    [+0x03c] Reserved         : 0x0 [Type: unsigned int]
    [+0x040] Flags            : 0x0 [Type: unsigned int]
    [+0x048] Name             : "LLTDIO" [Type: _UNICODE_STRING]
*/

typedef struct _NDIS_OPEN_BLOCK {
    uint8_t pad_0x220[0x208];
    void(__cdecl* prot_send_net_buffer_list_complete)(void*, _NET_BUFFER_LIST*, unsigned long);
    void* pad;
    void* pad_1;
    void(__cdecl* recieve_net_buffer_lists)(void*, _NET_BUFFER_LIST*, unsigned long, unsigned long, unsigned long);
}NDIS_OPEN_BLOCK, *PNDIS_OPEN_BLOCK;

typedef struct _NDIS_PROTOCOL_BLOCK {
    NDIS_OBJECT_HEADER header;
    void* protocol_driver_context;
    struct _NDIS_PROTOCOL_BLOCK* next_protocol;
    struct _NDIS_OPEN_BLOCK* open_queue;
    uint8_t pad_0x18[0x18];
    uint8_t major_ndis_version;
    uint8_t minor_ndis_version;
    uint8_t major_driver_version;
    uint8_t minor_driver_version;
    uint32_t reserved;
    uint32_t flags;
    UNICODE_STRING name;
}NDIS_PROTOCOL_BLOCK, *PNDIS_PROTOCOL_BLOCK;

namespace ndis_interface {
    NDIS_STATUS ProtoBindAdapterEx(NDIS_HANDLE ProtocolDriverContext, NDIS_HANDLE BindContext,
        PNDIS_BIND_PARAMETERS BindParameters);

    NDIS_STATUS ProtoUnbindAdapterEx(NDIS_HANDLE UnbindContext, NDIS_HANDLE ProtocolBindingContext);

    void ProtoOpenAdapterCompleteEx(NDIS_HANDLE ProtocolBindingContext, NDIS_STATUS Status);

    void ProtoCloseAdapterCompleteEx(NDIS_HANDLE ProtocolBindingContext);

    void ProtoOidRequestComplete(NDIS_HANDLE ProtocolBindingContext, PNDIS_OID_REQUEST OidRequest,
        NDIS_STATUS Status);

    void ProtoSendNetBufferListsComplete(NDIS_HANDLE ProtocolBindingContext, PNET_BUFFER_LIST NetBufferLists,
        ULONG SendCompleteFlags);

    void ProtoReceiveNetBufferLists(NDIS_HANDLE ProtocolBindingContext, PNET_BUFFER_LIST NetBufferLists,
        NDIS_PORT_NUMBER PortNumber,
        ULONG NumberOfNetBufferLists,
        ULONG ReceiveFlags);

    void ProtoStatusHandlerEx(NDIS_HANDLE ProtocolBindingContext, PNDIS_STATUS_INDICATION StatusIndication);

    NDIS_STATUS ProtoNetPnPEvent(NDIS_HANDLE ProtocolBindingContext, PNET_PNP_EVENT_NOTIFICATION NetPnPEventNotification);
    /*
    * We dump g_ndis_protocol_handle, which is actually a _NDIS_PROTOCOL_BLOCK structure (undocumented lol)
    *
    * g_ndis_protocol_handle _NDIS_PROTOCOL_BLOCK
    * -> Name: TCPIP
    * -> OpenQueue
    */
}

#endif //!ndis_hpp
#ifndef ndis_hpp
#define ndis_hpp

#include <ntifs.h>
#include <ntddk.h>

#define NDIS660 1
#define NDIS_PROTOCOL_DRIVER 1

#include <ndis.h>

extern NDIS_HANDLE g_ndis_protocol_handle;

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
#ifndef ndis_hpp
#define ndis_hpp

#include <ntddk.h>
#include <ndis.h>

extern NDIS_HANDLE g_NdisProtocolHandle;

NDIS_STATUS ProtoBindAdapterEx(
    _In_ NDIS_HANDLE ProtocolDriverContext,
    _In_ NDIS_HANDLE BindContext,
    _In_ PNDIS_BIND_PARAMETERS BindParameters)
{
    UNREFERENCED_PARAMETER(ProtocolDriverContext);

    NDIS_MEDIUM medium = BindParameters->MediaType;

    NDIS_OPEN_PARAMETERS openParams = { 0 };
    openParams.Header.Type = NDIS_OBJECT_TYPE_OPEN_PARAMETERS;
    openParams.Header.Revision = NDIS_OPEN_PARAMETERS_REVISION_1;
    openParams.Header.Size = NDIS_SIZEOF_OPEN_PARAMETERS_REVISION_1;

    openParams.AdapterName = BindParameters->AdapterName;
    openParams.MediumArray = &medium;
    openParams.MediumArraySize = 1;
    openParams.SelectedMediumIndex = NULL;
    openParams.FrameTypeArray = NULL;
    openParams.FrameTypeArraySize = 0;

    NDIS_HANDLE adapterHandle = NULL;

    NDIS_STATUS status = NdisOpenAdapterEx(
        g_NdisProtocolHandle,
        BindContext,         // ProtocolBindingContext - replace with your per-adapter context
        &openParams,
        BindContext,
        &adapterHandle);

    if (status == NDIS_STATUS_PENDING) {
        return NDIS_STATUS_PENDING;
    }

    if (status != NDIS_STATUS_SUCCESS) {
        return status;
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS ProtoUnbindAdapterEx(
    _In_ NDIS_HANDLE UnbindContext,
    _In_ NDIS_HANDLE ProtocolBindingContext)
{

    UNREFERENCED_PARAMETER(UnbindContext);
    UNREFERENCED_PARAMETER(ProtocolBindingContext);

    // TODO: call NdisCloseAdapterEx with your stored adapter handle
    // If it returns NDIS_STATUS_PENDING, return that and call
    // NdisCompleteUnbindAdapterEx(UnbindContext) from CloseAdapterCompleteEx.

    return NDIS_STATUS_SUCCESS;
}

VOID ProtoOpenAdapterCompleteEx(
    _In_ NDIS_HANDLE ProtocolBindingContext,
    _In_ NDIS_STATUS Status)
{
    UNREFERENCED_PARAMETER(ProtocolBindingContext);
    UNREFERENCED_PARAMETER(Status);

    // NdisOpenAdapterEx finished asynchronously.
    // Store the adapter handle, then complete the bind:
    // NdisCompleteBindAdapterEx(BindContext, Status);
}

VOID ProtoCloseAdapterCompleteEx(
    _In_ NDIS_HANDLE ProtocolBindingContext)
{
    UNREFERENCED_PARAMETER(ProtocolBindingContext);

    // NdisCloseAdapterEx finished asynchronously.
    // Free per-adapter resources, then:
    // NdisCompleteUnbindAdapterEx(UnbindContext);
}

VOID ProtoOidRequestComplete(
    _In_ NDIS_HANDLE ProtocolBindingContext,
    _In_ PNDIS_OID_REQUEST OidRequest,
    _In_ NDIS_STATUS Status)
{
    UNREFERENCED_PARAMETER(ProtocolBindingContext);
    UNREFERENCED_PARAMETER(OidRequest);
    UNREFERENCED_PARAMETER(Status);
}

VOID ProtoSendNetBufferListsComplete(
    _In_ NDIS_HANDLE ProtocolBindingContext,
    _In_ PNET_BUFFER_LIST NetBufferLists,
    _In_ ULONG SendCompleteFlags)
{
    UNREFERENCED_PARAMETER(ProtocolBindingContext);
    UNREFERENCED_PARAMETER(NetBufferLists);
    UNREFERENCED_PARAMETER(SendCompleteFlags);

    // Walk the NBL chain, check each NBL->Status, free/recycle NBLs.
}

VOID ProtoReceiveNetBufferLists(
    _In_ NDIS_HANDLE ProtocolBindingContext,
    _In_ PNET_BUFFER_LIST NetBufferLists,
    _In_ NDIS_PORT_NUMBER PortNumber,
    _In_ ULONG NumberOfNetBufferLists,
    _In_ ULONG ReceiveFlags)
{
    UNREFERENCED_PARAMETER(ProtocolBindingContext);
    UNREFERENCED_PARAMETER(NetBufferLists);
    UNREFERENCED_PARAMETER(PortNumber);
    UNREFERENCED_PARAMETER(NumberOfNetBufferLists);
    UNREFERENCED_PARAMETER(ReceiveFlags);

    // If (ReceiveFlags & NDIS_RECEIVE_FLAGS_RESOURCES):
    //   Copy data and return immediately � miniport reclaims NBLs.
    // Else:
    //   You own the NBLs. Process them, then call
    //   NdisReturnNetBufferLists(adapterHandle, NetBufferLists, 0);
}

VOID ProtoStatusHandlerEx(
    _In_ NDIS_HANDLE ProtocolBindingContext,
    _In_ PNDIS_STATUS_INDICATION StatusIndication)
{
    UNREFERENCED_PARAMETER(ProtocolBindingContext);
    UNREFERENCED_PARAMETER(StatusIndication);

    // Handle NDIS_STATUS_LINK_STATE, media connect/disconnect, etc.
}

NDIS_STATUS ProtoNetPnPEvent(
    _In_ NDIS_HANDLE ProtocolBindingContext,
    _In_ PNET_PNP_EVENT_NOTIFICATION NetPnPEventNotification)
{
    UNREFERENCED_PARAMETER(ProtocolBindingContext);
    UNREFERENCED_PARAMETER(NetPnPEventNotification);

    return NDIS_STATUS_SUCCESS;
}

/*
* We dump g_NdisProtocolHandle, which is actually a _NDIS_PROTOCOL_BLOCK structure (undocumented lol)
* 
* g_NdisProtocolHandle _NDIS_PROTOCOL_BLOCK
* -> Name: TCPIP
* -> OpenQueue 
*/



#endif //!ndis_hpp
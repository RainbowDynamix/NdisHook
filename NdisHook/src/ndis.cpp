#include <ndis.hpp>

namespace ndis_interface {
    NDIS_STATUS ProtoBindAdapterEx(NDIS_HANDLE ProtocolDriverContext, NDIS_HANDLE BindContext,
        PNDIS_BIND_PARAMETERS BindParameters) {
        UNREFERENCED_PARAMETER(ProtocolDriverContext);

        auto medium = BindParameters->MediaType;

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

        NDIS_STATUS status = NdisOpenAdapterEx(g_ndis_protocol_handle, BindContext,         // ProtocolBindingContext - replace with your per-adapter context
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

    NDIS_STATUS ProtoUnbindAdapterEx(NDIS_HANDLE UnbindContext, NDIS_HANDLE ProtocolBindingContext) {
        UNREFERENCED_PARAMETER(UnbindContext);
        UNREFERENCED_PARAMETER(ProtocolBindingContext);

        // TODO: call NdisCloseAdapterEx with your stored adapter handle
        // If it returns NDIS_STATUS_PENDING, return that and call
        // NdisCompleteUnbindAdapterEx(UnbindContext) from CloseAdapterCompleteEx.
        return NDIS_STATUS_SUCCESS;
    }

    void ProtoOpenAdapterCompleteEx(NDIS_HANDLE ProtocolBindingContext, NDIS_STATUS Status) {
        UNREFERENCED_PARAMETER(ProtocolBindingContext);
        UNREFERENCED_PARAMETER(Status);

        // NdisOpenAdapterEx finished asynchronously.
        // Store the adapter handle, then complete the bind:
        // NdisCompleteBindAdapterEx(BindContext, Status);
    }

    void ProtoCloseAdapterCompleteEx(NDIS_HANDLE ProtocolBindingContext) {
        UNREFERENCED_PARAMETER(ProtocolBindingContext);

        // NdisCloseAdapterEx finished asynchronously.
        // Free per-adapter resources, then:
        // NdisCompleteUnbindAdapterEx(UnbindContext);
    }

    void ProtoOidRequestComplete(NDIS_HANDLE ProtocolBindingContext, PNDIS_OID_REQUEST OidRequest,
        NDIS_STATUS Status) {
        UNREFERENCED_PARAMETER(ProtocolBindingContext);
        UNREFERENCED_PARAMETER(OidRequest);
        UNREFERENCED_PARAMETER(Status);
    }

    void ProtoSendNetBufferListsComplete(NDIS_HANDLE ProtocolBindingContext, PNET_BUFFER_LIST NetBufferLists,
        ULONG SendCompleteFlags) {
        UNREFERENCED_PARAMETER(ProtocolBindingContext);
        UNREFERENCED_PARAMETER(NetBufferLists);
        UNREFERENCED_PARAMETER(SendCompleteFlags);

        // Walk the NBL chain, check each NBL->Status, free/recycle NBLs.
    }

    void ProtoReceiveNetBufferLists(NDIS_HANDLE ProtocolBindingContext, PNET_BUFFER_LIST NetBufferLists,
        NDIS_PORT_NUMBER PortNumber,
        ULONG NumberOfNetBufferLists,
        ULONG ReceiveFlags) {
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

    void ProtoStatusHandlerEx(NDIS_HANDLE ProtocolBindingContext, PNDIS_STATUS_INDICATION StatusIndication) {
        UNREFERENCED_PARAMETER(ProtocolBindingContext);
        UNREFERENCED_PARAMETER(StatusIndication);

        // Handle NDIS_STATUS_LINK_STATE, media connect/disconnect, etc.
    }

    NDIS_STATUS ProtoNetPnPEvent(NDIS_HANDLE ProtocolBindingContext, PNET_PNP_EVENT_NOTIFICATION NetPnPEventNotification) {
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
}
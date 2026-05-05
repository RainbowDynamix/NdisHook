#ifndef utils_hpp
#define utils_hpp

#include <ntifs.h>
#include <ntddk.h>

NTSTATUS KillProcessById(HANDLE ProcessId)
{
    PEPROCESS Process;
    NTSTATUS status;

    status = PsLookupProcessByProcessId(ProcessId, &Process);
    if (!NT_SUCCESS(status)) {
        KdPrint(("KillProcessById: PsLookupProcessByProcessId failed for PID %llu: 0x%08X\n",
            (ULONG_PTR)ProcessId, status));
        return status;
    }

    HANDLE hProcess;
    status = ObOpenObjectByPointer(
        Process,
        OBJ_KERNEL_HANDLE,
        nullptr,
        PROCESS_ALL_ACCESS,
        *PsProcessType,
        KernelMode,
        &hProcess
    );

    if (!NT_SUCCESS(status)) {
        KdPrint(("KillProcessById: ObOpenObjectByPointer failed: 0x%08X\n", status));
        ObDereferenceObject(Process);
        return status;
    }

    status = ZwTerminateProcess(hProcess, STATUS_SUCCESS);
    if (!NT_SUCCESS(status)) {
        KdPrint(("KillProcessById: ZwTerminateProcess failed: 0x%08X\n", status));
    }

    ZwClose(hProcess);
    ObDereferenceObject(Process);
    return status;
}

#endif //!utils_hpp
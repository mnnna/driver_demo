#include "MDL.h"

NTSTATUS MmLockVaForWrite(PVOID Va, ULONG Length, PREPROTECT_CONTEXT ReprotectContext)
{   
    NTSTATUS status;
    status = STATUS_SUCCESS;

    ReprotectContext->Mdl = 0;
    ReprotectContext->Lockedva = 0;

    ReprotectContext->Mdl = IoAllocateMdl(Va, Length, FALSE, FALSE, NULL);

    if (!ReprotectContext->Mdl) {
        return STATUS_INSUFFICIENT_RESOURCES;
    };

    __try{
        MmProbeAndLockPages(ReprotectContext->Mdl, KernelMode, IoReadAccess);
    
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
    
    ReprotectContext->Lockedva = (PUCHAR)MmMapLockedPagesSpecifyCache(ReprotectContext->Mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
    if (!ReprotectContext->Lockedva) {
        IoFreeMdl(ReprotectContext->Mdl);
        ReprotectContext->Mdl = 0;
        return STATUS_UNSUCCESSFUL;
    }



    return NTSTATUS();
}

NTSTATUS MmUnlockVaForWrite(PREPROTECT_CONTEXT ReprotectContext)
{
    return NTSTATUS();
}

#include "MDL.h"

/*
    MDL  �ڴ�����ҳ�� �� MDL �����������ڴ棬 for DMA �豸������ ��
*/


NTSTATUS MmLockVaForWrite(PVOID Va, ULONG Length, PREPROTECT_CONTEXT ReprotectContext)
{   
    NTSTATUS status;
    status = STATUS_SUCCESS;

    ReprotectContext->Mdl = 0;
    ReprotectContext->Lockedva = 0;
    /*
    IoAllocateMdl��
        ���ܣ����ڷ���һ�������ڴ�ҳ��Ϣ�� MDL��Memory Descriptor List���ṹ��
        ������ͨ����Ҫ����Ҫ�������ڴ�����������ַ��Va���ͳ��ȣ�Length�����Լ������������Ƿ���丨�����Ƿ�ӷǷ�ҳ���з���ȡ�
        ʹ�ó�������Ҫ���ڴ��������ڴ�ҳ��Ϣ�� MDL �ṹ�������������ڴ�ҳ��ӳ�������
    */
    ReprotectContext->Mdl = IoAllocateMdl(Va, Length, FALSE, FALSE, NULL);

    if (!ReprotectContext->Mdl) {
        return STATUS_INSUFFICIENT_RESOURCES;
    };

    __try{
        MmProbeAndLockPages(ReprotectContext->Mdl, KernelMode, IoReadAccess); // access or  write ���ܻ���
    
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
    /*
        MmMapLockedPagesSpecifyCache��
            ���ܣ����ڽ��Ѿ��������ڴ�ҳӳ�䵽ϵͳ��ַ�ռ��У�������ӳ���������ַ��
            ��������Ҫ�����Ѿ�����õ� MDL �ṹ��ReprotectContext->Mdl����ӳ��ķ���ģʽ��KernelMode�����������ͣ�MmCached����ӳ��������ַ���Ƿ����������ȼ��ȡ�
            ʹ�ó�������Ҫ���ڽ��Ѿ��������ڴ�ҳӳ�䵽ϵͳ��ַ�ռ��У��Ա���ж�д�����ȡ�������ʹ�ó����������û��ռ�Ļ�����ӳ�䵽�ں˿ռ䣬���߽��ں˿ռ�Ļ�����ӳ�䵽�û��ռ䡣
    */
    ReprotectContext->Lockedva = (PUCHAR)MmMapLockedPagesSpecifyCache(ReprotectContext->Mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
    if (!ReprotectContext->Lockedva) {
        IoFreeMdl(ReprotectContext->Mdl);
        ReprotectContext->Mdl = 0;
        return STATUS_UNSUCCESSFUL;
    }

    status = MmProtectMdlSystemAddress(ReprotectContext->Mdl, PAGE_EXECUTE_READWRITE);
    
    if (!NT_SUCCESS(status)) {
        MmUnmapLockedPages(ReprotectContext->Lockedva, ReprotectContext->Mdl); 
        MmUnlockPages(ReprotectContext->Mdl);
        IoFreeMdl(ReprotectContext->Mdl);
        ReprotectContext->Lockedva = 0;
        ReprotectContext->Mdl = 0;
    }

    return status;
}

NTSTATUS MmUnlockVaForWrite(__out  PREPROTECT_CONTEXT ReprotectContext)
{
    NTSTATUS status;
    status = STATUS_SUCCESS;

    MmUnmapLockedPages(ReprotectContext->Lockedva, ReprotectContext->Mdl);
    MmUnlockPages(ReprotectContext->Mdl);
    IoFreeMdl(ReprotectContext->Mdl);
    ReprotectContext->Lockedva = 0;
    ReprotectContext->Mdl = 0;

    return status;
}

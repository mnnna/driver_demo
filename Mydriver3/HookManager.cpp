#include "HookManager.h"
#include<intrin.h>
#include"hde64.h"
#include"PageTable.h"

HookManager* HookManager::mInstance;

#pragma warning (disable : 4838)
#pragma warning (disable : 4309)
#pragma warning (disable : 4244)
#pragma warning (disable : 6328)
#pragma warning (disable : 6066)
#pragma warning (disable : 4996)


bool HookManager::InstallInlinehook(HANDLE pid, __inout void** originAddr, void* hookAddr)
{
    static bool bFirst = true;
    if (bFirst) {
        mTrampLinePool = (char*)ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE * 4, 'Jmp'); // ExAllocatePool2 ������������������

        if (!mTrampLinePool) {
            DbgPrint("Error InstallInlinehook");
            return false;
        };

        RtlZeroMemory(mTrampLinePool, PAGE_SIZE * 4);   
        mPoolUSED = 0;
        bFirst = false;
 
    }

    if (mHookCount == MAX_HOOK_COUNT) {
        DbgPrint("FULL");
        return false;
    };
    PEPROCESS process;
    if (!NT_SUCCESS(PsLookupProcessByProcessId(pid, &process))) return false ;
    
    if (!IsolationPageTable(process, *originAddr)) {
        ObDereferenceObject(process);
        return false;
    }


    const UINT32 trampLineByteCount = 20;
    const UINT32 fnBreakByteLeast = 12;

    /*
    push 0
    mov dword ptr ds : [rsp] , 0
    mov dword ptr ds : [rsp + 4] , 0
    */
    char TrampLineCode[trampLineByteCount] = { 
        0x6A,0x00 ,0x3E ,0xC7 ,0x04 ,0x24 ,0x00 ,0x00 ,0x00 ,
        0x00 ,0x3E ,0xC7 ,0x44 ,0x24 ,0x04 ,0x00 ,0x00 ,0x00 ,0x00 ,0xC3 };

    /*
        mov rax, 0 
        Jmp rax
    */
    char AbsoluteJmpCode[fnBreakByteLeast] = {
        0x48,0xB8,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0xFF,0xE0
    };



    char* curTrampLinePool = mTrampLinePool + mPoolUSED;
    char* startJmpAddr = (char*)*originAddr;  // ҪHOOK�������׵�ַ
    UINT32 uBreakBytes = 0; 
    hde64s hdeinfo = { 0 };

    while (uBreakBytes < fnBreakByteLeast) {
        if (!hde64_disasm(startJmpAddr + uBreakBytes, &hdeinfo)) {
            DbgPrint("hde64_disasm error \n");
            return false;
        };
        uBreakBytes += hdeinfo.len;
    };

    *(PUINT32)&TrampLineCode[6] = (UINT32)((UINT64)(startJmpAddr + uBreakBytes) & 0xFFFFFFFF); // ȡ��λ
    *(PUINT32)&TrampLineCode[15] = (UINT32)((UINT64)(startJmpAddr + uBreakBytes)>>32 & 0xFFFFFFFF); //ȡ��λ

    memcpy(curTrampLinePool, startJmpAddr, uBreakBytes); //����ԭ������ ����
    memcpy(curTrampLinePool + uBreakBytes, TrampLineCode, trampLineByteCount);  //return ���


    for (int i = 0; i < MAX_HOOK_COUNT; i++) {
        if (mHookInfo[i].pid != pid) {
            mHookInfo[i].pid = pid; 
            mHookInfo[i].originAddr = startJmpAddr;
            memcpy(mHookInfo[i].originBytes, startJmpAddr, uBreakBytes);
            mHookCount++;
            break;
        }
    }

    *(void**)&AbsoluteJmpCode[2] = hookAddr; // �����ַתλһ��ָ�룺���鱾����ǵ�ַ��& ȡһ��ֵ�ͱ���˶���ָ�룬 �� * ȡһ��ֵ
    REPROTECT_CONTEXT Content = { 0 };

    KAPC_STATE apc;
    KeStackAttachProcess(process, &apc);
    if (!NT_SUCCESS(MmLockVaForWrite(startJmpAddr, PAGE_SIZE, &Content))) {
        return false;
    }

    RtlCopyMemory(Content.Lockedva, AbsoluteJmpCode, fnBreakByteLeast);
    
    if (!NT_SUCCESS(MmUnlockVaForWrite(&Content))) {
        return false;
    }

    KeUnstackDetachProcess(&apc);


    *originAddr = curTrampLinePool;
    mPoolUSED += (uBreakBytes + trampLineByteCount);
    ObDereferenceObject(process);
    return true;
}

bool HookManager::RemoveInlinehook(HANDLE pid, void* hookAddr)
{
    pid;
    UNREFERENCED_PARAMETER(hookAddr);
    return false;
}

HookManager* HookManager::GetInstance()
{
    if (mInstance == nullptr) {
        mInstance = (HookManager*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HookManager), 'test');
    }
    return mInstance;
}

bool HookManager::IsolationPageTable(PEPROCESS process, void* isolateioAddr)
{
    bool bRet = false;
    KAPC_STATE apc; 
    KeStackAttachProcess(process, &apc);
    pde_64 NewPde = { 0 };
    void* alignAddrr; // ?? 

    alignAddrr= PAGE_ALIGN(isolateioAddr); // 0x1000 ����
    PAGE_TABLE page_table = { 0 };
    page_table.VirtualAddress = alignAddrr;
    GetPageTable(page_table);

    while (true) {
        if (page_table.Entry.Pde->large_page) {
            DbgPrint("size is 2MB \n");
            bRet = SplitLargePage(*page_table.Entry.Pde, NewPde);
            if (!bRet) break;
        }
        else if (page_table.Entry.Pdpte->large_page) {
            DbgPrint("size is 1GB \n");
            break;
        }
        else {
            DbgPrint("size is 4KB \n");
            break;
        }
        cr3 Cr3; 
        Cr3.flags = __readcr3();
        bRet = ReplacePageTable(Cr3, alignAddrr, &NewPde);

        if (bRet) {
            DbgPrint("isolation successfully \n");
        }
        else {
            DbgPrint("Failed isolation \n");
        }
    }

    KeUnstackDetachProcess(&apc);

    return bRet;
}

bool HookManager::SplitLargePage(pde_64 InPde, pde_64& OutPde)
{
    PHYSICAL_ADDRESS MaxAddrPA{ 0 }, LowAddrPa{ 0 }; 
    MaxAddrPA.QuadPart = MAXULONG64;
    LowAddrPa.QuadPart =  0 ;
    pt_entry_64* Pt;
    uint64_t StartPfn  =  InPde.page_frame_number;

    Pt = (pt_entry_64*)MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, LowAddrPa, MaxAddrPA, LowAddrPa, MmCached); // ��MmAllocateContiguousMemory �� 
    if (!Pt) {
        DbgPrint("failed to MmAllocateContiguousMemorySpecifyCache");
        return false;
    }

    for (int i = 0; i < 512; i++) {
        Pt[i].flags = InPde.flags;
        Pt[i].large_page = 0;
        Pt[i].page_frame_number = StartPfn + i;
    }

    OutPde.flags = InPde.flags;
    OutPde.large_page = 0; 
    OutPde.page_frame_number = VaToPa(Pt) / PAGE_SIZE;
    return true;
}

bool HookManager::ReplacePageTable(cr3 cr3, void* replaceAlignAddr, pde_64* pde)
{
    uint64_t *Va4kb, *Vapt, *VaPdt, *VaPdpt, *VaPml4t;
    PHYSICAL_ADDRESS MaxAddrPA{ 0 }, LowAddrPa{ 0 };
    MaxAddrPA.QuadPart = MAXULONG64;
    LowAddrPa.QuadPart = 0;
    PAGE_TABLE pagetable = { 0 };

    Va4kb = (uint64_t*)MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, LowAddrPa, MaxAddrPA, LowAddrPa, MmCached);
    Vapt = (uint64_t*)MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, LowAddrPa, MaxAddrPA, LowAddrPa, MmCached);
    VaPdt = (uint64_t*)MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, LowAddrPa, MaxAddrPA, LowAddrPa, MmCached);
    VaPdpt = (uint64_t*)MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, LowAddrPa, MaxAddrPA, LowAddrPa, MmCached);

    VaPml4t = (uint64_t*)PaToVa(cr3.address_of_page_directory * PAGE_SIZE);

    if (!Va4kb || !Vapt || !VaPdt || !VaPdpt) {
        DbgPrint(" Apply mm failed");
        return false;
    }

    pagetable.VirtualAddress = replaceAlignAddr;
    GetPageTable(pagetable);

    UINT64 pml4eindex = ((UINT64)replaceAlignAddr & 0xFF8000000000) >> 39;
    UINT64 pdpteindex = ((UINT64)replaceAlignAddr & 0x7FC0000000) >> 30;
    UINT64 pdeindex = ((UINT64)replaceAlignAddr & 0x3FE00000) >> 21;
    UINT64 pteindex = ((UINT64)replaceAlignAddr & 0x1FF000) >> 12;
     
    if (pagetable.Entry.Pde->large_page) {
        MmFreeContiguousMemorySpecifyCache(Vapt, PAGE_SIZE, MmCached);
        Vapt = (uint64_t*)PaToVa(pde->page_frame_number * PAGE_SIZE);
    }
    else {
        memcpy(Vapt, pagetable.Entry.Pte - pteindex, PAGE_SIZE);
    }
    memcpy(Va4kb, replaceAlignAddr, PAGE_SIZE);
    memcpy(Va4kb, pagetable.Entry.Pde - pdeindex, PAGE_SIZE);
    memcpy(VaPdpt, pagetable.Entry.Pdpte - pdpteindex, PAGE_SIZE);

    auto pReplacePte = (pte_64*) &Vapt[pteindex]; // & 
    pReplacePte->page_frame_number = VaToPa(Va4kb) / PAGE_SIZE;

    auto pReplacePde = (pde_64*)&VaPdt[pdeindex]; // & 
    pReplacePde->page_frame_number = VaToPa(Vapt) / PAGE_SIZE;
    pReplacePde->large_page = 0;

    auto pReplacePdpte = (pdpte_64*)&VaPdpt[pdpteindex]; // & 
    pReplacePdpte->page_frame_number = VaToPa(VaPdt) / PAGE_SIZE;

    auto pReplacePml4e = (pml4e_64*)&VaPml4t[pml4eindex]; // & 
    pReplacePml4e->page_frame_number = VaToPa(VaPdpt) / PAGE_SIZE;


    return true;
}

ULONG64 HookManager::VaToPa(void* va)
{
    PHYSICAL_ADDRESS pa; 
    pa = MmGetPhysicalAddress(va);
    return pa.QuadPart;
}

void* HookManager::PaToVa(ULONG64 pa)
{
    PHYSICAL_ADDRESS Pa{ 0 };
    Pa.QuadPart = pa;
    
    return MmGetVirtualForPhysical(Pa);
}

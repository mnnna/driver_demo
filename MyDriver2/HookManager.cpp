#include "HookManager.h"
#include"hde64.h"

HookManager* HookManager::mInstance;

bool HookManager::InstallInlinehook(void** originAddr, void* hookAddr)
{
    static bool bFrist = true;
    if (bFrist) {
        mTrampLinePool = (char*)ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE * 4, 'Jmp');

        if (!mTrampLinePool) {
            DbgPrint("Error InstallInlinehook");
            return false;
        };

        RtlSecureZeroMemory(mTrampLinePool, PAGE_SIZE * 4);   
        mPoolUSED = 0;
        bFrist = false;
        return true;
    }

    if (mHookCount == MAX_HOOK_COUNT) {
        DbgPrint("FULL");
        return false;
    };

    const UINT32 trampLineByteCount = 30;
    const UINT32 fnBreakByteLeast = 12;

    /*
    push 0
    mov dword ptr ds : [rsp] , 0
    mov dword ptr ds : [rsp + 4] , 0
    */
    char TrampLineCode[trampLineByteCount] = { 0x6A,0x00 ,0x3E ,0xC7 ,0x04 ,0x24 ,0x00 ,0x00 ,0x00 ,0x00 ,0x3E ,0xC7 ,0x44 ,0x24 ,0x04 ,0x00 ,0x00 ,0x00 ,0x00 ,0xC3 };

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
        uBreakBytes = uBreakBytes + hdeinfo.len;
    };

    *(PUINT32)&TrampLineCode[6] = (UINT32)((UINT64)(startJmpAddr + uBreakBytes) & 0xFFFFFFFF);
    *(PUINT32)&TrampLineCode[15] = (UINT32)((UINT64)(startJmpAddr + uBreakBytes)>>32 & 0xFFFFFFFF);

    memcpy(curTrampLinePool, startJmpAddr, uBreakBytes); //����ԭ������ ����
    memcpy(curTrampLinePool + uBreakBytes, TrampLineCode, trampLineByteCount);  //return ���

    *(void **)&AbsoluteJmpCode[2] = hookAddr // �����ַתλһ��ָ�룺���鱾����ǵ�ַ��& ȡһ��ֵ�ͱ���˶���ָ�룬 �� * ȡһ��ֵ

}

bool HookManager::RemoveInlinehook(void* hookAddr)
{

    return false;
}

HookManager* HookManager::GetInstance()
{
    if (mInstance == nullptr) {
        mInstance = (HookManager*) ExAllocatePoolWithTag(NonPagedPool, sizeof(HookManager), 'test');
    }
    return mInstance;
}


#define WIN32_LEAN_AND_MEAN             
#include <windows.h>
#include <string>

char buffer[200];

BOOL(__cdecl* HookFunction)(ULONG_PTR OriginalFunction, ULONG_PTR NewFunction);
VOID(__cdecl* UnhookFunction)(ULONG_PTR Function);
ULONG_PTR(__cdecl* GetOriginalFunction)(ULONG_PTR Hook);

HMODULE lib_ptr = NULL;

    
#pragma comment(linker, "/EXPORT:?is_global_id_enabled@DEV_DIE_INFO@@QEBA_NI@Z=is_global_id_enabled_patch")
extern "C" bool __cdecl is_global_id_enabled_patch(void* db, unsigned int id) 
{
    OutputDebugString(L"in patched is_global_id");

    if (id == 1174964) // global id of forbidden PCIe block
       return true;

    bool (*is_global_id_enabled_orig)(void* db, unsigned id) = (bool(__cdecl*)(void*, unsigned int))GetOriginalFunction((ULONG_PTR)is_global_id_enabled_patch);

    OutputDebugString(L"calling original is_global_id");
    bool next_result = is_global_id_enabled_orig(db, id);
    OutputDebugString(L"returned from original is_global_id");
    if (!next_result)
    {   // something else disabled, print it for interest's sake
        sprintf_s(buffer, "disabled %u\n", id);
        OutputDebugStringA(buffer);
    }

    OutputDebugString(L"returning from patched is_global_id");

    return next_result;
}

void patchLibrary() {
    if (lib_ptr != NULL)
        return;

    lib_ptr = LoadLibraryEx(L"ddb_dev_orig.dll", NULL, 0);
    if (lib_ptr == NULL) {
        sprintf_s(buffer, "failed to open ddb_dev_orig.dll\n");
        OutputDebugStringA(buffer);
        return;
    }

    OutputDebugString(L"Loaded ddb_dev_orig.dll");

    FARPROC orig_func = GetProcAddress(lib_ptr, "?is_global_id_enabled@DEV_DIE_INFO@@QEBA_NI@Z");
    if (orig_func == NULL) {
        sprintf_s(buffer, "failed to find function ?is_global_id_enabled@DEV_DIE_INFO@@QEBA_NI@Z\n");
        OutputDebugStringA(buffer);
        return;
    }

    sprintf_s(buffer, "found global_id_func at %p - patch is at %p", (VOID*)orig_func, (VOID*)is_global_id_enabled_patch);
    OutputDebugStringA(buffer);

    sprintf_s(buffer, "HookFunction is at %p", (VOID*)HookFunction);
    OutputDebugStringA(buffer); 

    if (!HookFunction((ULONG_PTR)orig_func, (ULONG_PTR)is_global_id_enabled_patch))
    {
        sprintf_s(buffer, "failed to hook function ?is_global_id_enabled@DEV_DIE_INFO@@QEBA_NI@Z\n");
        OutputDebugStringA(buffer);
        return;
    }

    OutputDebugString(L"hooked global_id function");
}

BOOL setupHookLib()
{
    HMODULE hHook = LoadLibraryEx(L"NtHookEngine.dll", NULL, 0);
    if (hHook == NULL) {
        sprintf_s(buffer, "failed to open NtHookEngine.dll\n");
        OutputDebugStringA(buffer);
        return false;
    }

    OutputDebugString(L"Loaded NtHookEngine.dll");

    FARPROC fHook = GetProcAddress(hHook, "HookFunction");
    FARPROC fFindOrig = GetProcAddress(hHook, "GetOriginalFunction");
    FARPROC fUnhook = GetProcAddress(hHook, "UnhookFunction");
    if (fHook == NULL || fFindOrig == NULL || fUnhook == NULL) {
        sprintf_s(buffer, "failed to find hooking function\n");
        OutputDebugStringA(buffer);
        return false;
    }

    HookFunction = (BOOL(_cdecl*)(ULONG_PTR, ULONG_PTR))fHook;
    UnhookFunction = (void(__cdecl*)(ULONG_PTR))fUnhook;
    GetOriginalFunction = (ULONG_PTR(__cdecl*)(ULONG_PTR))fFindOrig;

    OutputDebugString(L"Fixed hooking infra\n");

    return true;
}

DWORD WINAPI ThreadMain(LPVOID lpParam)
{
    OutputDebugString(L"setup hooking start");
    if (setupHookLib())
    {
        patchLibrary();
    }
    OutputDebugString(L"setup hooking finished");

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        {
            OutputDebugString(L"ddb_dev wrapper - process attach start");
            DWORD dwThreadId;
            HANDLE hThread = CreateThread(NULL, 0, ThreadMain, NULL, 0, &dwThreadId);
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


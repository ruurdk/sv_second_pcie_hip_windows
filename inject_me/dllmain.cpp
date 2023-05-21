// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include <stdio.h>
#include <winbase.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "dbghelp.h"

HMODULE lib_ptr = NULL;
char buffer[200];

BOOL(__cdecl* HookFunction)(ULONG_PTR OriginalFunction, ULONG_PTR NewFunction);
VOID(__cdecl* UnhookFunction)(ULONG_PTR Function);
ULONG_PTR(__cdecl* GetOriginalFunction)(ULONG_PTR Hook);

bool is_global_id_enabled_patch(void* db, unsigned id) 
{
    OutputDebugString(L"in patched is_global_id");

    if (id == 1174964) // global id of forbidden PCIe block
        return 1;

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

    lib_ptr = LoadLibraryEx(L"ddb_dev.dll", NULL, 0);
    if (lib_ptr == NULL) {
        sprintf_s(buffer, "failed to open ddb_dev.dll\n");
        OutputDebugStringA(buffer);
        return;
    }

    OutputDebugString(L"Loaded ddb_dev.dll");

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

    if (HookFunction == NULL)
    {
        OutputDebugString(L"Something went wrong in linking, HookFunction is NULL");
        return;
    }

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
    OutputDebugString(L"inject_me start");
    if (setupHookLib())
    {
        patchLibrary();
    }
    OutputDebugString(L"inject_me finished");

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
                OutputDebugString(L"inject_me - process attach start");
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

/*
void getAllExports(HMODULE lib)
{
    HANDLE hFile = CreateFile(original_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "failed to open ddb_dev_orig.dll for getting exports\n");
        exit(1);
    }
    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY | SEC_IMAGE_NO_EXECUTE, 0, 0, NULL);
    if (hMapping == NULL)
    {
        fprintf(stderr, "failed to create file mapping\n");
        exit(1);
    }

    LPVOID lpMap = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (lpMap == NULL)
    {
        fprintf(stderr, "failed to map view of file\n");
        exit(1);
    }

    IMAGE_NT_HEADERS* pHeader = ImageNtHeader(lpMap);
    if (pHeader == NULL)
    {
        fprintf(stderr, "failed to parse headers\n");
        exit(1);
    }
    
    IMAGE_EXPORT_DIRECTORY* lpExports = ImageRvaToVa(pHeader, lpMap, pHeader->OptionalHeader.DataDirectory[0].VirtualAddress, NULL);
    if (lpExports == NULL)
    {
        fprintf(stderr, "failed to find exports\n");
        exit(1);
    }
    LPVOID lpNames = ImageRvaToVa(pHeader, lpMap, lpExports->AddressOfNames, NULL);
    if (lpNames == NULL) 
    {
        fprintf(stderr, "failed to find names\n");
        exit(1);
    }

    for (int i = 0; i < lpExports->NumberOfNames; i++)
    {
        LPVOID lpName = ImageRvaToVa(pHeader, lpMap, lpNames, NULL);

    }

    UnmapViewOfFile(lpMap);
    CloseHandle(hMapping);
    CloseHandle(hFile);
}
*/
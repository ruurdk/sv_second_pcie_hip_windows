// inject.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

LPCWSTR lpcwszInjectDllName = L"inject_me.dll";
LPCWSTR exeName = L"quartus.exe";


void printLastError()
{
	wprintf(L"Error 0x%x\n", GetLastError());
}

// find process ID by process name
int findProcessId(const wchar_t* procname) {

	int pid = 0;	

	printf("Finding running process\n");

	// snapshot of all processes in the system
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hSnapshot) return 0;

	// initializing size: needed for using Process32First
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);

	// info about first process encountered in a system snapshot
	BOOL hResult = Process32First(hSnapshot, &pe);

	// retrieve information about the processes
	// and exit if unsuccessful
	while (hResult) {
		// if we find the process: return process ID
		if (wcscmp(procname, pe.szExeFile) == 0) {
			pid = pe.th32ProcessID;
			break;
		}
		hResult = Process32Next(hSnapshot, &pe);
	}

	// closes an open handle (CreateToolhelp32Snapshot)
	CloseHandle(hSnapshot);
	return pid;
}

int inject(DWORD processId)
{
	printf("Process id: %i\n", processId);
	size_t nInjectDllNameLength = wcslen(lpcwszInjectDllName) * sizeof(WCHAR);

	// handle to kernel32 and pass it to GetProcAddress - will be at the same address in all processes
	FARPROC lpRemoteLoadLibrary = GetProcAddress(GetModuleHandle(L"KERNEL32.DLL"), "LoadLibraryW");
	if (lpRemoteLoadLibrary == NULL)
	{
		printf("Failed to get address of LoadLibraryW\n");
		printLastError();
		return 1;
	}

	HANDLE hRemoteProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	if (hRemoteProcess == NULL)
	{
		printf("Failed to open remote process\n");
		printLastError();
		return 1;
	}

	// allocate memory buffer for remote process
	LPVOID lpRemoteString = VirtualAllocEx(hRemoteProcess, NULL, nInjectDllNameLength + 1, (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);
	if (lpRemoteString == NULL)
	{
		printf("Failed to allocate memory in remote process\n");
		printLastError();
		CloseHandle(hRemoteProcess);
		return 1;
	}

	// "copy" dll name into remote process
	if (WriteProcessMemory(hRemoteProcess, lpRemoteString, lpcwszInjectDllName, nInjectDllNameLength, NULL) == 0)
	{
		printf("Failed to write dll name to remote process\n");
		printLastError();
		VirtualFreeEx(hRemoteProcess, lpRemoteString, 0, MEM_RELEASE);
		CloseHandle(hRemoteProcess);
		return 1;
	}

	// our process start new thread
	HANDLE hRemoteThread = CreateRemoteThread(hRemoteProcess, NULL, 0, (LPTHREAD_START_ROUTINE)lpRemoteLoadLibrary, lpRemoteString, 0, NULL);
	if (hRemoteThread == NULL)
	{
		printf("Failed to create remote thread\n");
		printLastError();
		VirtualFreeEx(hRemoteProcess, lpRemoteString, 0, MEM_RELEASE);
		CloseHandle(hRemoteProcess);
		return 1;
	}
	
	WaitForSingleObject(hRemoteThread, 4000);
	//resume suspended thread
	ResumeThread(hRemoteThread);	

	VirtualFreeEx(hRemoteProcess, lpRemoteString, 0, MEM_RELEASE);
	CloseHandle(hRemoteProcess);

	printf("Hook succesfull.\n");
	return 0;
}

int main()
{
	OutputDebugString(L"Inject - start\n");

	/*
	HMODULE hTest = LoadLibrary(lpcwszInjectDllName);
	if (hTest == NULL)
	{
		printLastError();
	}
	*/
	int pId = findProcessId(exeName);
	if (pId == 0)
	{
		printf("Failed to find exe process\n");
		exit(1);
	}
	return inject(pId);
}

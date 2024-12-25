#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <thread>
#include <chrono>

DWORD GetProcessIdByName(const wchar_t* processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (wcscmp(pe32.szExeFile, processName) == 0) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return 0;
}

bool InjectDLL(DWORD processId, const wchar_t* dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) return false;

    void* allocatedMemory = VirtualAllocEx(hProcess, nullptr, (wcslen(dllPath) + 1) * sizeof(wchar_t), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!allocatedMemory) {
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, allocatedMemory, dllPath, (wcslen(dllPath) + 1) * sizeof(wchar_t), nullptr)) {
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, allocatedMemory, 0, nullptr);
    if (!hThread) {
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return true;
}

int main() {
    const wchar_t* processName = L"PROCESS NAME HERE.exe";
    const wchar_t* dllName = L"DLL NAME HERE.dll";

    wchar_t dllPath[MAX_PATH];
    if (!GetFullPathNameW(dllName, MAX_PATH, dllPath, nullptr)) {
        std::wcerr << L"[Error] Failed to get full path of " << dllName << std::endl;
        return 1;
    }

    std::wcout << L"Searching for process: " << processName << std::endl;
    DWORD processId = GetProcessIdByName(processName);

    if (!processId) {
        std::wcerr << L"[Error] Process not found: " << processName << std::endl;
        return 1;
    }

    std::wcout << L"Process found: " << processName << L" (PID: " << processId << L")" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(6)); // await of 6 seconds to make it look cooler lol, remove this part if u dont want cooldown.

    std::wcout << L"Injecting DLL: " << dllPath << std::endl;
    if (InjectDLL(processId, dllPath)) {
        std::wcout << L"[Success] DLL injected successfully!" << std::endl;
    }
    else {
        std::wcerr << L"[Error] DLL injection failed." << std::endl;
        return 1;
    }

    std::wcout << L"Press any key to exit..." << std::endl;
    std::cin.get();

    return 0;
}

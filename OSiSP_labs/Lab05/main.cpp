#include <windows.h>
#include <iostream>
#include <locale>

HANDLE mutex;
HANDLE outputMutex;
int totalChanges = 0;

struct Args
{
    HKEY hKey;
    LPCWSTR subKey;
    LPCWSTR valueName;
    LPCWSTR taskName;
};

void ChangeTotalChanges(LPCWSTR taskName) {
    DWORD waitResult;
    waitResult = WaitForSingleObject(
        mutex,    // handle to mutex
        INFINITE);  // no time-out interval
    switch (waitResult)
    {
    case WAIT_OBJECT_0:
        __try {
            totalChanges++;
            WaitForSingleObject(
                outputMutex,    // handle to mutex
                INFINITE);  // no time-out interval
            std::wcout << taskName << L": changed total changes to " << totalChanges << std::endl;
            ReleaseMutex(outputMutex);
        }

        __finally {
            ReleaseMutex(mutex);
        }
        break;

        // The thread got ownership of an abandoned mutex
    case WAIT_ABANDONED:
        return;
    }
}

void MonitorRegistryChanges(LPVOID lpParam) {
    Args* args = static_cast<Args*>(lpParam);

    HKEY hKey = args->hKey;
    LPCWSTR subKey = args->subKey;
    LPCWSTR valueName = args->valueName;
    LPCWSTR taskName = args->taskName;

    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvent == NULL) {
        std::cerr << "Failed to create event. Error code: " << GetLastError() << std::endl;
        return;
    }

    RegNotifyChangeKeyValue(hKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);

    DWORD lastValueSize = 0;
    WCHAR lastValue[MAX_PATH] = L"";
    DWORD lastValueCount = 0;

    while (true) {
        DWORD waitResult = WaitForSingleObject(hEvent, INFINITE);

        if (waitResult == WAIT_OBJECT_0) {
            DWORD dataSize;
            WCHAR data[MAX_PATH];
            if (wcscmp(valueName, L"") == 0)
            {
                LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, subKey, 0, KEY_READ, &hKey);

                if (result == ERROR_SUCCESS) {
                    DWORD currentValueCount;
                    if (RegQueryInfoKey(hKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &currentValueCount, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                        if (currentValueCount != lastValueCount) {
                            WaitForSingleObject(
                                outputMutex,    // handle to mutex
                                INFINITE);  // no time-out interval

                            std::wcout << taskName << L": Values count has changed: " << lastValueCount << " >> " << currentValueCount << std::endl;

                            ReleaseMutex(outputMutex);

                            lastValueCount = currentValueCount;

                            ChangeTotalChanges(taskName);
                        }
                    }
                }
                RegCloseKey(hKey);
            }
            else if (RegGetValue(hKey, subKey, valueName, RRF_RT_REG_SZ, NULL, data, &dataSize) == ERROR_SUCCESS) {
                if (wcscmp(data, lastValue) != 0) {
                    WaitForSingleObject(
                        outputMutex,    // handle to mutex
                        INFINITE);  // no time-out interval

                    std::wcout << taskName << L": New value: " << data << std::endl;

                    ReleaseMutex(outputMutex);

                    wcscpy_s(lastValue, data);
                    ChangeTotalChanges(taskName);
                }
            }

            RegNotifyChangeKeyValue(hKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);
        }
        else if (waitResult == WAIT_FAILED) {
            std::cerr << "WaitForSingleObject failed. Error code: " << GetLastError() << std::endl;
            break;
        }
    }

    CloseHandle(hEvent);
}

int main() {
    std::setlocale(LC_ALL, "Russian");

    mutex = CreateMutex(
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex

    outputMutex = CreateMutex(
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex

    Args* args1 = new Args;
    Args* args2 = new Args;

    args1->hKey = HKEY_CURRENT_USER;
    args1->subKey = L"Control Panel\\Desktop";
    args1->valueName = L"Wallpaper";
    args1->taskName = L"Background";

    args2->hKey = HKEY_CURRENT_USER;
    args2->subKey = L"Keyboard Layout\\Preload";
    args2->valueName = L"";
    args2->taskName = L"Keyboard layouts";

    HANDLE thread1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MonitorRegistryChanges, (LPVOID)args1, 0, NULL);
    HANDLE thread2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MonitorRegistryChanges, (LPVOID)args2, 0, NULL);

    WaitForSingleObject(thread1, INFINITE);
    WaitForSingleObject(thread2, INFINITE);

    return 0;
}

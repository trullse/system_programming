#include <windows.h>
#include <vector>

std::vector<int> cpuUsageHistory;
std::vector<int> memoryUsageHistory;
bool isStopped = false;

static float CalculateCPULoad();
static unsigned long long FileTimeToInt64();
float GetCPULoad();

ULARGE_INTEGER lastKernelTime, lastUserTime, lastIdleTime;

HBRUSH hCpuBrush;
HBRUSH hMemoryBrush;
HBRUSH hRectBrush;
HWND hProcessor;
HWND hMemory;
int startSpace = 200;

const int maxHistorySize = 100;

DWORD UpdateMemoryData(LPVOID lpParam) {
    while (!isStopped) {
        MEMORYSTATUSEX memoryStatus;
        memoryStatus.dwLength = sizeof(memoryStatus);
        GlobalMemoryStatusEx(&memoryStatus);

        auto total = memoryStatus.ullTotalPhys / 1024 / 1024;
        auto available = memoryStatus.ullAvailPhys / 1024 / 1024;
        int memoryUsage = ((float)total - (float)available) / (float)total * 100;

        TCHAR buffer2[256];
        wsprintf(buffer2, L"Memory usage is %d o/o", memoryUsage);
        SetWindowText(hMemory, buffer2);

        if (memoryUsageHistory.size() >= maxHistorySize) {
            memoryUsageHistory.erase(memoryUsageHistory.begin());
        }

        memoryUsageHistory.push_back(memoryUsage);

        Sleep(1000);
    }
    ExitThread(0);
    return 1;
}

DWORD UpdatePerformanceData(LPVOID lpParam) {
    while (!isStopped) {

        int cpuUsage = GetCPULoad() * 100;

        TCHAR buffer[256];
        wsprintf(buffer, L"Cpu usage is %d o/o", cpuUsage);
        SetWindowText(hProcessor, buffer);

        if (cpuUsageHistory.size() >= maxHistorySize) {
            cpuUsageHistory.erase(cpuUsageHistory.begin());
        }

        cpuUsageHistory.push_back(cpuUsage);

        Sleep(1000);
    }
    ExitThread(0);
    return 1;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(255, 255, 255));

    switch (uMsg) {
    case WM_CREATE:
        SetTimer(hwnd, 1, 1000, NULL);
        hCpuBrush = CreateSolidBrush(RGB(255, 0, 0));
        hMemoryBrush = CreateSolidBrush(RGB(0, 0, 255));
        hRectBrush = CreateSolidBrush(RGB(255, 255, 255));
        hProcessor = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 10, 100, 180, 20, hwnd, (HMENU)1001, NULL, NULL);
        hMemory = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 10, 300, 180, 20, hwnd, (HMENU)1002, NULL, NULL);
        break;

    case WM_TIMER:
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        SelectObject(hdc, hRectBrush);
        Rectangle(hdc, startSpace, 200, startSpace + 500, 100);
        Rectangle(hdc, startSpace, 400, startSpace + 500, 300);

        // CPU
        SelectObject(hdc, hCpuBrush);
        for (int i = 1; i < cpuUsageHistory.size(); ++i) {
            Rectangle(hdc, i * 5 + startSpace, 200, (i + 1) * 5 + startSpace, 200 - cpuUsageHistory[i]);
        }

        // Memory
        SelectObject(hdc, hMemoryBrush);
        for (int i = 1; i < memoryUsageHistory.size(); ++i) {
            Rectangle(hdc, i * 5 + startSpace, 400, (i + 1) * 5 + startSpace, 400 - memoryUsageHistory[i]);
        }

        EndPaint(hwnd, &ps);
    }
    break;

    case WM_DESTROY:
        isStopped = true;
        KillTimer(hwnd, 1);
        DeleteObject(hCpuBrush);
        DeleteObject(hMemoryBrush);
        DeleteObject(hCpuBrush);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"Performance";

    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Performance",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    HANDLE cpuThread = CreateThread(
        NULL,                   // default security attributes
        0,                      // use default stack size  
        UpdatePerformanceData,       // thread function name
        NULL,          // argument to thread function 
        0,                      // use default creation flags 
        NULL);

    HANDLE memoryThread = CreateThread(
        NULL,                   // default security attributes
        0,                      // use default stack size  
        UpdateMemoryData,       // thread function name
        NULL,          // argument to thread function 
        0,                      // use default creation flags 
        NULL);

    SetThreadPriority(cpuThread, THREAD_PRIORITY_HIGHEST);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

static float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks)
{
    static unsigned long long _previousTotalTicks = 0;
    static unsigned long long _previousIdleTicks = 0;

    unsigned long long totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
    unsigned long long idleTicksSinceLastTime = idleTicks - _previousIdleTicks;


    float ret = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);

    _previousTotalTicks = totalTicks;
    _previousIdleTicks = idleTicks;
    return ret;
}

static unsigned long long FileTimeToInt64(const FILETIME& ft)
{
    return (((unsigned long long)(ft.dwHighDateTime)) << 32) | ((unsigned long long)ft.dwLowDateTime);
}

float GetCPULoad()
{
    FILETIME idleTime, kernelTime, userTime;
    return GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime) + FileTimeToInt64(userTime)) : -1.0f;
}
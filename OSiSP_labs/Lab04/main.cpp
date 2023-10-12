#include <windows.h>
#include <vector>
#include <thread>
#include <mutex>

std::vector<int> cpuUsageHistory;
std::vector<int> memoryUsageHistory;
std::thread dataThread;
std::mutex mtx;
bool isStopped = false;

HBRUSH hCpuBrush;
HBRUSH hMemoryBrush;

void UpdatePerformanceData() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        if (isStopped)
            break;

        MEMORYSTATUSEX memoryStatus;
        memoryStatus.dwLength = sizeof(memoryStatus);
        GlobalMemoryStatusEx(&memoryStatus);

        int cpuUsage = 0;

        auto total = memoryStatus.ullTotalPhys / 1024 / 1024;
        auto available = memoryStatus.ullAvailPhys / 1024 / 1024;
        int memoryUsage = ((float)total - (float)available) / (float)total * 100;

        const int maxHistorySize = 100;
        if (cpuUsageHistory.size() >= maxHistorySize) {
            cpuUsageHistory.erase(cpuUsageHistory.begin());
        }
        if (memoryUsageHistory.size() >= maxHistorySize) {
            memoryUsageHistory.erase(memoryUsageHistory.begin());
        }

        cpuUsageHistory.push_back(cpuUsage);
        memoryUsageHistory.push_back(memoryUsage);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(255, 255, 255));

    switch (uMsg) {
    case WM_CREATE:
        SetTimer(hwnd, 1, 1000, NULL);
        hCpuBrush = CreateSolidBrush(RGB(255, 0, 0));
        hMemoryBrush = CreateSolidBrush(RGB(0, 0, 255));
        break;

    case WM_TIMER:
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // CPU
        SelectObject(hdc, hCpuBrush);
        for (int i = 1; i < cpuUsageHistory.size(); ++i) {
            Rectangle(hdc, i * 5, 200, (i + 1) * 5, 200 - cpuUsageHistory[i]);
        }

        // Memory
        SelectObject(hdc, hMemoryBrush);
        for (int i = 1; i < memoryUsageHistory.size(); ++i) {
            Rectangle(hdc, i * 5, 400, (i + 1) * 5, 400 - memoryUsageHistory[i]);
        }

        EndPaint(hwnd, &ps);
    }
    break;

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        DeleteObject(hCpuBrush);
        DeleteObject(hMemoryBrush);
        {
            std::lock_guard<std::mutex> lock(mtx);
            isStopped = true;
        }
        dataThread.join();
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

    dataThread = std::thread(UpdatePerformanceData);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

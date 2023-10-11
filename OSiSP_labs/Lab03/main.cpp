#include <Windows.h>
#include <tchar.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <filesystem>

namespace fs = std::filesystem;

#define ID_BUTTON_START 1001
#define ID_BUTTON_PAUSE 1002

HWND hwndButtonStart, hwndButtonPauseResume, hwndEditSource, hwndEditDestination;
HANDLE hThread;
std::mutex mtx;
std::condition_variable cv;
bool isPaused = false;

TCHAR sourcePath[MAX_PATH], destinationPath[MAX_PATH];

void SetButtonsState(BOOL isCopying) {
    EnableWindow(hwndButtonStart, !isCopying);
    EnableWindow(hwndButtonPauseResume, isCopying);
}

void CopyFileAsync(LPCTSTR sourcePath, LPCTSTR destinationPath) {

    SetButtonsState(TRUE);

    HANDLE hSourceFile = CreateFile(sourcePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSourceFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        LPWSTR errorMessage;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            error,
            0,
            (LPWSTR)&errorMessage,
            0,
            NULL
        );

        MessageBox(NULL, errorMessage, _T("Ошибка"), MB_OK | MB_ICONERROR);
        LocalFree(errorMessage);

        SetButtonsState(FALSE);
        return;
    }

    fs::path path(sourcePath);
    std::wstring fileName = path.filename().wstring();
    fs::path destinationDirectory = fs::path(destinationPath) / fileName;
    destinationPath = destinationDirectory.c_str();

    HANDLE hDestinationFile = CreateFile(destinationPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDestinationFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        LPWSTR errorMessage;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            error,
            0,
            (LPWSTR)&errorMessage,
            0,
            NULL
        );

        MessageBox(NULL, errorMessage, _T("Ошибка"), MB_OK | MB_ICONERROR);
        LocalFree(errorMessage);
        CloseHandle(hSourceFile);
        SetButtonsState(FALSE);
        return;
    }

    BYTE buffer[4096];
    DWORD bytesRead, bytesWritten;

    while (ReadFile(hSourceFile, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return !isPaused; });

        WriteFile(hDestinationFile, buffer, bytesRead, &bytesWritten, NULL);
    }

    CloseHandle(hSourceFile);
    CloseHandle(hDestinationFile);
    MessageBox(NULL, _T("Операция копирования завершена успешно."), _T("Успех"), MB_OK | MB_ICONINFORMATION);

    SetButtonsState(FALSE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        hwndEditSource = CreateWindow(_T("EDIT"), _T(""), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 20, 20, 300, 25, hwnd, NULL, NULL, NULL);
        hwndEditDestination = CreateWindow(_T("EDIT"), _T(""), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 20, 60, 300, 25, hwnd, NULL, NULL, NULL);
        hwndButtonStart = CreateWindow(_T("BUTTON"), _T("Старт"), WS_CHILD | WS_VISIBLE, 20, 100, 80, 30, hwnd, (HMENU)ID_BUTTON_START, NULL, NULL);
        hwndButtonPauseResume = CreateWindow(_T("BUTTON"), _T("Приостановить"), WS_CHILD | WS_VISIBLE, 110, 100, 120, 30, hwnd, (HMENU)ID_BUTTON_PAUSE, NULL, NULL);
        SetButtonsState(FALSE);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BUTTON_START:
            isPaused = FALSE;
            GetWindowText(hwndEditSource, sourcePath, MAX_PATH);
            GetWindowText(hwndEditDestination, destinationPath, MAX_PATH);
            std::thread(CopyFileAsync, sourcePath, destinationPath).detach();
            break;
        case ID_BUTTON_PAUSE:
            {
                std::lock_guard<std::mutex> lock(mtx);
                isPaused = !isPaused;
            }
            if (!isPaused)
                cv.notify_one();
            SetWindowText(hwndButtonPauseResume, isPaused ? _T("Возобновить") : _T("Приостановить"));
            break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = _T("AsyncCopyAppClass");
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(_T("AsyncCopyAppClass"), _T("Асинхронное копирование файлов"), WS_OVERLAPPEDWINDOW, 100, 100, 400, 200, NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

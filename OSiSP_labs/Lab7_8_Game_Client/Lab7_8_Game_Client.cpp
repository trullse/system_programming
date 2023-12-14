#include <WS2tcpip.h>
#include <windows.h>
#include <string>
#include <commctrl.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")

//HWND g_hListView;
SOCKET g_clientSocket;
HANDLE g_hThread;
//HWND hwndEdit;
//HWND hwndBtn;
HWND hwndText;
HWND gameButtons[9];

bool your_turn = false;
bool game_end = false;

enum gameCondition {
    START_GAME_YOU = 0,
    START_GAME_WAIT = 1,
    MOVE = 2,
    DISCONNECT = 3,
    READY = 4,
};

void SendData(gameCondition cond, int x, int y, bool senderWon);
void ReadData(char* data);
void ClearField();

struct GameState {
    char field[9];
    char winner;
};

void InitializeGameState(GameState* gameState) {
    for (int i = 0; i < 9; i++) {
        gameState->field[i] = ' ';
    }
    gameState->winner = ' ';
}

bool CheckWinner(GameState* gameState) {
    // Check rows
    for (int i = 0; i < 3; i++) {
        if (gameState->field[i * 3] == gameState->field[i * 3 + 1] &&
            gameState->field[i * 3] == gameState->field[i * 3 + 2] &&
            gameState->field[i * 3] != ' ') {
            gameState->winner = gameState->field[i * 3];
            return true;
        }
    }

    // Check columns
    for (int i = 0; i < 3; i++) {
        if (gameState->field[i] == gameState->field[i + 3] &&
            gameState->field[i] == gameState->field[i + 6] &&
            gameState->field[i] != ' ') {
            gameState->winner = gameState->field[i];
            return true;
        }
    }

    // Check diagonals
    if (gameState->field[0] == gameState->field[4] &&
        gameState->field[0] == gameState->field[8] &&
        gameState->field[0] != ' ') {
        gameState->winner = gameState->field[0];
        return true;
    }

    if (gameState->field[2] == gameState->field[4] &&
        gameState->field[2] == gameState->field[6] &&
        gameState->field[2] != ' ') {
        gameState->winner = gameState->field[2];
        return true;
    }

    bool freeSpace = false;
    for (int i = 0; i < 9; i++) {
        if (gameState->field[i] == ' ') {
            freeSpace = true;
        }
    }
    if (!freeSpace) {
        return true;
    }

    return false;
}

GameState* currentGameState = new GameState;

void SendMessageToServer(const char* message)
{
    send(g_clientSocket, message, sizeof(message), 0);
}

DWORD WINAPI MessageListener(LPVOID lpParam)
{
    char buffer[1024];
    while (true) {
        int recvResult = recv(g_clientSocket, buffer, sizeof(buffer), 0);
        if (recvResult > 0) {
            buffer[recvResult] = '\0';

            char* message = new char[1024];
            for (int i = 0; i < 1024; i++) {
                message[i] = buffer[i];
            }
            ReadData(message);
            delete(message);
        }
        else if (recvResult == 0) {
            closesocket(g_clientSocket);
            MessageBox(nullptr, L"Server disconnected", L"Info", MB_OK | MB_ICONINFORMATION);
            break;
        }
        else {
            MessageBox(nullptr, L"Error receiving message", L"Error", MB_OK | MB_ICONERROR);
            closesocket(g_clientSocket);
            break;
        }
    }
}
bool isConnected;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int button_i = 0;
    int x, y;

    switch (uMsg) {
    case WM_CREATE: {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&icex);

        hwndText = CreateWindow(
            L"STATIC", L"window text", WS_CHILD | WS_VISIBLE | SS_CENTER,
            10, 10, 250, 50, hwnd, nullptr, nullptr, nullptr);

        for (button_i = 0; button_i < 9; button_i++) {
            gameButtons[button_i] = CreateWindow(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                (button_i % 3) * 100, (button_i / 3) * 100 + 50, 100, 100, hwnd, (HMENU)(1000 + button_i), NULL, NULL);
        }

        break;
    }
    case WM_COMMAND: {
        if (game_end) {
            SendData(START_GAME_WAIT, 0, 0, false);
            SetWindowText(hwndText, L"Your turn");
            ClearField();
            game_end = false;
            your_turn = true;
        }
        else if (your_turn) {
            your_turn = false;
            button_i = LOWORD(wParam) % 1000;

            x = button_i / 3;
            y = button_i % 3;

            // check for rules

            SetWindowText(gameButtons[button_i], L"X");
            currentGameState->field[button_i] = 'X';
            if (CheckWinner(currentGameState)) {
                if (currentGameState->winner == 'X') {
                    SetWindowText(hwndText, L"You won!");
                    SendData(MOVE, x, y, true);
                    game_end = true;
                }
                else if (currentGameState->winner == ' ') {
                    SendData(MOVE, x, y, false);
                    SetWindowText(hwndText, L"Nobody won");
                    game_end = true;
                }
            }
            else {
                SendData(MOVE, x, y, false);
                SetWindowText(hwndText, L"Wait for opponents move");
            }
        }
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default: {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    }

    return 0;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return -1;
    }

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"GameClientClass";



    if (RegisterClass(&wc)) {
        HWND hwnd = CreateWindowEx(
            0, L"GameClientClass", L"Крестики-нолики",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 315, 385,
            nullptr, nullptr, hInstance, nullptr);

        if (hwnd) {
            ShowWindow(hwnd, nCmdShow);

            InitializeGameState(currentGameState);

            if (!isConnected) {
                g_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
                if (g_clientSocket == INVALID_SOCKET) {
                    MessageBox(hwnd, L"Error creating socket", L"Error", MB_OK | MB_ICONERROR);
                    return 0;
                }

                sockaddr_in serverAddr;
                serverAddr.sin_family = AF_INET;
                serverAddr.sin_port = htons(8888);
                inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // ip to binary
                //family, address, buffer

                if (connect(g_clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
                    MessageBox(hwnd, L"Error connecting to server", L"Error", MB_OK | MB_ICONERROR);
                    closesocket(g_clientSocket);
                    return 0;
                }
                // first data send
                //WCHAR buff[1024];
                //GetWindowText(hwndEdit, buff, 1024);
                //SendMessageToServer(buff);
                isConnected = true;
                SetWindowText(hwndText, L"Connected");
                g_hThread = CreateThread(nullptr, 0, MessageListener, nullptr, 0, nullptr);
                //SetWindowText(hwndBtn, L"Send");
            }

            MSG msg;
            while (GetMessage(&msg, nullptr, 0, 0)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    return 0;
}

void SendData(gameCondition cond, int x = 0, int y = 0, bool senderWon = false) {
    char* buff = new char[4];
    buff[0] = static_cast<WCHAR>(cond);
    buff[1] = static_cast<WCHAR>(x);
    buff[2] = static_cast<WCHAR>(y);
    buff[3] = senderWon ? L'1' : L'0';
    SendMessageToServer(buff);
}

void ReadData(char* data) {
    gameCondition cond = static_cast<gameCondition>(data[0]);
    int x = static_cast<int>(data[1]);
    int y = static_cast<int>(data[2]);
    bool senderWon = data[3] == '1' ? true : false;

    switch (cond) {
    case START_GAME_WAIT:
        SetWindowText(hwndText, L"Wait for opponents move");
        your_turn = false;
        game_end = false;
        ClearField();
        break;
    case START_GAME_YOU:
        SetWindowText(hwndText, L"Your turn");
        your_turn = true;
        break;
    case MOVE:
        SetWindowText(gameButtons[x * 3 + y], L"O");
        currentGameState->field[x * 3 + y] = 'O';

        if (CheckWinner(currentGameState)) {
            if (currentGameState->winner == 'O') {
                SetWindowText(hwndText, L"You losed!");
                game_end = true;
            }
            else if (currentGameState->winner == ' ') {
                SetWindowText(hwndText, L"Nobody won");
                game_end = true;
            }
        }
        else {
            SetWindowText(hwndText, L"Your turn");
            your_turn = true;
        }
        break;
    case DISCONNECT:
        SetWindowText(hwndText, L"Unable to connect");
        your_turn = false;
        break;
    }
}

void ClearField() {
    for (int i = 0; i < 9; i++) {
        SetWindowText(gameButtons[i], L"");
        currentGameState->field[i] = ' ';
        currentGameState->winner = ' ';
    }
}
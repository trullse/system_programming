#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#include <ws2tcpip.h>

int connected_num = 0;

enum gameCondition {
    START_GAME_YOU = 0,
    START_GAME_WAIT = 1,
    MOVE = 2,
    DISCONNECT = 3,
    READY = 4,
};

char* SendData(gameCondition cond, int x = 0, int y = 0, bool senderWon = false);

class GameRoom {
public:
    int join(SOCKET gamerSocket)
    {
        gamers.push_back(gamerSocket);
        std::cout << "Player connected\n";
        if (connected_num == 2) {
            char* msg1 = SendData(START_GAME_YOU);
            char* msg2 = SendData(START_GAME_WAIT);
            send(gamers[0], msg1, sizeof(msg1), 0);
            send(gamers[1], msg2, sizeof(msg2), 0);
            delete(msg1);
            delete(msg2);
            std::cout << "Game started\n";
        }
        return 1;
    }

    void leave(SOCKET clientSocket)
    {
        auto it = std::find_if(gamers.begin(), gamers.end(), [clientSocket](const SOCKET& info) {
            return info == clientSocket;
            });

        if (it != gamers.end()) {
            gamers.erase(it);
        }
    }

    void sendMessage(const char* message, SOCKET srcSocket, SOCKET excludeGamer = INVALID_SOCKET)
    {
        for (const auto& gamer : gamers) {
            if (srcSocket != excludeGamer && srcSocket != gamer) {
                send(gamer, message, sizeof(message), 0);
            }
        }
    }

private:

    std::vector<SOCKET> gamers;
};

GameRoom globalRoom;

DWORD WINAPI ClientThread(LPVOID lpParam)
{
    SOCKET clientSocket = reinterpret_cast<SOCKET>(lpParam);
    char buffer[1024];
    int recvResult;

    //recvResult = recv(clientSocket, buffer, sizeof(buffer), 0); // returns the number of bytes received
    //socket descriptor, pointer to buffer, len of buffer, flags

    int res = globalRoom.join(clientSocket);

    while (true) {
        recvResult = recv(clientSocket, buffer, sizeof(buffer), 0);
        std::cout << "recieved some messages\n";
        if (recvResult > 0) {
            buffer[recvResult] = '\0';

            std::cout << "some message was sent\n";
            globalRoom.sendMessage(buffer, clientSocket);
        }
        else if (recvResult == 0) {
            globalRoom.leave(clientSocket);
            closesocket(clientSocket);
            connected_num--;
            std::cout << "Client disconnected\n";
            break;
        }
        else {
            std::cerr << "Recv failed\n";
            globalRoom.leave(clientSocket);
            closesocket(clientSocket);
            connected_num--;
            globalRoom.sendMessage(SendData(DISCONNECT), clientSocket);
            break;
        }
    }

    return 0;
}

int main()
{
    WSADATA wsaData; // information about the Windows Sockets implementation
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { // initiates use of the Winsock DLL by a process like a load library
        //version and wsadata
        std::cerr << "WSAStartup failed\n";
        return -1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    // af_inet - ipv4 (adress family specification), sock_stream - for stream socket
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket\n";
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr; //specifies a transport address and port for the AF_INET address family
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888); // converts port to big-endian
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) { // bind local address with a socket
        // socket descriptor, pointer to sockaddr, length of sockaddr
        std::cerr << "Bind failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    if (listen(serverSocket, 2) == SOCKET_ERROR) { //somaxconn - max
        std::cerr << "Listen failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Server is listening on port 8888...\n";


    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed\n";
            break;
        }
        if (connected_num >= 2) {
            std::cerr << "Max amount of players\n";
            char* msg = SendData(DISCONNECT);
            send(clientSocket, msg, sizeof(msg), 0);
            delete(msg);
            closesocket(clientSocket);
            continue;
        }

        connected_num++;
        std::cout << "Client connected\n";

        HANDLE threadHandle = CreateThread(nullptr, 0, ClientThread, reinterpret_cast<LPVOID>(clientSocket), 0, nullptr);
        //pointer to a SECURITY_ATTRIBUTES (null - cannot be inherited), size (0 - default value is used), function, pointer to params to pass, flags (0 - runs immidiately)
        //pointer to a variable that recieves thread identifier
        if (threadHandle == nullptr) {
            std::cerr << "Failed to create thread\n";
            closesocket(clientSocket);
            break;
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

char* SendData(gameCondition cond, int x, int y, bool senderWon) {
    char* buff = new char[4];
    buff[0] = static_cast<WCHAR>(cond);
    buff[1] = static_cast<WCHAR>(x);
    buff[2] = static_cast<WCHAR>(y);
    buff[3] = senderWon ? L'1' : L'0';
    return buff;
}
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <cstring>
#include <string>
#include "NimH.h"
#include "gameBoard.h"

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

void runServer(const char userName[]);
bool runClient(const char userName[]);

void printMainMenu();

void logSent(const char msg[], const sockaddr_in& addr);
void logReceived(const char msg[], const sockaddr_in& addr);

bool sameAddress(const sockaddr_in& a, const sockaddr_in& b);
void buildResponse(char dest[], const char prefix[], const char value[]);

// ===========================================================================
// main
// ===========================================================================
int main()
{
    WSADATA wsaData;
    int iResult;

    char userName[MAX_NAME] = "";
    int menuChoice = 0;
    bool running = true;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        cout << "WSAStartup() failed: " << iResult << endl;
        return 1;
    }

    cout << "Enter your name: ";
    cin.getline(userName, MAX_NAME);

    while (running)
    {
        printMainMenu();
        cin >> menuChoice;
        cin.ignore(1000, '\n');

        switch (menuChoice)
        {
        case 1: runServer(userName);                          break;
        case 2: if (runClient(userName)) running = false;    break;
        case 3: running = false;                              break;
        default: cout << "Invalid choice.\n";
        }
    }

    WSACleanup();
    return 0;
}


// ===========================================================================
// runServer
// ===========================================================================
void runServer(const char userName[])
{
    int iResult;

    // Resolve local bind address
    addrinfo hints{}, * result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    char portStr[20];
    _itoa_s(DEFAULT_PORT, portStr, 10);

    iResult = getaddrinfo(NULL, portStr, &hints, &result);
    if (iResult != 0)
    {
        cerr << "getaddrinfo() failed: " << gai_strerrorA(iResult) << endl;
        return;
    }

    SOCKET s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (s == INVALID_SOCKET)
    {
        cout << "socket() failed: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        return;
    }

    iResult = bind(s, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        cout << "bind() failed: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        closesocket(s);
        return;
    }
    freeaddrinfo(result);

    char recvBuf[DEFAULT_BUFLEN] = "";
    char sendBuf[DEFAULT_BUFLEN] = "";
    sockaddr_in senderAddr{};
    int senderAddrSize = sizeof(senderAddr);

    cout << "\nHosting game on port " << DEFAULT_PORT << "...\n";
    cout << "Waiting for requests...\n";

    // Main listen loop
    while (true)
    {
        memset(recvBuf, 0, DEFAULT_BUFLEN);
        memset(sendBuf, 0, DEFAULT_BUFLEN);

        iResult = recvfrom(s, recvBuf, DEFAULT_BUFLEN, 0,
            (sockaddr*)&senderAddr, &senderAddrSize);
        if (iResult == SOCKET_ERROR)
        {
            cout << "recvfrom() failed: " << WSAGetLastError() << endl;
            continue;
        }
        logReceived(recvBuf, senderAddr);

        // Discovery query
        if (_stricmp(recvBuf, Nim_QUERY) == 0)
        {
            buildResponse(sendBuf, Nim_NAME, userName);
            iResult = sendto(s, sendBuf, (int)strlen(sendBuf) + 1, 0,
                (sockaddr*)&senderAddr, senderAddrSize);
            if (iResult == SOCKET_ERROR)
                cout << "sendto() failed: " << WSAGetLastError() << endl;
            else
                logSent(sendBuf, senderAddr);
            continue;
        }

        // Challenge request — client sends Nim_PLAYER prefix + username
        if (_strnicmp(recvBuf, Nim_PLAYER, strlen(Nim_PLAYER)) == 0)
        {
            char answer;
            // The challenger's name follows the Nim_PLAYER prefix
            const char* challengerName = recvBuf + strlen(Nim_PLAYER);
            cout << "Challenge received from " << challengerName << "!\nAccept? (y/n): ";
            cin >> answer;
            cin.ignore(1000, '\n');

            // Decline
            if (answer == 'n' || answer == 'N')
            {
                sendto(s, Nim_NO, (int)strlen(Nim_NO) + 1, 0,
                    (sockaddr*)&senderAddr, senderAddrSize);
                logSent(Nim_NO, senderAddr);
                continue;
            }

            if (answer != 'y' && answer != 'Y')
            {
                cout << "Invalid input — challenge ignored.\n";
                continue;
            }

            // Accept: send Nim_YES then wait for Nim_GREAT
            sendto(s, Nim_YES, (int)strlen(Nim_YES) + 1, 0,
                (sockaddr*)&senderAddr, senderAddrSize);
            logSent(Nim_YES, senderAddr);

            if (wait(s, 30, 0) != 1)
            {
                cout << "Timed out waiting for client acknowledgment.\n";
                continue;
            }
            memset(recvBuf, 0, DEFAULT_BUFLEN);
            iResult = recvfrom(s, recvBuf, DEFAULT_BUFLEN, 0,
                (sockaddr*)&senderAddr, &senderAddrSize);
            if (iResult <= 0 || _stricmp(recvBuf, Nim_GREAT) != 0)
            {
                cout << "Did not receive expected acknowledgment.\n";
                continue;
            }
            logReceived(recvBuf, senderAddr);

            // Build and send the game board
            char boardStr[DEFAULT_BUFLEN];
			bool invalidInput = true;

            while (invalidInput == true) {
                cout << "Create the game board.\n"
                    << "  Format : MnnNnNN...  (M = pile count, each pile = 2 digits)\n"
                    << "  Example: 3120803  ->  3 piles: 12, 8, 3\n"
                    << "  Rules  : 3-9 piles, 1-20 rocks each\n"
                    << "> ";
                cin >> boardStr;
                cin.ignore(1000, '\n');

                if (boardStr[0] < '3' || boardStr[0] > '9') {
                    cout << "Invalid board, must have 3-9 piles.\n";
					invalidInput = true;
                }
                else {
					invalidInput = false;
                }
            }

            sendto(s, boardStr, (int)strlen(boardStr) + 1, 0,
                (sockaddr*)&senderAddr, senderAddrSize);
            logSent(boardStr, senderAddr);

            GameBoard serverGame(boardStr);

            // Game loop: client moves first
            bool gameOver = false;
            while (!gameOver)
            {
                // Wait for client's move
                bool clientMoved = false;
                int attempts = 0;
                while (!clientMoved && attempts < 6)
                {
                    if (wait(s, 30, 0) != 1) { ++attempts; continue; }

                    memset(recvBuf, 0, DEFAULT_BUFLEN);
                    iResult = recvfrom(s, recvBuf, DEFAULT_BUFLEN, 0,
                        (sockaddr*)&senderAddr, &senderAddrSize);
                    if (iResult <= 0) { ++attempts; continue; }

                    logReceived(recvBuf, senderAddr);

                    // A digit-leading string is a move; anything else is a comment
                    if (!isdigit((unsigned char)recvBuf[0]))
                    {
                        cout << "Opponent says: " << recvBuf << "\n";
                        continue;
                    }

                    if (serverGame.makeMove(recvBuf) == -1)
                    {
                        cout << "Client sent invalid move, ignoring.\n";
                        ++attempts;
                        continue;
                    }
                    clientMoved = true;
                }

                if (!clientMoved)
                {
                    cout << "Opponent timed out. You win by default!\n";
                    closesocket(s);
                    return;
                }

                // Client moved last; if board is empty, client wins
                if (serverGame.checkWin())
                {
                    cout << "Opponent wins!\n";
                    gameOver = true;
                    break;
                }

                // Server's turn
                bool moveMade = false;
                while (!moveMade)
                {
                    char choice[DEFAULT_BUFLEN];
                    cout << "Your turn:\n"
                        << "  Move    : pnn  (pile digit + 2-digit count, e.g. 211 = pile 2, remove 11)\n"
                        << "  Comment : C<text>\n"
                        << "  Forfeit : F\n"
                        << "> ";
                    cin.getline(choice, DEFAULT_BUFLEN);

                    if (choice[0] == 'C' || choice[0] == 'c')
                    {
                        const char* txt = choice + 1;
                        sendto(s, txt, (int)strlen(txt) + 1, 0,
                            (sockaddr*)&senderAddr, senderAddrSize);
                        logSent(txt, senderAddr);
                        continue;
                    }

                    if (choice[0] == 'F' || choice[0] == 'f')
                    {
                        cout << "You forfeited.\n";
                        closesocket(s);
                        return;
                    }

                    if (serverGame.makeMove(choice) == -1)
                    {
                        cout << "Invalid move, try again.\n";
                        continue;
                    }

                    sendto(s, choice, (int)strlen(choice) + 1, 0,
                        (sockaddr*)&senderAddr, senderAddrSize);
                    logSent(choice, senderAddr);
                    moveMade = true;
                }

                // Server moved last; if board is empty, server wins
                if (serverGame.checkWin())
                {
                    cout << "You win!\n";
                    gameOver = true;
                }
            }

            closesocket(s);
            return;
        }

        cout << "Unknown datagram ignored.\n";
    }

    closesocket(s);
}


// ===========================================================================
// runClient
// ===========================================================================
bool runClient(const char userName[])
{
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET)
    {
        cout << "socket() failed: " << WSAGetLastError() << endl;
        return false;
    }

    // Discover servers
    ServerStruct servers[MAX_SERVERS]{};
    int numServers = getServers(s, servers);
    if (numServers <= 0)
    {
        cout << "No games found.\n";
        closesocket(s);
        return false;
    }

    cout << "\nAvailable games:\n";
    for (int i = 0; i < numServers; i++)
        cout << "  " << i + 1 << ". " << servers[i].name << "\n";

    int selection;
    cout << "Choose a game: ";
    cin >> selection;
    cin.ignore(1000, '\n');

    if (selection < 1 || selection > numServers)
    {
        cout << "Invalid selection.\n";
        closesocket(s);
        return false;
    }

    sockaddr_in selectedServer = servers[selection - 1].addr;
    sockaddr_in fromAddr{};
    int fromAddrSize = sizeof(fromAddr);

    char sendBuf[DEFAULT_BUFLEN] = "";
    char recvBuf[DEFAULT_BUFLEN] = "";
    int  iResult;

    // Send challenge
    strcpy_s(sendBuf, DEFAULT_BUFLEN, Nim_PLAYER);
    strcat_s(sendBuf, DEFAULT_BUFLEN, userName);

    iResult = sendto(s, sendBuf, (int)strlen(sendBuf) + 1, 0,
        (sockaddr*)&selectedServer, sizeof(selectedServer));
    if (iResult == SOCKET_ERROR)
    {
        cout << "sendto() failed: " << WSAGetLastError() << endl;
        closesocket(s);
        return false;
    }
    logSent(sendBuf, selectedServer);

    // Wait for accept / decline
    if (wait(s, 10, 0) != 1)
    {
        cout << "Timed out, no response from server.\n";
        closesocket(s);
        return false;
    }

    memset(recvBuf, 0, DEFAULT_BUFLEN);
    iResult = recvfrom(s, recvBuf, DEFAULT_BUFLEN, 0,
        (sockaddr*)&fromAddr, &fromAddrSize);
    if (iResult <= 0)
    {
        cout << "recvfrom() failed: " << WSAGetLastError() << endl;
        closesocket(s);
        return false;
    }
    if (!sameAddress(fromAddr, selectedServer))
    {
        cout << "Ignored datagram from unexpected sender.\n";
        closesocket(s);
        return false;
    }
    logReceived(recvBuf, fromAddr);

    // Declined
    if (_stricmp(recvBuf, Nim_NO) == 0)
    {
        cout << "Challenge declined.\n";
        cout << "1 = choose another game   2 = exit\n> ";
        int ch; cin >> ch; cin.ignore(1000, '\n');
        closesocket(s);
        return (ch == 1) ? runClient(userName) : true;
    }

    // Not accepted either — unexpected message
    if (_stricmp(recvBuf, Nim_YES) != 0)
    {
        cout << recvBuf << "\n";
        closesocket(s);
        return false;
    }

    // Send Nim_GREAT acknowledgment
    sendto(s, Nim_GREAT, (int)strlen(Nim_GREAT) + 1, 0,
        (sockaddr*)&fromAddr, fromAddrSize);
    logSent(Nim_GREAT, fromAddr);

    // Receive game board
    if (wait(s, 30, 0) != 1)
    {
        cout << "Timed out waiting for game board.\n";
        closesocket(s);
        return false;
    }

    memset(recvBuf, 0, DEFAULT_BUFLEN);
    iResult = recvfrom(s, recvBuf, DEFAULT_BUFLEN, 0,
        (sockaddr*)&fromAddr, &fromAddrSize);
    if (iResult <= 0)
    {
        cout << "Failed to receive game board.\n";
        closesocket(s);
        return false;
    }
    logReceived(recvBuf, fromAddr);

    char boardStr[DEFAULT_BUFLEN];
    strcpy_s(boardStr, DEFAULT_BUFLEN, recvBuf);

    GameBoard clientGame(boardStr);

    // Game loop: client moves first
    bool gameOver = false;
    while (!gameOver)
    {
        // Client's turn
        bool moveMade = false;
        while (!moveMade)
        {
            char choice[DEFAULT_BUFLEN];
            cout << "Your turn:\n"
                << "  Move    : pnn  (pile digit + 2 digit count, e.g. 211 = pile 2, remove 11)\n"
                << "  Comment : C<text>\n"
                << "  Forfeit : F\n"
                << "> ";
            cin.getline(choice, DEFAULT_BUFLEN);

            if (choice[0] == 'C' || choice[0] == 'c')
            {
                const char* txt = choice + 1;
                sendto(s, txt, (int)strlen(txt) + 1, 0,
                    (sockaddr*)&fromAddr, fromAddrSize);
                logSent(txt, fromAddr);
                continue;
            }

            if (choice[0] == 'F' || choice[0] == 'f')
            {
                cout << "You forfeited.\n";
                closesocket(s);
                return true;
            }

            if (clientGame.makeMove(choice) == -1)
            {
                cout << "Invalid move, try again.\n";
                continue;
            }

            sendto(s, choice, (int)strlen(choice) + 1, 0,
                (sockaddr*)&fromAddr, fromAddrSize);
            logSent(choice, fromAddr);
            moveMade = true;
        }

        // Client moved last; if board is empty, client wins
        if (clientGame.checkWin())
        {
            cout << "You win!\n";
            gameOver = true;
            break;
        }

        // Wait for server's move
        bool serverMoved = false;
        int attempts = 0;
        while (!serverMoved && attempts < 6)
        {
            if (wait(s, 30, 0) != 1) { ++attempts; continue; }

            memset(recvBuf, 0, DEFAULT_BUFLEN);
            iResult = recvfrom(s, recvBuf, DEFAULT_BUFLEN, 0,
                (sockaddr*)&fromAddr, &fromAddrSize);
            if (iResult <= 0) { ++attempts; continue; }

            logReceived(recvBuf, fromAddr);

            // Comment vs move
            if (!isdigit((unsigned char)recvBuf[0]))
            {
                cout << "Opponent says: " << recvBuf << "\n";
                continue;
            }

            if (clientGame.makeMove(recvBuf) == -1)
            {
                cout << "Server sent invalid move, ignoring.\n";
                ++attempts;
                continue;
            }
            serverMoved = true;
        }

        if (!serverMoved)
        {
            cout << "Opponent timed out. You win by default!\n";
            closesocket(s);
            return true;
        }

        // Server moved last; if board is empty, server wins
        if (clientGame.checkWin())
        {
            cout << "Opponent wins!\n";
            gameOver = true;
        }
    }

    closesocket(s);
    return false;
}


// ===========================================================================
// Helpers
// ===========================================================================
void printMainMenu()
{
    cout << "\nNIM Game:\n"
        << "  1. Host a game\n"
        << "  2. Join a game\n"
        << "  3. Exit\n"
        << "Choice: ";
}

void logSent(const char msg[], const sockaddr_in& addr)
{
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, (void*)&addr.sin_addr, ip, INET_ADDRSTRLEN);
    cout << "[SENT     -> " << ip << ":" << ntohs(addr.sin_port) << "] " << msg << "\n";
}

void logReceived(const char msg[], const sockaddr_in& addr)
{
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, (void*)&addr.sin_addr, ip, INET_ADDRSTRLEN);
    cout << "[RECEIVED <- " << ip << ":" << ntohs(addr.sin_port) << "] " << msg << "\n";
}

bool sameAddress(const sockaddr_in& a, const sockaddr_in& b)
{
    return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}

void buildResponse(char dest[], const char prefix[], const char value[])
{
    strcpy_s(dest, DEFAULT_BUFLEN, prefix);
    strcat_s(dest, DEFAULT_BUFLEN, value);
}

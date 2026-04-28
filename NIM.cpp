#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <cstring>
#include "NimH.h"
#include "gameBoard.cpp"

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

void runServer(const char userName[]);
bool runClient(const char userName[]);

void printMainMenu();
void printClientMenu();

void logSent(const char msg[], const sockaddr_in& addr);
void logReceived(const char msg[], const sockaddr_in& addr);

bool sameAddress(const sockaddr_in& a, const sockaddr_in& b);
void addNameToList(char list[], const char name[]);
void buildResponse(char dest[], const char prefix[], const char value[]);




int main() 
{
    WSADATA wsaData;
    int iResult;

    char userName[MAX_NAME] = "";
    int menuChoice = 0;
    bool running = true;

    //Initialize Winsock
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

        switch (menuChoice) {
        case 1:
            runServer(userName);
            break;
        case 2:
            if (runClient(userName)) {
                running = false;
            }
            break;
        case 3:
            running = false;
            break;
        default:
            cout << "Invalid choice.\n";
        }
    }

    WSACleanup();
    return 0;
}


void runServer(const char userName[]) 
{
    int iResult;

    //Resolve local bind address
    addrinfo hints, * result = NULL;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    char portStr[20];
    _itoa_s(DEFAULT_PORT, portStr, 10);

    iResult = getaddrinfo(NULL, portStr, &hints, &result);
    if (iResult != 0) 
    {
        cerr << "getaddrinfo() failed: " << gai_strerrorA(iResult) << " (" << iResult << ")" << endl;
        return;
    }

    //Create socket
    SOCKET s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (s == INVALID_SOCKET) 
    {
        cout << "socket() failed: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        return;
    }

    //Bind socket
    iResult = bind(s, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) 
    {
        cout << "bind() failed: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        closesocket(s);
        return;
    }

    freeaddrinfo(result);



	char move[3] = ""; 
	char comment[DEFAULT_BUFLEN] = "";
	char forfeit[DEFAULT_BUFLEN] = "";
    char recvBuf[DEFAULT_BUFLEN] = "";
    char sendBuf[DEFAULT_BUFLEN] = "";

    sockaddr_in senderAddr{};
    int senderAddrSize = sizeof(senderAddr);


    cout << "\nHosting Game on port " << DEFAULT_PORT << "...\n";
    cout << "Waiting for requests...\n";

    while (true) 
    {
        memset(recvBuf, 0, DEFAULT_BUFLEN);
        memset(sendBuf, 0, DEFAULT_BUFLEN);

        iResult = recvfrom(
            s,
            recvBuf,
            DEFAULT_BUFLEN,
            0,
            (sockaddr*)&senderAddr,
            &senderAddrSize
        );

        if (iResult == SOCKET_ERROR) 
        {
            cout << "recvfrom() failed: " << WSAGetLastError() << endl;
            continue;
        }

        logReceived(recvBuf, senderAddr);

        if (_stricmp(recvBuf, Nim_QUERY) == 0) 
        {
            buildResponse(sendBuf, Nim_NAME, userName);
        }

        
        else if (_stricmp(recvBuf, Nim_PLAYER) == 0)
        {
            char comment;
			cout << "Received Challenge from " << recvBuf<< endl;
			cout << "Accept ? (y/n): \n";
			cin >> comment;
            if (comment == 'y' || comment == 'Y')
            {
                sendto(s, Nim_YES, strlen(Nim_YES), 0, (sockaddr*)&senderAddr, senderAddrSize);
				wait(s, 2, 0); //wait for client to respond before sending game data
                if (strcmp(sendBuf, Nim_GREAT) == 0) 
                {
                    //call game board constructor here
                    // check for number of piles < 0
                }
                else
                {
                    continue; // loop back arround to wait
				}
                
            } else if (comment == 'n' || comment == 'N')
            {
                sendto(s, Nim_NO, strlen(Nim_NO), 0, (sockaddr*)&senderAddr, senderAddrSize);
            }
        }
		else if (_stricmp(recvBuf, Study_WHAT) == 0)  
        {
            buildResponse(sendBuf, Study_COURSES, courses);
        }

        /*
          else if (_stricmp(recvBuf, Study_MEMBERS) == 0)
        {
            buildResponse(sendBuf, Study_MEMLIST, members);
        }
        else if (_strnicmp(recvBuf, Study_JOIN, strlen(Study_JOIN)) == 0)
        {
            char joiningName[MAX_NAME] = "";
            strcpy_s(joiningName, MAX_NAME, recvBuf + strlen(Study_JOIN));
            addNameToList(members, joiningName);
            strcpy_s(sendBuf, DEFAULT_BUFLEN, Study_CONFIRM);
        } */
        else
        {
            cout << "Unknown datagram ignored.\n";
            continue;
        }

        iResult = sendto(
            s,
            sendBuf,
            (int)strlen(sendBuf) + 1,
            0,
            (sockaddr*)&senderAddr,
            senderAddrSize
        );

        if (iResult == SOCKET_ERROR) 
        {
            cout << "sendto() failed: " << WSAGetLastError() << endl;
        }
        else 
        {
            logSent(sendBuf, senderAddr);
        }
    }

    closesocket(s);
}


bool runClient(const char userName[]) 
{
    SOCKET s = INVALID_SOCKET;
    int iResult;

    //Create UDP socket
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) 
    {
        cout << "socket() failed: " << WSAGetLastError() << endl;
        return false;
    }

    //Find servers using provided utility
    ServerStruct servers[MAX_SERVERS]{};
    int numServers = getServers(s, servers);

    if (numServers <= 0) 
    {
        cout << "No Games Found.\n";
        closesocket(s);
        return false;
    }

    cout << "\nAvailable Games:\n";
    for (int i = 0; i < numServers; i++) 
    {
        cout << i + 1 << ". " << servers[i].name << endl;
    }

    int selection;
    cout << "Choose a Game: ";
    cin >> selection;
    cin.ignore(1000, '\n');

    if (selection < 1 || selection > numServers) 
    {
        cout << "Invalid selection.\n";
        closesocket(s);
        return false;
    }

    // Server accepts or declines challenge

    // If server says no, it closes the connection and return to the join menu

    sockaddr_in selectedServer = servers[selection - 1].addr;
    sockaddr_in fromAddr{};
    int fromAddrSize = sizeof(fromAddr);

    char sendBuf[DEFAULT_BUFLEN] = "";
    char recvBuf[DEFAULT_BUFLEN] = "";

    int choice;
    bool clientMenu = true;

    while (clientMenu) 
    {
        printClientMenu();
        cin >> choice;
        cin.ignore(1000, '\n');

        memset(sendBuf, 0, DEFAULT_BUFLEN);
        memset(recvBuf, 0, DEFAULT_BUFLEN);

        /*
        switch (choice) {
        case 1:
            strcpy_s(sendBuf, DEFAULT_BUFLEN, Study_WHERE);
            break;
        case 2:
            strcpy_s(sendBuf, DEFAULT_BUFLEN, Study_WHAT);
            break;
        case 3:
            strcpy_s(sendBuf, DEFAULT_BUFLEN, Study_MEMBERS);
            break;
        case 4:
            strcpy_s(sendBuf, DEFAULT_BUFLEN, Study_JOIN);
            strcat_s(sendBuf, DEFAULT_BUFLEN, userName);
            break;
        case 5:
            clientMenu = false;
            continue;
        default:
            cout << "Invalid choice.\n";
            continue;
        }
        */
        iResult = sendto(
            s,
            sendBuf,
            (int)strlen(sendBuf) + 1,
            0,
            (sockaddr*)&selectedServer,
            sizeof(selectedServer)
        );

        if (iResult == SOCKET_ERROR) 
        {
            cout << "sendto() failed: " << WSAGetLastError() << endl;
            continue;
        }

        logSent(sendBuf, selectedServer);

        int ready = wait(s, 2, 0);
        if (ready == 1)
        {
            iResult = recvfrom(
                s,
                recvBuf,
                DEFAULT_BUFLEN,
                0,
                (sockaddr*)&fromAddr,
                &fromAddrSize
            );

            if (iResult > 0) 
            {
                if (sameAddress(fromAddr, selectedServer)) 
                {
                    logReceived(recvBuf, fromAddr);

                    if (choice == 4 && _stricmp(recvBuf, Study_CONFIRM) == 0) 
                    {
                        cout << "Successfully Joined Game\n";
                        closesocket(s);
                        return true;
                    }
                    else 
                    {
                        cout << recvBuf << endl;
                    }
                }
                else 
                {
                    cout << "Ignored datagram from a different sender.\n";
                }
            }
            else if (iResult == 0) 
            {
                cout << "Connection closed.\n";
            }
            else 
            {
                cout << "recvfrom() failed: " << WSAGetLastError() << endl;
            }
        }
        else 
        {
            cout << "Timed out, no response from server.\n";
        }
    }

    closesocket(s);
    return false;
}

void printMainMenu() 
{
    cout << "\nNIM Game:\n";
    cout << "1. Host a game\n";
    cout << "2. Join a game\n";
    cout << "3. Exit\n";
    cout << "Choice: ";
}

void printClientMenu() //repurpose for if server declinies challenge
{
    cout << "\nJoin Menu:\n";
    cout << "1. Ask location\n";
    cout << "2. Ask courses\n";
    cout << "3. Ask members\n";
    cout << "4. Join group\n";
    cout << "5. Back\n";
    cout << "Choice: ";
}

void logSent(const char msg[], const sockaddr_in& addr) 
{
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, (void*)&addr.sin_addr, ipStr, INET_ADDRSTRLEN);

    cout << "[SENT to " << ipStr << ":" << ntohs(addr.sin_port) << "] "
        << msg << endl;
}

void logReceived(const char msg[], const sockaddr_in& addr) 
{
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, (void*)&addr.sin_addr, ipStr, INET_ADDRSTRLEN);

    cout << "[RECEIVED from " << ipStr << ":" << ntohs(addr.sin_port) << "] "
        << msg << endl;
}

bool sameAddress(const sockaddr_in& a, const sockaddr_in& b) 
{
    return a.sin_addr.s_addr == b.sin_addr.s_addr &&
        a.sin_port == b.sin_port;
}

void addNameToList(char list[], const char name[]) 
{
    if (strlen(list) > 0) {
        strcat_s(list, DEFAULT_BUFLEN, "\n");
    }
    strcat_s(list, DEFAULT_BUFLEN, name);
}

void buildResponse(char dest[], const char prefix[], const char value[]) 
{
    strcpy_s(dest, DEFAULT_BUFLEN, prefix);
    strcat_s(dest, DEFAULT_BUFLEN, value);
}



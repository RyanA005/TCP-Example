#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::thread;

void printLocalIPAddress() {
    char host[NI_MAXHOST];
    if (gethostname(host, sizeof(host)) == SOCKET_ERROR) {
        cout << "Error getting hostname. Quitting" << endl;
        return;
    }

    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* info;
    if (getaddrinfo(host, nullptr, &hints, &info) != 0) {
        cout << "Error getting local IP. Quitting" << endl;
        return;
    }

    for (addrinfo* ptr = info; ptr != nullptr; ptr = ptr->ai_next) {
        sockaddr_in* ipv4 = (sockaddr_in*)ptr->ai_addr;
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ipv4->sin_addr, ipStr, INET_ADDRSTRLEN);
        cout << "Server IP: " << ipStr << endl;
    }

    freeaddrinfo(info);
}

// Function to receive messages in a separate thread
void handleReceiving(SOCKET sock) {
    char buf[4096];
    while (true) {
        ZeroMemory(buf, 4096);
        int bytesReceived = recv(sock, buf, 4096, 0);
        if (bytesReceived <= 0) {
            cout << "\nConnection lost. Exiting...\n";
            break;
        }
        cout << "\nReceived: " << string(buf, 0, bytesReceived) << "\n> ";
    }
}

// Function to send messages in a separate thread
void handleSending(SOCKET sock) {
    string userInput;
    while (true) {
        cout << "> ";
        getline(cin, userInput);
        if (userInput.empty()) continue;

        int sendResult = send(sock, userInput.c_str(), userInput.size(), 0);
        if (sendResult == SOCKET_ERROR) {
            cout << "Error sending message. Connection lost." << endl;
            break;
        }
    }
}

void server() {
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);
    if (WSAStartup(ver, &wsData) != 0) {
        cout << "Winsock initialization failed!" << endl;
        return;
    }

    SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == INVALID_SOCKET) {
        cout << "Socket creation failed!" << endl;
        WSACleanup();
        return;
    }

    sockaddr_in hint = {};
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54000);
    hint.sin_addr.S_un.S_addr = INADDR_ANY;

    printLocalIPAddress();

    if (bind(listening, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
        cout << "Binding failed!" << endl;
        closesocket(listening);
        WSACleanup();
        return;
    }

    listen(listening, SOMAXCONN);
    cout << "Waiting for a connection..." << endl;

    sockaddr_in client;
    int clientSize = sizeof(client);
    SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
    if (clientSocket == INVALID_SOCKET) {
        cout << "Client connection failed!" << endl;
        closesocket(listening);
        WSACleanup();
        return;
    }

    closesocket(listening);

    cout << "Client connected! Start chatting.\n";

    thread receiveThread(handleReceiving, clientSocket);
    thread sendThread(handleSending, clientSocket);

    receiveThread.join();
    sendThread.join();

    closesocket(clientSocket);
    WSACleanup();
}

void client(string ipAddress) {
    int port = 54000;

    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);
    if (WSAStartup(ver, &wsData) != 0) {
        cout << "Can't start Winsock! Quitting" << endl;
        return;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cout << "Can't create socket, Err #" << WSAGetLastError() << endl;
        WSACleanup();
        return;
    }

    sockaddr_in hint = {};
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

    if (connect(sock, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
        cout << "Can't connect to server, Err #" << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return;
    }

    cout << "Connected to server! Start chatting.\n";

    thread receiveThread(handleReceiving, sock);
    thread sendThread(handleSending, sock);

    receiveThread.join();
    sendThread.join();

    closesocket(sock);
    WSACleanup();
}

int main() {
    char option;
    cout << "Would you like to be server or client? (s/c) > ";
    cin >> option;
    cin.ignore();

    if (option == 's') {
        server();
    }
    else if (option == 'c') {
        string ip;
        cout << "Enter server IP: ";
        cin >> ip;
        client(ip);
    }
    else {
        cout << "Invalid input" << endl;
    }

    return 0;
}

#include <iostream>
#include <string>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using std::cin;
using std::cout;
using std::endl;
using std::string;

void printLocalIPAddress() {
    char host[NI_MAXHOST];
    if (gethostname(host, sizeof(host)) == SOCKET_ERROR) {
        cout << "Error getting hostname. Quitting" << endl;
        return;
    }

    addrinfo hints = {};
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP, UDP is SOCK_DRGAM
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

void server() {
    // Initialize Winsock
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);
    if (WSAStartup(ver, &wsData) != 0) {
        cout << "winsock error" << endl;
        return;
    }

    // Create socket
    SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == INVALID_SOCKET) {
        cout << "socket creation error" << endl;
        WSACleanup();
        return;
    }

    // Bind IP and port
    sockaddr_in hint = {};
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54000);
    hint.sin_addr.S_un.S_addr = INADDR_ANY;

    printLocalIPAddress();

    if (bind(listening, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
        cout << "binding error" << endl;
        closesocket(listening);
        WSACleanup();
        return;
    }

    // Start listening
    listen(listening, SOMAXCONN);
    cout << "Waiting for a connection..." << endl;

    // Accept connection
    sockaddr_in client;
    int clientSize = sizeof(client);
    SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
    if (clientSocket == INVALID_SOCKET) {
        cout << "Client connection failed!" << endl;
        closesocket(listening);
        WSACleanup();
        return;
    }

    // Get client IP
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.sin_addr, clientIP, INET_ADDRSTRLEN);

    // Get client hostname
    char clientHost[NI_MAXHOST];
    char clientService[NI_MAXSERV];
    if (getnameinfo((sockaddr*)&client, sizeof(client), clientHost, NI_MAXHOST, clientService, NI_MAXSERV, 0) == 0) {
        cout << "Client connected: " << clientHost << " (" << clientIP << ") on port " << ntohs(client.sin_port) << endl;
    }
    else {
        cout << "Client connected: " << clientIP << " (hostname unknown) on port " << ntohs(client.sin_port) << endl;
    }

    closesocket(listening); // Close listening socket

    // Receive & echo loop
    char buf[4096];
    while (true) {
        ZeroMemory(buf, 4096);
        int bytesReceived = recv(clientSocket, buf, 4096, 0);
        if (bytesReceived <= 0) {
            cout << "Client disconnected." << endl;
            break;
        }

        cout << "Received: " << string(buf, 0, bytesReceived) << endl;
        send(clientSocket, buf, bytesReceived, 0);
    }

    // Cleanup
    closesocket(clientSocket);
    WSACleanup();
}

void client(string ipAddress) {
    int port = 54000;

    // Initialize Winsock
    WSADATA data;
    WORD ver = MAKEWORD(2, 2);
    if (WSAStartup(ver, &data) != 0) {
        cout << "Can't start Winsock! Quitting" << endl;
        return;
    }

    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cout << "Can't create socket, Err #" << WSAGetLastError() << endl;
        WSACleanup();
        return;
    }

    // Fill in server details
    sockaddr_in hint = {};
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

    // Connect to server
    if (connect(sock, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
        cout << "Can't connect to server, Err #" << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return;
    }

    cout << "Connected to server!" << endl;

    // Message loop
    char buf[4096];
    string userInput;
    while (true) {
        cout << "> ";
        getline(cin, userInput);
        if (userInput.empty()) continue;

        int sendResult = send(sock, userInput.c_str(), userInput.size(), 0);
        if (sendResult != SOCKET_ERROR) {
            ZeroMemory(buf, 4096);
            int bytesReceived = recv(sock, buf, 4096, 0);
            if (bytesReceived > 0) {
                cout << "SERVER> " << string(buf, 0, bytesReceived) << endl;
            }
        }
    }

    // Cleanup
    closesocket(sock);
    WSACleanup();
}

int main() {
    char option;
    cout << "Would you like to be server or client? (s/c) > ";
    cin >> option;
    cin.ignore(); // Ignore newline in input buffer

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

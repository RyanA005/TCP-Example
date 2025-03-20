#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::thread;
using std::chrono::milliseconds;

#define PORT 54000
#define MAX_ATTEMPTS 250
#define PACKET_INTERVAL 40 // 25 packets per second (1000ms / 25 = 40ms)

void receivePackets(SOCKET sock, sockaddr_in& other)
{
    char buf[1024];
    sockaddr_in sender;
    int senderSize = sizeof(sender);
    char senderIP[INET_ADDRSTRLEN];

    while (true)
    {
        ZeroMemory(buf, 1024);
        int bytesReceived = recvfrom(sock, buf, 1024, 0, (sockaddr*)&sender, &senderSize);
        if (bytesReceived > 0)
        {
            inet_ntop(AF_INET, &sender.sin_addr, senderIP, INET_ADDRSTRLEN);
            cout << "Received: " << string(buf, 0, bytesReceived) << " from " << senderIP << endl;
        }
    }
}

void startHolePunching(string targetIP)
{
    WSADATA data;
    WORD ver = MAKEWORD(2, 2);
    if (WSAStartup(ver, &data) != 0) {
        cout << "Can't start Winsock! Quitting" << endl;
        return;
    }

    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == INVALID_SOCKET)
    {
        cout << "Failed to create UDP socket." << endl;
        WSACleanup();
        return;
    }

    sockaddr_in localAddr;
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(PORT);
    localAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udpSocket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR)
    {
        cout << "Failed to bind socket." << endl;
        closesocket(udpSocket);
        WSACleanup();
        return;
    }

    sockaddr_in targetAddr;
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, targetIP.c_str(), &targetAddr.sin_addr);

    thread receiverThread(receivePackets, udpSocket, std::ref(targetAddr));
    receiverThread.detach();

    for (int i = 0; i < MAX_ATTEMPTS; i++)
    {
        string message = "Hello from " + targetIP;
        sendto(udpSocket, message.c_str(), message.size(), 0, (sockaddr*)&targetAddr, sizeof(targetAddr));
        cout << "Sent packet to " << targetIP << " Attempt " << (i + 1) << endl;
        std::this_thread::sleep_for(milliseconds(PACKET_INTERVAL));
    }

    cout << "Hole punching attempts completed." << endl;
    closesocket(udpSocket);
    WSACleanup();
}

int main()
{
    string targetIP;
    cout << "Enter the IP address of the other player: ";
    cin >> targetIP;

    startHolePunching(targetIP);
    return 0;
}
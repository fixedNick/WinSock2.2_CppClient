// Защита от повторного включения Windows.h заголовка, т.к. он есть включается в заголовке winsock
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h> // after winsock2
#include <stdio.h>
#include <iostream>
#include <conio.h>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

#ifdef NEW_CLIENT_CONNECTED
#undef NEW_CLIENT_CONNECTED
#endif

#ifdef CLIENT_DISCONNECTED
#undef CLIENT_DISCONNECTED
#endif


#define NEW_CLIENT_CONNECTED	1
#define CLIENT_DISCONNECTED		2

using std::cout;
using std::endl;
using std::string;
using std::thread;
using std::vector;

vector<SOCKET> Clients;

WSADATA wsaData;

BOOL TransactMSG(SOCKET ClientSocket, string msg = "");
void ProcessClient(SOCKET ClientSocket);
void NotificateClient(SOCKET ClientSocket, int notificationType);

int main() {
	std::cout << "Server application started!" << std::endl;

	int iResult;

	// Initialize Winsock
	// MAKEWORD 2,2 -> Запрашивание версии win сокетов 2.2 
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cout << "WSAStartup failed: " << iResult << "\n";
		return 1;
	}

	struct addrinfo* result = NULL, * ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed: " << iResult << "\n";
		WSACleanup();
		return 1;
	}

	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (ListenSocket == INVALID_SOCKET) {
		cout << "Error at socket(): " << WSAGetLastError() << "\n";
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError() << "\n";
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		cout << "Listen failed with error: " << WSAGetLastError() << "\n";
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	while (true)
	{
		SOCKET ClientSocket;
		ClientSocket = INVALID_SOCKET;

		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			cout << "accept failed: " << WSAGetLastError() << "\n";
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		Clients.push_back(ClientSocket);

		thread t(ProcessClient, ClientSocket);
		t.detach();
	}

	closesocket(ListenSocket);

	_getch();
	return 0;
}

void ProcessClient(SOCKET ClientSocket)
{
	TransactMSG(ClientSocket);
}

string ReceiveMSGFromClient(SOCKET ClientSocket)
{
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	// Receive until the peer shuts down the connection
	do {

		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			cout << "Bytes received: " << iResult << "\n";
			recvbuf[iResult] = 0;
			cout << "Message from client: " << recvbuf << endl;;
		}
		else if (iResult == 0)
			cout << "Client Disconnected...\n";
		else {
			cout << "recv failed: " << WSAGetLastError() << "\n";
			closesocket(ClientSocket);
			WSACleanup();
			return "";
		}

	} while (iResult > 0);
}

BOOL SendMSGToClient(SOCKET ClientSocket, string msg = "")
{
	int iSendResult;
	// Echo the buffer back to the sender
	iSendResult = send(ClientSocket, msg.c_str(), (unsigned)strlen(msg.c_str()), 0);
	if (iSendResult == SOCKET_ERROR) {
		cout << "send failed: " << WSAGetLastError() << "\n";
		closesocket(ClientSocket);
		WSACleanup();
		return false;
	}
	cout << "Bytes sent: " << iSendResult << "\n";
	return true;
}

BOOL TransactMSG(SOCKET ClientSocket, string msg)
{
	int iResult;
	string recvbuf = ReceiveMSGFromClient(ClientSocket);
	SendMSGToClient(ClientSocket, msg.length() > 0 ? msg : recvbuf);
	if (recvbuf == "stop_server") {
		iResult = shutdown(ClientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			cout << "shutdown failed: " << WSAGetLastError() << "\n";
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
		closesocket(ClientSocket);
		WSACleanup();
	}
	return 0;
}
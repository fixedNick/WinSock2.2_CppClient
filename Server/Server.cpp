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

#pragma comment(lib, "Ws2_32.lib")

using std::cout;
using std::endl;
using std::string;

WSADATA wsaData;

BOOL TransactMSG(SOCKET ClientSocket);

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

	closesocket(ListenSocket);

	TransactMSG(ClientSocket);

	_getch();
	return 0;
}

BOOL TransactMSG(SOCKET ClientSocket)
{
	char recvbuf[DEFAULT_BUFLEN];
	int iResult, iSendResult;
	int recvbuflen = DEFAULT_BUFLEN;

	// Receive until the peer shuts down the connection
	do {

		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			cout << "Bytes received: " << iResult << "\n";
			recvbuf[iResult] = 0;
			cout << "Message from client: " << recvbuf << endl;;

			// Echo the buffer back to the sender
			iSendResult = send(ClientSocket, recvbuf, iResult, 0);
			if (iSendResult == SOCKET_ERROR) {
				cout << "send failed: " << WSAGetLastError() << "\n";
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			cout << "Bytes sent: " << iSendResult << "\n";
		}
		else if (iResult == 0)
			cout << "Connection closing...\n";
		else {
			cout << "recv failed: " << WSAGetLastError() << "\n";
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}

	} while (iResult > 0);

	if (recvbuf == "exit") {
		iResult = shutdown(ClientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
		closesocket(ClientSocket);
		WSACleanup();
	}
	return 0;
}
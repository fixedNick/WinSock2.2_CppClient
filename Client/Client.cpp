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
using std::cin;

WSADATA wsaData;

BOOL SendMSG(SOCKET ConnectSocket, string msg);

int main() {
	std::cout << "Client application started!" << std::endl;

	int iResult;

	// Initialize Winsock
	// MAKEWORD 2,2 -> Запрашивание версии win сокетов 2.2 
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cout << "WSAStartup failed: " << iResult << "\n";
		return 1;
	}

	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;


	// Resolve the server address and port
	iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed: " << iResult << "\n";
		WSACleanup();
		return 1;
	}

	SOCKET ConnectSocket = INVALID_SOCKET;

	ptr = result;

	// Create a SOCKET for connecting to server
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
		ptr->ai_protocol);

	if (ConnectSocket == INVALID_SOCKET) {
		cout << "Error at socket(): " << WSAGetLastError() << "\n";
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}

	// Should really try the next address returned by getaddrinfo
	// if the connect call failed
	// But for this simple example we just free the resources
	// returned by getaddrinfo and print an error message

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		cout << "Unable to connect to server!\n";
		WSACleanup();
		return 1;
	}


	while (true) 
	{
		cout << "Write message: ";
		string msg;
		cin >> msg;
		BOOL res = SendMSG(ConnectSocket, msg);
		if (!res)
		{
			cout << "Connection closed..\n";
			break;
		}
	}

	_getch();
	return 0;
}

BOOL SendMSG(SOCKET ConnectSocket, string msg)
{
	int recvbuflen = DEFAULT_BUFLEN;

	const char* sendbuf = msg.c_str();
	char recvbuf[DEFAULT_BUFLEN];

	int iResult;

	// Send an initial buffer
	iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		cout << "send failed: " << WSAGetLastError() << "\n";
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	printf("Bytes Sent: %ld\n", iResult);

	// shutdown the connection for sending since no more data will be sent
	// the client can still use the ConnectSocket for receiving data
	if (msg == "exit") {
		iResult = shutdown(ConnectSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			cout << "shutdown failed: " << WSAGetLastError() << "\n";
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}
	}

	// Receive data until the server closes the connection

	cout << "Message from server: ";
	while (true)
	{
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			recvbuf[iResult] = 0;
			cout << recvbuf << "\n";
			cout << "Bytes received: " << iResult << "\n";

			if (string(&recvbuf[0], iResult) == "exit")
				return 0;

			if (iResult >= DEFAULT_BUFLEN)
				continue;
			else
				break;
		}
		else cout << "recv failed: " << WSAGetLastError() << "\n";
	}
}
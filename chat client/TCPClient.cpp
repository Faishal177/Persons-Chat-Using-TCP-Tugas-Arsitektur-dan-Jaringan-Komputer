/*
	langkag-langkah untuk mempersiapkan Winsock client (dari https://docs.microsoft.com/en-us/windows/desktop/winsock/winsock-client-application):
		- Membuat socket. 
		- Connect kan socket ke IP address & port server. 
			-> IP address & port server di stored didalam sockaddr_in variable, hint. 
		- Sending & receiving  data pada client. 
			->untuk receiving, gunakan external thread untuk terus menerus receive data. 
			-> untuk sending, terus menerus check untuk input & send message ketika user menekan enter. 
				-> bagian ini diselesaikan di main.cpp file. 
		- Disconnect client. 

*/

#include "TCPClient.h"
#include <iostream>
#include <string>
#include <thread>

using namespace std;



TCPClient::TCPClient()
{
	recvThreadRunning = false; 
}


TCPClient::~TCPClient()
{
	closesocket(serverSocket); 
	WSACleanup();
	if (recvThreadRunning) {
		recvThreadRunning = false;
		recvThread.join();	//Destroy safely ke thread. 
	}
}


bool TCPClient::initWinsock() {
	
	WSADATA data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0) {
		cout << "Error: can't start Winsock." << endl;
		return false;
	}
	return true;
}

SOCKET TCPClient::createSocket() {

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		cout << "Error: can't create socket." << endl;
		WSACleanup();
		return -1;
	}

	//Specify data untuk hint structure. 
	hint.sin_family = AF_INET;
	hint.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverIP.c_str(), &hint.sin_addr);

	return sock;

}

void TCPClient::threadRecv() {

	recvThreadRunning = true;
	while (recvThreadRunning) {

		char buf[4096];
		ZeroMemory(buf, 4096);

		int bytesReceived = recv(serverSocket, buf, 4096, 0);	
		if (bytesReceived > 0) {			//jika client disconnects, bytesReceived = 0; jika error, bytesReceived = -1;
				
			std::cout << string(buf, 0, bytesReceived) << std::endl;

		}

	}
}

void TCPClient::connectSock() {

	//jika !initWinsock -> return false. 

	serverSocket = createSocket();

	int connResult = connect(serverSocket, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR) {
		cout << "Error: can't connect to server." << endl;
		closesocket(serverSocket);
		WSACleanup();
		return;
	}

}

void TCPClient::sendMsg(string txt) {

	if (!txt.empty() && serverSocket != INVALID_SOCKET) {

		send(serverSocket, txt.c_str(), txt.size() + 1, 0);

	}

}

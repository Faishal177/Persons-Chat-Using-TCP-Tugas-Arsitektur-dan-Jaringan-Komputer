/*
	langkah-langkah untuk mempersiapkan Winsock server (dari https://docs.microsoft.com/en-us/windows/desktop/winsock/winsock-server-application):
		- inisialisasi Winsock, winsock sendiri adalah singkatan dari Windows socket API disingkat Winsock, 
		  merupakan sebuah mekanisme interprocess communication yang menyediakan sarana komunikasi dua arah berorientasi koneksi 
		  atau komunikasi tanpa koneksi antara proses-proses di dalam dua komputer di dalam sebuah jaringan. 
		- Membuat soket listening untuk server. 
		- ikatkan socket listening ke server IP address & listening port. 
			-> gunakan sockaddr_in structure untuk masalah ini.
		- terima connection dari client. 
			-> sekali koneksi terbentuk, recv() function digunakan untuk receive data dari specific client. 
		- menerima dan mengirim data. 
		- Disconnect server jika semua selesai. 

	untuk mengatasi multiple clients didalam server tanpa menggunakan multithreading, gunakan the select() function.
	(dari https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/)
		- Select allows untuk store multiple active sockets didalam file descriptor. 
		- Select will detect jika socket activated. 
			-> ketika socket "activated", dua hal bisa terjadi: 
				1. jika socket adalah listening socket, artinya client baru mencoba untuk connect ke server. 
				2. jika tidak, artinya connected client mengirim data. 
*/

#include "TCPServer.h"
#include <iostream>
#include <string>
#include <sstream>

const int MAX_BUFFER_SIZE = 4096;			//konstan value untuk buffer size = dimana akn menstore data received.


TCPServer::TCPServer() { }


TCPServer::TCPServer(std::string ipAddress, int port)
	: listenerIPAddress(ipAddress), listenerPort(port) {
}

TCPServer::~TCPServer() {
	cleanupWinsock();			//Cleanup Winsock ketika server shuts down. 
}


//Function untuk check apa bisa untuk initialize Winsock & memulai server. 
bool TCPServer::initWinsock() {

	WSADATA data;
	WORD ver = MAKEWORD(2, 2);

	int wsInit = WSAStartup(ver, &data);

	if (wsInit != 0) {
		std::cout << "Error: Tidak bisa initialize Winsock." << std::endl; 
		return false;
	}

	return true;

}


//Function ymenciptakan listening socket server. 
SOCKET TCPServer::createSocket() {

	SOCKET listeningSocket = socket(AF_INET, SOCK_STREAM, 0);	//AF_INET = IPv4. 

	if (listeningSocket != INVALID_SOCKET) {

		sockaddr_in hint;		//Structure digunakan untuk bind IP address & port ke specific socket. 
		hint.sin_family = AF_INET;		//Memberitahu hint bahwa network address adalah IPv4 addresses. 
		hint.sin_port = htons(listenerPort);	//Memberitahu hint port apa yang digunakan. 
		inet_pton(AF_INET, listenerIPAddress.c_str(), &hint.sin_addr); 	//Converts IP string ke bytes & melewatinya ke hint. hint.sin_addr adalah the buffer. 

		int bindCheck = bind(listeningSocket, (sockaddr*)&hint, sizeof(hint));	//Bind listeningSocket ke hint structure. membertiahu apa IP address family & port untuk digunakan. 

		if (bindCheck != SOCKET_ERROR) {			//jika bind OK:

			int listenCheck = listen(listeningSocket, SOMAXCONN);	//memberitahu socket untuk listening. 
			if (listenCheck == SOCKET_ERROR) {
				return -1;
			}
		}

		else {
			return -1;
		}

		return listeningSocket;

	}

}


//Function melakukan main work server -> evaluates sockets & baik menerima connections atau menerima data. 
void TCPServer::run() {

	char buf[MAX_BUFFER_SIZE];		//menciptakan buffer untuk menerima data dari clients. 
	SOCKET listeningSocket = createSocket();		//menciptakan listening socket untuk server. 

	while (true) {

		if (listeningSocket == INVALID_SOCKET) {
			break;
		}

		fd_set master;				//File descriptor menympan semua sockets.
		FD_ZERO(&master);			//mengosongkan file-file descriptor. 

		FD_SET(listeningSocket, &master);		//menambah listening socket ke file descriptor. 

		while (true) {

			fd_set copy = master;	//menciptakan file baru descriptor karena file descriptor dihancurkan setiap saat. 
			int socketCount = select(0, &copy, nullptr, nullptr, nullptr);				//Select() menentukan status dari sockets & returns, sockets melakukan "work". 

			for (int i = 0; i < socketCount; i++) {				//Server hanya bisa menerima connection & receive msg dari client. 

				SOCKET sock = copy.fd_array[i];					//Loop melalui semua sockets di file descriptor, identified sebagai "active". 

				if (sock == listeningSocket) {				//Case 1: menertima connection baru.

					SOCKET client = accept(listeningSocket, nullptr, nullptr);		//menerima incoming connection & identify sebagai new client. 
					FD_SET(client, &master);		//memasukkan connection bru kedalam list of sockets.  
					std::string welcomeMsg = "Welcome Kedalam Chat.\n";			//Notify client jika memasuki chat. 
					send(client, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);
					std::cout << "New user joined the chat." << std::endl;			//Log connection di server side. 

				}
				else {										//Case 2: menerima msg.	

					ZeroMemory(buf, MAX_BUFFER_SIZE);		//membersihkan buffer sebelum receiving data. 
					int bytesReceived = recv(sock, buf, MAX_BUFFER_SIZE, 0);	//Receive data ke buf & memesakukan ke dalam bytesReceived. 

					if (bytesReceived <= 0) {	//No msg = drop client. 
						closesocket(sock);
						FD_CLR(sock, &master);	//Remove connection dari file director.
					}
					else {						//kirim msg ke clients lain & tidak listening socket. 

						for (int i = 0; i < master.fd_count; i++) {			//Loop melalui sockets. 
							SOCKET outSock = master.fd_array[i];	

							if (outSock != listeningSocket) {

								if (outSock == sock) {		//jika current socket adalah yang mengirim message:
									std::string msgSent = "Message delivered.";
									send(outSock, msgSent.c_str(), msgSent.size() + 1, 0);	//beritahu client jika msg sudah delivered. 	
								}
								else {						//jika current sock bukan sender -> maka harus receive the msg. 
									//std::ostringstream ss;
									//ss << "SOCKET " << sock << ": " << buf << "\n";
									//std::string strOut = ss.str();
									send(outSock, buf, bytesReceived, 0);		//kirim msg ke current socket. 
								}

							}
						}

						std::cout << std::string(buf, 0, bytesReceived) << std::endl;			//Log the message ke server side. 

					}

				}
			}
		}


	}

}


//Function to kirim message ke specific client. 
void TCPServer::sendMsg(int clientSocket, std::string msg) {

	send(clientSocket, msg.c_str(), msg.size() + 1, 0);

}


void TCPServer::cleanupWinsock() {

	WSACleanup();

}


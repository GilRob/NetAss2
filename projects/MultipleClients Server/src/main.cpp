
/*
- Recieve Updates from all clients
- pack everything into one packet and distrbute it to all clients

- keep track of the clients - use list, array, vector of clients
- update interval 100ms
	- the bigger interval, more interpolation your client has to do, have to do more prediction
	- in our games it can be a variable so users can change it

-Client Connects
	-create a thread
-Server sends the update
	go through list of clients adn send update packet to each client in that list

- we're going to having multiple threads and the update buffer is going to be shared by all the threads
	- this is going to be out shared resource and we're going to have to syncronize the use of this resource
	- we can't have 2 thread writing to it at the same time
		- do this by using mutex objects
		- buffer will be a mutex object
*/

//GAME SERVER
/*
////////////////////////////////////

Jessica Le - 100555079
Gil Robern - 100651824

////////////////////////////////////
*/

#include <iostream>
#include <WinSock2.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GLM/glm.hpp>

#pragma comment(lib, "ws2_32.lib")

#define MAXHOSTS 32
#define MSG_SIZE 512

int UPDATE_INTERVAL = 100;
//provide abstraction of a resource, don't need to know much about resource itself to use it
//mutex is a program object that allows multiple program threads to share the same resource
HANDLE hMutex; // mutex handles

char buf[MSG_SIZE] = { " " }; //shared resource

SOCKET host[MAXHOSTS]; //array of client sockets

int h = 0; //host count

GLFWwindow* window;

//DWORD is a double word
//will be executed in a single thread
//send - be called as thread
DWORD WINAPI send(LPVOID lpParam)
{
	DWORD waitResult;
	char tempBuf[MSG_SIZE] = { "" };

	while (true)
	{//request ownership of mutex
		waitResult = WaitForSingleObject(hMutex, 50L); //time we are willing to wait, if it doesnt get it will return something else (50 milliseconds) 
		switch (waitResult)
		{//thread got ownership of the object, mutex
		case WAIT_OBJECT_0:
			__try {
				strcpy(tempBuf, buf); //send data we recieve from clients and put it in tempBuf
				strcpy(buf, ""); //initalize buffer
				std::cout << "SEND: " << tempBuf << std::endl;
			}
			__finally {
				//if eveything was okay then we release the mutex so other threads can use it
				if (!ReleaseMutex(hMutex))
				{
					printf("release mutex failed");
					ExitProcess(0);
				}
				else
				{
					//printf("Mutex Released Send\n");
					break;
				}
			}
		case WAIT_TIMEOUT:
			printf("Time out elpased\n");
			return FALSE;
		case WAIT_ABANDONED:
			printf("Mutex abandonded\n");
		}//end swwitch

		//send tempbuf to all connected clients
		for (int i = 0; i < h; i++)
		{
			send(host[i], tempBuf, sizeof(tempBuf), 0);
		}
		strcpy(tempBuf, ""); //clean buffer after we send it

		//since its instead a thread, frce out interval to sleep
		Sleep(UPDATE_INTERVAL);

	}//end while

	return 1;
}

//recieve - be called as thread
DWORD WINAPI recieve(LPVOID lpParam)
{
	printf("thread created!\n");

	SOCKET client = (SOCKET)lpParam;

	char recBuf[MSG_SIZE];
	char sendBuf[MSG_SIZE];

	int sError;

	//char tempbuf;

	DWORD waitResult;

	//mode in 1 bc its non blocking mode
	//we need o do this bc we are recieving
	u_long mode = 1;

	if (ioctlsocket(client, FIONBIO, &mode) != NO_ERROR)
		printf("Non blocking error\n");

	//recieve loop
	while (true)
	{
		//clean buffer
		strcpy(recBuf, ""); // copy nothing into recvBuf

		if (recv(client, recBuf, sizeof(recBuf), 0) == 0)
		{//this function recieves stuff

			//if its not fine then error check
			printf("recv error \n");
			closesocket(client);
			ExitThread(0);
		}

		sError = WSAGetLastError(); //get latest error


		//going aroundbc its nont blocking?? (before &&)
		//check if we received something (after &&)
		if (sError != WSAEWOULDBLOCK && strcmp(recBuf, "") != 0)
		{
			//if its different from 0 = that we have recieved something
			//try to get the mutex and try to save something
			waitResult = WaitForSingleObject(hMutex, 50L); // we will wait 50 miliseconds for the mutex

			//if we can get a hold of the mutex object....
			switch (waitResult)
			{
			case WAIT_OBJECT_0:
				__try {
					//copy contents from recBuf, stuff we just received, into the buffer that is the shared object
					//append data to this buffer (shared) and send all values ive recieved at once
					strcat(buf, recBuf); //save the contents of my buffer into the shared buffer
					strcat(buf, " "); //seperate osition updates from different clients
					std::cout << "RECIEVE: " << recBuf << std::endl;
				}
				__finally {
					if (!ReleaseMutex(hMutex))// release the mutex so others can use it
					{
						printf("release mutext failed");
						ExitProcess(0);
					}
					else {
						//printf("mutex releaseRecv\n");
						break;
					}
				}
			case WAIT_TIMEOUT:

				printf("Timeout\n");
				return FALSE;

			case WAIT_ABANDONED:
				printf("mutex abandoned\n");

			} //end switch
		}//end if
	}

}
std::string input;
void keyboard()
{
	//doesn't work atm
	std::cin >> input;
	if (input == "a") {
		UPDATE_INTERVAL += 25;
		std::cout << UPDATE_INTERVAL;
	}
	if (input=="z") {
		UPDATE_INTERVAL -= 25;
		std::cout << UPDATE_INTERVAL;
		if (UPDATE_INTERVAL <= 0.0f)
		{
			UPDATE_INTERVAL = 25.0f;
		}
	}
}

int main()
{
	sockaddr_in from; //when the client connect address im going to store, where im getting from
	int fromlen = sizeof(from);

	HANDLE aThread[MAXHOSTS]; //as many threads as the max hosts we haev
	DWORD threadID;

	WSADATA wsaData;

	hMutex = CreateMutex(NULL, FALSE, NULL); //(NULL bc security attributes, inital owner, name of mutex) this one has no inital owner, we dont need of a mutex bc we have a handle for it

	if (hMutex == NULL)
		printf("Mutex failed %d\n", GetLastError());
	else
		printf("Mutex created\n");

	//Start winsock
	int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (ret != 0)
		return 0;

	//Setup our server address
	sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8888);

	//Create a socket
	SOCKET sock; //server socket, listens for connections

	//up to you to implement a UDP socket - we prolly need this
	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == INVALID_SOCKET)
		return 0;

	//bind - at server
	if (bind(sock,(sockaddr*)&server, sizeof(server)) != 0)
		return 0;
	if (listen(sock, SOMAXCONN) != 0) //SOMAXCONN is max connections
		return 0;

	//THREADS and accept connections

	DWORD sendThread;

	CreateThread(NULL, 0, send, (LPVOID)host, 0, &sendThread); //paramteter we will pass to the thread when it is executed, passing the host socket

	SOCKET client;

	//accept connections
	while (true)
	{
		//keyboard();
		//try to accept connections 
		client = accept(sock, (struct sockaddr*) &from, &fromlen);
		printf("Client connected!!\n");

		//store the client into oour array of hosts
		host[h] = client; //store client socket at that position

		//create a thread for the connected client
		//(LPVOID)client client we pass to the thread, how we get it to the thread, fuction recieve, above
		aThread[h] = CreateThread(NULL, 0, recieve, (LPVOID)client, 0, &threadID);

		h++;
	}

	closesocket(sock);

	WSACleanup();
	return 1;

}
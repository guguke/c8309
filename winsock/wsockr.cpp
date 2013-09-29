/*
Create a TCP socket
*/
#include<stdio.h>
#include<winsock2.h>
#include<conio.h>
#pragma comment(lib,"ws2_32.lib") //Winsock Library
int main(int argc , char *argv[])
{
	WSADATA wsa;
	SOCKET s;
	struct sockaddr_in server;
	char *message , server_reply[2000];
	int recv_size;
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
	{
		printf("Failed. Error Code : %d",WSAGetLastError());
		return 1;
	}
	printf("Initialised.\n");
	//Create a socket
	if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d" , WSAGetLastError());
	}
	printf("Socket created.\n");
	server.sin_addr.s_addr = inet_addr("192.168.1.224");
	server.sin_family = AF_INET;
	server.sin_port = htons( 2000 );
	//Connect to remote server
	if (connect(s , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("connect error");
		return 1;
	}
	puts("Connected");
	//Send some data
	message = "GET / HTTP/1.1\r\n\r\n";
	if( send(s , message , strlen(message) , 0) < 0)
	{
		puts("Send failed");
		return 1;
	}
	puts("Data Send\n");
	//Receive a reply from the server
	if((recv_size = recv(s , server_reply , 2000 , 0)) == SOCKET_ERROR)
	{
		puts("recv failed");
	}
	puts("Reply received\n");
	//Add a NULL terminating character to make it a proper string before printing
	server_reply[recv_size] = '\0';
	puts(server_reply);
	getch();
	return 0;
}

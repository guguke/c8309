#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
//#define NAME "socket"
/*
* This program creates a UNIX domain datagram socket, binds a
* name to it, then reads from the socket.
*/
int main(int argc,char *argv[])
{
	int socket_fd;
	struct sockaddr_un server_address; 
	struct sockaddr_un client_address; 
	int bytes_received, bytes_sent, address_length, integer_buffer;
	socklen_t address_length;
	char buf[200];

	printf(" usage: urf name_unix_socket\n");
	if(argc<2)return;

	if((socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
	{
		perror("client: socket");
		return 1;
	}
#if 0
	memset(&client_address, 0, sizeof(struct sockaddr_un));
	client_address.sun_family = AF_UNIX;
	strcpy(client_address.sun_path, argv[1]);

	unlink("./UDSDGCLNT");
	if(bind(socket_fd, (const struct sockaddr *) &client_address, 
		sizeof(struct sockaddr_un)) < 0)
	{
		perror("client: bind");
		return 1;
	}
#endif
	memset(&server_address, 0, sizeof(struct sockaddr_un));
	server_address.sun_family = AF_UNIX;
	strcpy(server_address.sun_path, argv[1]);

	integer_buffer = 5;
#if 0
	bytes_sent = sendto(socket_fd, (char *) &integer_buffer, sizeof(int), 0,
		(struct sockaddr *) &server_address, 
		sizeof(struct sockaddr_un));
#endif
	address_length = sizeof(struct sockaddr_un);
	//bytes_received = recvfrom(socket_fd, (char *) &integer_buffer, sizeof(int), 0, 
	bytes_received = recvfrom(socket_fd, buf, 100, 0, 
		(struct sockaddr *) &(server_address),
		&address_length);

	close(socket_fd);


	if( bytes_received > 0 ){
		buf[bytes_received]=0;
		printf("%s\n", buf);
	}
	else printf(" no recv\n");

	return 0;

}





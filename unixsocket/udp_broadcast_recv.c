/*
demo-udp-03: udp-recv: a simple udp server
receive udp messages

usage:  udp-recv

Paul Krzyzanowski
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
//#include "port.h"

//#define BUFSIZE 2048

//int main(int argc, char **argv)
int udp_broadcast_recv(char *paddr,int nport,char buf,int bufsize)
{
	struct sockaddr_in myaddr;	/* our address */
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	int fd;				/* our socket */
	//unsigned char buf[BUFSIZE];	/* receive buffer */
	int n;
	int so_broadcast=1;

	/* create a UDP socket */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return -1;
	}

	/* bind the socket to any valid IP address and a specific port */
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	//myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(inet_aton(paddr,&myaddr.sin_addr)==0){
		printf("  inet_aton() failed\n");
		close(fd);
		return -2;
	}
	myaddr.sin_port = htons(nport);
	//myaddr.sin_port = htons(SERVICE_PORT);

	//if(-1 == setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&so_broadcast,sizeof so_broadcast)) fprintf(stderr," setsockopt BROADCAST error\n");
	// use SO_REUSEADDR not SO_REUSEPORT
	if(-1 == setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&so_broadcast,sizeof so_broadcast)) fprintf(stderr," setsockopt REUSEADDR error\n");

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		printf(" bind failed");
		return -3;
	}

	printf("waiting on port %d\n", nport);
	for(;;){
		recvlen = recvfrom(fd, buf, bufsize, 0, (struct sockaddr *)&remaddr, &addrlen);
		printf("received %d bytes\n", recvlen);
		if (recvlen > 0) {
			//buf[recvlen] = 0;
			printf("received message: ");
			for(n=0;n<recvlen;n++) printf(" %02x",0x0ff & buf[n]);
			printf("\n");
			break;
		}
	}
	close(fd);
	return recvlen;
}
int main(int argc,char *argv[])
{
	char addr[100];
	int nport;
	char buf[2000];
	int bufsize=1000;

	strcpy(addr,"127.255.255.255");
	nport=9010;

	printf(" usage: udp_broadcast_recv ip_addr port \n");
	printf("        udp_broadcast_recv \n");
	printf("        udp_broadcast_recv 127.255.255.255 \n");
	printf("        udp_broadcast_recv 127.255.255.255 9010 \n");

	switch(argc){
	case 3:
		nport=atoi(argv[2]);
	case 2:
		strcpy(addr,argv[1]);
	default:
		break;
	}
	printf(" udp broadcast recv :   ip:%s  port:%d \n",addr,nport);

	udp_broadcast_recv(addr,nport,buf,bufsize);
	return 0;
}



/*
demo-udp-03: udp-send: a simple udp client
send udp messages
sends a sequence of messages (the # of messages is defined in MSGS)
The messages are sent to a port defined in SERVICE_PORT

usage:  udp-send

Paul Krzyzanowski
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
//#include "port.h"

#define BUFLEN 2048
#define MSGS 10	/* number of messages to send */

//int main(void)
// nn , send times
int udp_broadcast_send(char *server,int nport,char *buf,int len,int nn)
{
	struct sockaddr_in myaddr, remaddr;
	int fd, i, slen=sizeof(remaddr);
	//char *server = "127.255.255.255";	/* change this to use a different server */
	//char buf[BUFLEN];
	//int nport=9010;
	int so_broadcast=1;

	/* create a socket */
	if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1){
		printf("socket created\n");
		return -3;
	}

	/* bind it to all local addresses and pick any port number */
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);
        printf(" sender port : %d \n", ntohs(myaddr.sin_port));

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		close(fd);
		return -1;
	}       

	if(-1 == setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&so_broadcast,sizeof so_broadcast)) fprintf(stderr," setsockopt BROADCAST error\n");
	/* now define remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */

	memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(nport);

	if (inet_aton(server, &remaddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		close(fd);
		return -2;
	}
	for(i=0;i<nn;i++){
		printf("Sending packet %d to %s port %d\n", i, server, nport);
		if (sendto(fd, buf, len, 0, (struct sockaddr *)&remaddr, slen)==-1){
			printf("sendto error \n");
			close(fd);
			return -4;
		}
		if(nn>1) sleep(10);
	}
	close(fd);
	return 0;
}
int main(int argc,char *argv[])
{
	char addr[100];
	int nport;
	char buf[2000];
	int len;
	int nn;

	strcpy(addr,"127.255.255.255");
	nport=9010;
	strcpy(buf," msg udp broadcast send ");
	len = strlen(buf);
	nn = 1;

	printf(" usage: udp_broadcast_send ip_addr port times msg_send \n");
	printf(" usage: udp_broadcast_send \n");
	printf(" usage: udp_broadcast_send 127.255.255.255 \n");
	printf(" usage: udp_broadcast_send 127.255.255.255 9010 \n");
	printf(" usage: udp_broadcast_send 127.255.255.255 9010 5 \n");
	printf(" usage: udp_broadcast_send 127.255.255.255 9010 5 msg.send.msg.send \n");

	switch(argc){
	case 5:
		strcpy(buf,argv[4]);
		len=strlen(buf);
	case 4:
		nn=atoi(argv[3]);
	case 3:
		nport=atoi(argv[2]);
	case 2:
		strcpy(addr,argv[1]);
	default:
		break;
	}
	udp_broadcast_send(addr,nport,buf,len,nn);
	return 0;
}


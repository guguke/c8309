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

#define BUFSIZE 2048

int
main(int argc, char **argv)
{
	struct sockaddr_in myaddr;	/* our address */
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	int fd;				/* our socket */
	unsigned char buf[BUFSIZE];	/* receive buffer */
        int n;
        int nport=9010;
        int so_broadcast=1;


	/* create a UDP socket */

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}

	/* bind the socket to any valid IP address and a specific port */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	//myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(inet_aton("127.255.255.255",&myaddr.sin_addr)==0){
                fprintf(stderr,"inet_aton() failed\n");
                exit(1);
                close(fd);
        }
	myaddr.sin_port = htons(nport);
	//myaddr.sin_port = htons(SERVICE_PORT);

        //if(-1 == setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&so_broadcast,sizeof so_broadcast)) fprintf(stderr," setsockopt BROADCAST error\n");
        // use SO_REUSEADDR not SO_REUSEPORT
        if(-1 == setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&so_broadcast,sizeof so_broadcast)) fprintf(stderr," setsockopt REUSEADDR error\n");

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}

	/* now loop, receiving data and printing what we received */
	for (n=0;n<5;n++) {
		printf("waiting on port %d\n", nport);
		recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		printf("received %d bytes\n", recvlen);
		if (recvlen > 0) {
			buf[recvlen] = 0;
			printf("received message(%d): \"%s\"\n", n, buf);
		}
	}
        close(fd);
	/* never exits */
}

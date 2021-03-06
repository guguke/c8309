/* serverprog.c - a stream socket server demo */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

/* listen on sock_fd, new connection on new_fd */
int new_fd=-1;    // to global
int gUDPfd;

/* the port users will be connecting to */
#define MYPORT 13500

/* how many pending connections queue will hold */
#define BACKLOG 10

pthread_t tid[2];
int ret1,ret2;

// 127.255.255.255 9010 
int udp_broadcast_init(char *paddr,int nport)
{
	struct sockaddr_in myaddr;	/* our address */
	//struct sockaddr_in remaddr;	/* remote address */
	//socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	//int fd;				/* our socket */    //  ==> global udpfd
	//unsigned char buf[BUFSIZE];	/* receive buffer */
	int n;
	int so_broadcast=1;
	int i;
	//char ip[100];

	/* create a UDP socket */
	if ((gUDPfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return -1;
	}

	/* bind the socket to any valid IP address and a specific port */
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	//myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(inet_aton(paddr,&myaddr.sin_addr)==0){
		printf("  inet_aton() failed\n");
		close(gUDPfd);
		return -2;
	}
	myaddr.sin_port = htons(nport);
	//myaddr.sin_port = htons(SERVICE_PORT);

	//if(-1 == setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&so_broadcast,sizeof so_broadcast)) fprintf(stderr," setsockopt BROADCAST error\n");
	// use SO_REUSEADDR not SO_REUSEPORT
	if(-1 == setsockopt(gUDPfd,SOL_SOCKET,SO_REUSEADDR,&so_broadcast,sizeof so_broadcast)) fprintf(stderr," setsockopt REUSEADDR error\n");

	if (bind(gUDPfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		printf(" bind failed");
		return -3;
	}

	return 0;
#if 0
	printf("waiting on port %d\n", nport);
	for(i=0;i<nn;){
		recvlen = recvfrom(fd, buf, bufsize, 0, (struct sockaddr *)&remaddr, &addrlen);
		printf("received %d bytes\n", recvlen);
		if (recvlen > 0) {
			ip[0]=0;
			inet_ntop(AF_INET,&remaddr.sin_addr,ip,90);
			printf(" recv from : ip:%s  port:%d \n",ip,ntohs(remaddr.sin_port));

			//buf[recvlen] = 0;
			printf("received message: ");
			for(n=0;n<recvlen;n++) printf(" %02x",0x0ff & buf[n]);
			printf("\n");
			i++;
		}
	}
	close(fd);
	return recvlen;
#endif
}


void* threadUDPr(void *arg)
{
	int len;
	char buf[600],outbuf[600];
	int recvlen,outlen;
	int op;
	char ip[100];
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */

	int i = 0;
	int n;
	pthread_t id = pthread_self();

	strcpy(buf,"alskjd asl das dfas df asdf asd fas  \n");
	len = strlen(buf);
	//len=read(new_fd,buf,200);
	//if(len<0)continue;
	//buf[len]=0;
	//printf(" recv from client(%d) : %s \n",n1,buf);
	udp_broadcast_init("127.255.255.255",9010);
	for(;;){
		len=500;
		recvlen = recvfrom(gUDPfd, buf, len, 0, (struct sockaddr *)&remaddr, &addrlen);
		printf("received %d bytes\n", recvlen);
		if (recvlen > 0) {
			ip[0]=0;
			inet_ntop(AF_INET,&remaddr.sin_addr,ip,90);
			printf(" recv from : ip:%s  port:%d \n",ip,ntohs(remaddr.sin_port));

			//buf[recvlen] = 0;
			//printf("received message: ");
			//for(n=0;n<recvlen;n++) printf(" %02x",0x0ff & buf[n]);
			//printf("\n");
			op=udp2send(buf,recvlen,outbuf,&outlen);
			/////////////// same as tcp send back //////////////////////////////////////////////
			if(new_fd==-1)continue;
			switch(op){
			case 0:// sendback
				if(outlen>0){
					if(send(new_fd, outbuf,outlen, 0) == -1){
						perror("Server-send() error 1 lol!");
					}
					else printf(" udp ==> tcp client \n");
				}
				break;
			default:
				break;
			}
		}
	}
	close(gUDPfd);
#if 0
	for(i=0; i<(0x0FFFF);i++);

	if(pthread_equal(id,tid[0]))
	{
		printf("\n First thread processing done\n");
		ret1  = 100;
		//pthread_exit(&ret1);
	}
	else
	{
		printf("\n Second thread processing done\n");
		ret2  = 200;
		pthread_exit(&ret2);
	}
#endif
	printf(" waitUDP == thread return null \n");
	return NULL;
}

int startUDPrecv(void)
{
	int i = 0;  
	int err;
	//int *ptr[2];

	//while(i < 2)	{
		err = pthread_create(&(tid[i]), NULL, &threadUDPr, NULL);
		if (err != 0)
			printf("\ncan't create thread :[%s]", strerror(err));
		else
			printf("\n Thread created successfully\n");

		//i++;
	//}
#if 0
	pthread_join(tid[0], (void**)&(ptr[0]));
	//pthread_join(tid[1], (void**)&(ptr[1]));

	if( ptr[0] != NULL )printf("\n return value from first thread is [%d]\n", *ptr[0]);
	//printf(" main:  thread exit return null \n");
	//printf("\n return value from second thread is [%d]\n", *ptr[1]);

	printf(" end of startUDPrecv \n");
#endif
	return err;
}
int stopUDPrecv(void)
{
	int *ptr[2];

	pthread_join(tid[0], (void**)&(ptr[0]));
	//pthread_join(tid[1], (void**)&(ptr[1]));

	if( ptr[0] != NULL )printf("\n return value from first thread is [%d]\n", *ptr[0]);
	//printf(" main:  thread exit return null \n");
	//printf("\n return value from second thread is [%d]\n", *ptr[1]);

	printf(" end of stopUDPrecv \n");
	return 0;
}


void sigchld_handler(int s)
{
	while(wait(NULL) > 0);
	printf("     sigchld exit \n");
}

int main(int argc, char *argv[ ])
{
	/* listen on sock_fd, new connection on new_fd */
	int sockfd;
	//int new_fd;    // to global

	/* my address information */
	struct sockaddr_in my_addr;

	/* connector��s address information */
	struct sockaddr_in their_addr;
	int sin_size;
	struct sigaction sa;
	int yes = 1;
	int numCli;
	char buf[400],outbuf[400];
	int len,poutlen[10];
	int n1;
	int threadErr;
	int retTcpSend;
	int op;
	int i;
	char *p;

	init_gd();///////////////// init global data

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)	{
		perror("Server-socket() error lol!");
		return -1;
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)	{
		perror("Server-setsockopt() error lol!");
		return -2;
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYPORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	printf("Server-Using %s and port %d...\n", inet_ntoa(my_addr.sin_addr), MYPORT);

	/* zero the rest of the struct */
	memset(&(my_addr.sin_zero), '\0', 8);

	if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)	{
		perror("Server-bind() error");
		return -3;
	}

	if(listen(sockfd, BACKLOG) == -1)	{
		perror("Server-listen() error");
		return -4;
	}
	printf("Server-listen() is OK...Listening...\n");

	/* clean all the dead processes */
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if(sigaction(SIGCHLD, &sa, NULL) == -1){
		perror("Server-sigaction() error");
		exit(1);
	}
	else 		printf("Server-sigaction() is OK...\n");

	/* accept() loop */
	threadErr=startUDPrecv();
	for(numCli=1;;)	{// loop for ever,  numCli : for test
		sin_size = sizeof(struct sockaddr_in);
		if((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)		{
			perror("Server-accept() error");
			continue;
		}
		else 			printf("Server-accept() is OK...  numClient:%d \n",numCli++);

		printf("Server-new socket, new_fd is OK...\n");
		printf("Server: Got connection from %s\n", inet_ntoa(their_addr.sin_addr));

		/* this is the child process */
		/////////// child , stop thread 
		printf("  ===== before  fork , stop thread  ============= \n");
		pthread_cancel(tid[0]);
		// kill UDP thread // stop UDPthread
		if(threadErr!=0) stopUDPrecv();

		if(!fork())		{
			/* child doesn��t need the listener */
			close(sockfd);

			// wait ser2net local udp, 
			printf("      fork child , start udp thread \n");
			threadErr=startUDPrecv();

			// recv from client , 
			for(n1=0,retTcpSend=0;retTcpSend>=0;n1++){// n1: no use 
				len=read(new_fd,buf,300);
				if(len<0){ 
					printf(" read from client return error, break, kill thread, fork exit \n");
					break;  // read error 
					//continue;   // error should break 
				}
				else if(len==0){ // normal end of client socket connect 
					printf(" client socket read return 0, end of the socket \n");
					break;
				}

				////////////////
				printf(" recv tcp client, len:%d -0x- ",len);
				for(i=0;i<len;i++) printf(" %02x",0x0ff & buf[i]);
				printf(" \n");

				op=tcp2send(buf,len,outbuf,poutlen);      // rcv2send 

				printf(" prep reply tcp client, len:%d -0x- ",poutlen[0]);
				for(i=0;i<poutlen[0];i++) printf(" %02x",0x0ff & outbuf[i]);
				printf(" \n");

				switch(op){
				case 0:// send back
					if(send(new_fd, outbuf,poutlen[0], 0) == -1){
						perror("   reply to client error , for break \n");
						retTcpSend = -1;// connect break;
					}
					break;
				case 9:// send back , idx=0x0ff
					p=outbuf;
					for(i=0;i<9;i++){
						if(send(new_fd, p,poutlen[i], 0) == -1){
							perror("   reply to client error , for break \n");
							retTcpSend = -1;// connect break;
							break;
						}
						p+=poutlen[i];
					}
					break;
				case 0x51:
					if(send(new_fd, outbuf,poutlen[0], 0) == -1){
						perror("   reply to client error , for break \n");
						retTcpSend = -1;// connect break;
						break;
					}
					setnewip(buf,len);
					break;
				default:
					break;
				}
			}
			pthread_cancel(tid[0]);
			// kill UDP thread // stop UDPthread
			if(threadErr!=0) stopUDPrecv();

			close(new_fd);
			new_fd=-1;
			printf(" == fork end \n");
			return 0;
		}
		else{
			printf(" fork main , restart udp thread \n");
			threadErr=startUDPrecv();
			printf("  fork parent,              \n");
		}

		/* parent doesn��t need this*/
		close(new_fd);
		new_fd=-1;
		printf("Server-new socket, new_fd closed successfully...    wait new client(%d) \n",numCli);
	}

	pthread_cancel(tid[0]);
	// kill UDP thread // stop UDPthread
	if(threadErr!=0) stopUDPrecv();
	close(sockfd);
	printf("  main exit \n");
	return 0;

}




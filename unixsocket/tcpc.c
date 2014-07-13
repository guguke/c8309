/* Sample TCP client */

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

struct in_addr gip;

int crc(char *p)
{
	int ret=0;
	int i,n;
	int len=0x0ff & p[2];
	n=len+3;
	for(i=0;i<n;i++)ret+=0x0ff&p[i];
	ret = 0x0ff - (0x0ff&ret);
	return ret;
}

int main(int argc, char**argv)
{
	int sockfd,n;
	struct sockaddr_in servaddr,cliaddr;
	char sendline[300];
	char recvline[1000];
	int nport=13500;
	int sendlen=5;
	char ip[30];//="192.168.2.24";
	int i,j;
	int nl;

	strcpy(ip,"127.0.0.1");
	printf(" usage: tcpc ip port \n");
	printf(" usage: tcpc 192.168.2.24 \n");
	printf(" usage: tcpc 192.168.2.24 3490 \n");

	sendline[0]=0x50;
	sendline[1]=0x00;
	sendline[2]=0x01;
	sendline[3]=0x00;
	sendline[4]=crc(sendline);
	//sendline[5]=0x;
	sendlen=(0x0ff & sendline[2])+4;

	switch(argc){
	case 3:
		nport=atoi(argv[2]);
	case 2:
		strcpy(ip,argv[1]);
	default:
		break;
	}

	printf(" == server:%s port:%d \n",ip,nport);

	sockfd=socket(AF_INET,SOCK_STREAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(ip);
	servaddr.sin_port=htons(nport);

	connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	//if (fgets(sendline, 10000,stdin) != NULL)
	//{
	sendto(sockfd,sendline,sendlen,0,
		(struct sockaddr *)&servaddr,sizeof(servaddr));
	n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
	recvline[n]=0;
	printf(" tcp socket recv: ");
	for(i=0;i<n;i++) printf("%02x ",0x0ff & recvline[i]);
	printf(" \n  recv len: %d \n",n);

	bcopy(recvline+3,&nl,4);
	gip.s_addr=ntohl(nl);
	printf(" ip : %s \n",inet_ntoa(gip));
	bcopy(recvline+7,&nl,4);
	gip.s_addr=ntohl(nl);
	printf(" mask : %s \n",inet_ntoa(gip));
	//fputs(recvline,stdout);
	//}

	//////////////////////////////////////////////////////////////////////////////
	for(j=0;j<8;j++){
	sendline[0]=0x50;
	sendline[1]=0x10+j;
	sendline[2]=0x01;
	sendline[3]=0x00;
	sendline[4]=crc(sendline);
	//sendline[5]=0x;
	sendlen=(0x0ff & sendline[2])+4;

	sendto(sockfd,sendline,sendlen,0,
		(struct sockaddr *)&servaddr,sizeof(servaddr));
	n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
	recvline[n]=0;
	printf(" tcp socket recv: ");
	for(i=0;i<n;i++) printf("%02x ",0x0ff & recvline[i]);
	printf(" \n  recv len: %d \n",n);

	bcopy(recvline+9,&nl,4);
	gip.s_addr=ntohl(nl);
	printf(" ip : %s \n\n",inet_ntoa(gip));
	}
	/////////////////////////////////////////////////////////////////////////////////////
	sendline[0]=0x50;
	sendline[1]=0x20;
	sendline[2]=0x01;
	sendline[3]=0x00;
	sendline[4]=crc(sendline);
	//sendline[5]=0x;
	sendlen=(0x0ff & sendline[2])+4;

	sendto(sockfd,sendline,sendlen,0,
		(struct sockaddr *)&servaddr,sizeof(servaddr));
	n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
	recvline[n]=0;
	printf(" tcp socket recv: ");
	for(i=0;i<n;i++) printf("%02x ",0x0ff & recvline[i]);
	printf(" \n  recv len: %d \n",n);

	//////////////////////////////////////////////////////////////////////// 
	sendline[0]=0x50;
	sendline[1]=0x0ff;
	sendline[2]=0x01;
	sendline[3]=0x00;
	sendline[4]=crc(sendline);
	//sendline[5]=0x;
	sendlen=(0x0ff & sendline[2])+4;

	printf(" tcp socket send: -0x- ");
	for(i=0;i<sendlen;i++) printf("%02x ",0x0ff & sendline[i]);
	printf(" \n           == send len: %d     \n",sendlen);
	sendto(sockfd,sendline,sendlen,0,
		(struct sockaddr *)&servaddr,sizeof(servaddr));
	sleep(3);/////////////////////////////////////////////////// man 3 sleep , seconds
	n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
	recvline[n]=0;
	printf(" tcp socket recv: ");
	for(i=0;i<n;i++) printf("%02x ",0x0ff & recvline[i]);
	printf(" \n  recv len: %d     \n",n);
	////////////////////////////////////////////   change ip 
	sendline[0]=0x51;
	sendline[1]=0x0;
	sendline[2]=0x09;
	sendline[3]=0x00;
	inet_aton("10.11.12.13",&gip);//gip,&inp);
	i=htonl(gip.s_addr);
	bcopy(&i,sendline+4,4);
	inet_aton("255.0.0.0",&gip);//gip,&inp);
	i=htonl(gip.s_addr);
	bcopy(&i,sendline+8,4);
	sendline[12]=crc(sendline);
	//sendline[5]=0x;
	sendlen=(0x0ff & sendline[2])+4;

	printf(" tcp socket send: -0x- ");
	for(i=0;i<sendlen;i++) printf("%02x ",0x0ff & sendline[i]);
	printf(" \n           == send len: %d     \n",sendlen);
	sendto(sockfd,sendline,sendlen,0,
		(struct sockaddr *)&servaddr,sizeof(servaddr));
	n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
	recvline[n]=0;
	printf(" tcp socket recv: ");
	for(i=0;i<n;i++) printf("%02x ",0x0ff & recvline[i]);
	printf(" \n  recv len: %d     \n",n);
	////////////////////////////////////////////   change ip 222222222
	sendline[0]=0x51;
	sendline[1]=0x0;
	sendline[2]=0x09;
	sendline[3]=0x00;
	inet_aton("10.11.12.13",&gip);//gip,&inp);
	i=htonl(gip.s_addr);
	bcopy(&i,sendline+4,4);
	inet_aton("255.0.0.0",&gip);//gip,&inp);
	i=htonl(gip.s_addr);
	bcopy(&i,sendline+8,4);
	sendline[12]=crc(sendline);
	//sendline[5]=0x;
	sendlen=(0x0ff & sendline[2])+4;

	printf(" tcp socket send: -0x- ");
	for(i=0;i<sendlen;i++) printf("%02x ",0x0ff & sendline[i]);
	printf(" \n           == send len: %d     \n",sendlen);
	sendto(sockfd,sendline,sendlen,0,
		(struct sockaddr *)&servaddr,sizeof(servaddr));
	n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
	recvline[n]=0;
	printf(" tcp socket recv: ");
	for(i=0;i<n;i++) printf("%02x ",0x0ff & recvline[i]);
	printf(" \n  recv len: %d     \n",n);

	//////////////////////////////////////////////////////////////////////// error crc test
	sendline[0]=0x50;
	sendline[1]=0x20;
	sendline[2]=0x01;
	sendline[3]=0x00;
	sendline[4]=crc(sendline)+0x01;
	//sendline[5]=0x;
	sendlen=(0x0ff & sendline[2])+4;

	sendto(sockfd,sendline,sendlen,0,
		(struct sockaddr *)&servaddr,sizeof(servaddr));

	return 0;
}





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

	strcpy(ip,"192.168.28.100");
	printf(" usage: tcpc_getip ip port \n");
	printf(" usage: tcpc_getip 192.168.28.100 \n");
	printf(" usage: tcpc_getip 192.168.28.100 13500 \n\n");

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
	printf(" send data(0x) :  ");
	for(i=0;i<5;i++)printf(" %02x",sendline[i] & 0x0ff);
	printf(" \n\n");

	sendto(sockfd,sendline,sendlen,0,
		(struct sockaddr *)&servaddr,sizeof(servaddr));
	n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
	recvline[n]=0;
	printf(" tcp socket recv(0x): ");
	for(i=0;i<n;i++) printf("%02x ",0x0ff & recvline[i]);
	printf(" \n  recv len: %d \n",n);

	printf(" =============== \n");
	bcopy(recvline+3,&(gip.s_addr),4);
	printf(" ip : %s \n",inet_ntoa(gip));
	bcopy(recvline+7,&(gip.s_addr),4);
	printf(" mask : %s \n",inet_ntoa(gip));
	//fputs(recvline,stdout);
	//}

	return 0;
}





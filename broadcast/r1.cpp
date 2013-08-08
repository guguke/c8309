#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h> 
#include <time.h>

#define MC_ADDR     "226.1.1.1"
#define MC_PORT     4321
#define MAXLEN      2048

void main(int argc, char **argv)
{
	int i;
	SOCKET          s;
	SOCKADDR_IN     mcAddr;
	int             nMcLen;
	char            buf[MAXLEN];      
	struct ip_mreq  ipmr;
	SOCKADDR_IN     localAddr;
	WSADATA         wsaData;
	int             nErr;      
	if(WSAStartup(0x0202, &wsaData) != 0) 	{
		//ReportErr("WSAStartup(..)");
		return;
	};
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if(s == INVALID_SOCKET)        	{
		//WSAReportErr("socket(...)");
		WSACleanup();
		return;
	} 
	////////////////////////////////////////// timeout
	int ntimeout=1000;
	unsigned int nnt=sizeof(ntimeout);
	//setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(char*)&ntimeout,nnt);
	/* */
	
	memset(&localAddr, 0, sizeof(localAddr));      
	localAddr.sin_family        = AF_INET;
	localAddr.sin_addr.s_addr   = htonl(INADDR_ANY)/*inet_addr(MC_ADDR)*/;
	localAddr.sin_port          = htons(MC_PORT);     
	nErr = bind(s, (struct sockaddr*)&localAddr, sizeof(localAddr));
	if(nErr == SOCKET_ERROR) 	{
		//WSAReportErr("bind(...)");
		WSACleanup();
		return;
	}
	/* Join multicast group */
	ipmr.imr_multiaddr.s_addr  = inet_addr(MC_ADDR);
	ipmr.imr_interface.s_addr  = htonl(INADDR_ANY);
	nErr = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&ipmr, sizeof(ipmr));
	if(nErr == SOCKET_ERROR) 	{
		//WSAReportErr("setsockopt(.. IP_ADD_MEMBERSHIP ..)");
		WSACleanup();
		return;
	}
	mcAddr.sin_family       = AF_INET;
	mcAddr.sin_addr.s_addr  = inet_addr(MC_ADDR);
	mcAddr.sin_port         = htons(MC_PORT);
	nMcLen = sizeof(mcAddr);  
	for(i=0;i<5;i++)	{
		//Sleep(1000);
		nErr = recvfrom(s, buf, MAXLEN, 0, (struct sockaddr*)&mcAddr, &nMcLen);
		if(nErr == SOCKET_ERROR)
		{
			time_t t1;
			time(&t1);
			printf("error or timeout %d error:%d\n",t1,nErr);
			continue;
		}
		if(nErr>=0)buf[nErr]=0;
		time_t t;
		time(&t);
		printf("%s %d \n", buf,t);
	}
	WSACleanup();
}


#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <time.h>
#define MC_ADDR     "226.1.1.1"
#define MC_PORT     4321
#define MAXLEN      256

// vc6 compile : prj setting add ws2_32.lib


void getTimeStr(char *str)
{
	time_t t;
	tm *now;
	time(&t);
	now=localtime(&t);
	sprintf(str,("%02d:%02d:%02d"),now->tm_hour,now->tm_min,now->tm_sec);
}

void main(int argc, char **argv)
{
	SOCKET          s;
	SOCKADDR_IN     mcAddr;
	int             nMcLen;
	int             n;
	char            buf[MAXLEN];
	char sec[40];
	WSADATA         wsaData;
	int             nErr;    
	if(WSAStartup(0x0202, &wsaData) != 0)
	{
		//ReportErr("WSAStartup(..)");
		return;
	};
	s = socket(AF_INET, SOCK_DGRAM, 0);    
	if(s == INVALID_SOCKET)
	{
		//WSAReportErr("socket(...)");
		WSACleanup();
		return;
	}
	mcAddr.sin_family       = AF_INET;
	mcAddr.sin_addr.s_addr  = inet_addr(MC_ADDR);
	mcAddr.sin_port         = htons(MC_PORT);     
	nMcLen = sizeof(mcAddr);
	n = 0;
	//for(int i=0;i<3;i++)
	//{
	//Sleep(1000);
	sec[0]=0;
	getTimeStr(sec);
	sprintf(buf, "getip 226.1.1.2 4322 %s",sec);
	nErr = sendto(s, buf, strlen(buf), 0, (struct sockaddr*)&mcAddr, nMcLen);
#if 0
	if(nErr == SOCKET_ERROR)
	{
		//WSAReportErr("sendto(...)");
		break;
	}
#endif
	printf("sent: %s   (%s:%d\n", buf,MC_ADDR,MC_PORT);
	//}
	WSACleanup();
}

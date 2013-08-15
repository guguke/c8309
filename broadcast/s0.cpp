
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#define MC_ADDR     "226.1.1.1"
#define MC_PORT     4321
#define MAXLEN      256

// vc6 compile : prj setting add ws2_32.lib

void main(int argc, char **argv)
{
	SOCKET          s;
	SOCKADDR_IN     mcAddr;
	int             nMcLen;
	int             n;
	char            buf[MAXLEN];
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
		sprintf(buf, "getip 226.1.1.2 4322 ....");
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


#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <time.h>

//#define MC_ADDR     "226.1.1.1"
//#define MC_PORT     4321
//#define MAXLEN      256

// vc6 compile : prj setting add ws2_32.lib

int rmchar(char *buf,char ch)
{
	int i;
	int len=strlen(buf);
	for(i=0;i<len;i++){
		if( (0x0ff&buf[i]) == (0x0ff&ch) ) buf[i]=0x20;
	}
	return 0;
}

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
	char            buf[1000];
	char sec[40];
	WSADATA         wsaData;
	int             nErr;   
	char mip[30];
	int mport=4321;

	strcpy(mip,"226.1.1.1");

	printf(" == usage: s0 226.1.1.1 4321 getip_226.1.1.1_4321\n");
	switch(argc){
	case 4:
		strcpy(buf,argv[3]);
		mport=atoi(argv[2]);
		strcpy(mip,argv[1]);
		break;
	case 3:
		mport=atoi(argv[2]);
	case 2:
		strcpy(mip,argv[1]);
	default:
		break;
	}

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
	mcAddr.sin_addr.s_addr  = inet_addr(mip);
	mcAddr.sin_port         = htons(mport);     
	nMcLen = sizeof(mcAddr);
	n = 0;
	//for(int i=0;i<3;i++)
	//{
	//Sleep(1000);
	if(argc < 4 ){
		sec[0]=0;
		getTimeStr(sec);
		sprintf(buf, "getip %s %d %s",mip,mport,sec);
	}
	rmchar(buf,'_');
	printf("multicast send test: %s:%d  :: %s \n",mip,mport,buf);

	nErr = sendto(s, buf, strlen(buf), 0, (struct sockaddr*)&mcAddr, nMcLen);

	if(nErr == SOCKET_ERROR)
	{
		printf(" multicast %s:%d send error\n",mip,mport);
		//WSAReportErr("sendto(...)");
	}

	printf("sent: %s   (%s:%d\n", buf,mip,mport);
	//}
	WSACleanup();
}

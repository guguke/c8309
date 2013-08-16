#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//#define MCASTADDR     "226.1.1.1"
//#define MCASTPORT      4321
#define BUFSIZE        4048
#define DEFAULT_COUNT  500

//BOOL  bSender = FALSE,      // Act as a sender?
//bLoopBack = FALSE;    // Disable loopback?

DWORD dwInterface,          // Local interface to bind to
dwMulticastGroup,     // Multicast group to join
dwCount;              // Number of messages to send/receive

short iPort;                // Port number to use

void getTimeStr(char *str)
{
	time_t t;
	tm *now;
	time(&t);
	now=localtime(&t);
	sprintf(str,("%02d:%02d:%02d"),now->tm_hour,now->tm_min,now->tm_sec);
}

// MCASTADDR : 226.1.1.1
// MCASTPORT : 4321
// sendbuf :
int sleaf(char *MCASTADDR,int MCASTPORT,char *sendbuf,int nLen)
{
    struct sockaddr_in  local,		remote;
    SOCKET              sock, sockM;
    //TCHAR               sendbuf[BUFSIZE];
    int                 len = sizeof(struct sockaddr_in),
		optval;

	dwInterface = INADDR_ANY;
    dwMulticastGroup = inet_addr(MCASTADDR);
    iPort = MCASTPORT;

    // Create the socket. In Winsock 2, you do have to specify the
    // multicast attributes that this socket will be used with.
    //
    if ((sock = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0,
		WSA_FLAG_MULTIPOINT_C_LEAF 
		| WSA_FLAG_MULTIPOINT_D_LEAF 
		| WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
    {
        printf("socket failed with: %d\n", WSAGetLastError());
        //WSACleanup();
        return -2;
    }
    // Bind to the local interface. This is done to receive data.
    local.sin_family = AF_INET;
    local.sin_port   = htons(iPort);
    local.sin_addr.s_addr = dwInterface;
    
    if (bind(sock, (struct sockaddr *)&local, 
        sizeof(local)) == SOCKET_ERROR)
    {
        printf("bind failed with: %d\n", WSAGetLastError());
        closesocket(sock);
        //WSACleanup();
        return -3;
    }
    // Set up the SOCKADDR_IN structure describing the multicast 
    // group we want to join
    //
    remote.sin_family      = AF_INET;
    remote.sin_port        = htons(iPort);
    remote.sin_addr.s_addr = dwMulticastGroup;
    //
    // Change the TTL to something more appropriate
    //
	
    optval = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, 
        (char *)&optval, sizeof(int)) == SOCKET_ERROR)
    {
        printf("setsockopt(IP_MULTICAST_TTL) failed: %d\n",
            WSAGetLastError());
        closesocket(sock);
        //WSACleanup();
        return -4;
    }
    // Disable loopback if needed
    //
#if 0
    if (bLoopBack)
    {
        optval = 0;
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP,
            (char *)&optval, sizeof(optval)) == SOCKET_ERROR)
        {
            printf("setsockopt(IP_MULTICAST_LOOP) failed: %d\n",
                WSAGetLastError());
            closesocket(sock);
            WSACleanup();
            return -1;
        }
    }
#endif
    // Join the multicast group.  Note that sockM is not used 
    // to send or receive data. It is used when you want to 
    // leave the multicast group. You simply call closesocket() 
    // on it.
    //
    if ((sockM = WSAJoinLeaf(sock, (SOCKADDR *)&remote, 
        sizeof(remote), NULL, NULL, NULL, NULL, 
        JL_BOTH)) == INVALID_SOCKET)
    {
        printf("WSAJoinLeaf() failed: %d\n", WSAGetLastError());
        closesocket(sock);
        //WSACleanup();
        return -5;
    }
	
	// Send data
	//sprintf(sendbuf, "server 1: This is a test: ");
	if (sendto(sock, (char *)sendbuf, nLen, 0,
		(struct sockaddr *)&remote, 
		sizeof(remote)) == SOCKET_ERROR)
	{
		printf("sendto failed with: %d\n", 
			WSAGetLastError());
		closesocket(sockM);
		closesocket(sock);
		//WSACleanup();
		return -7;
	}
    // Leave the multicast group by closing sock.
    // For nonrooted control and data plane schemes, WSAJoinLeaf
    // returns the same socket handle that you pass into it.
    //
	closesocket(sockM);
    closesocket(sock);
	
    //WSACleanup();
    return 0;
}

int main(int argc, char **argv)
{
	char sendbuf[1024];
	int nLen=0;
    WSADATA             wsd;
	char szTime[40];

    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
    {
        printf("WSAStartup() failed\n");
        return -1;
    }

	getTimeStr(szTime);
	sprintf(sendbuf,"getip 226.1.1.2 4322 %s",szTime);
	nLen=strlen(sendbuf);
	sleaf("226.1.1.1",4321,sendbuf,nLen);

    WSACleanup();
	return 0;
}

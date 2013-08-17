#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//struct in_addr localInterface;
//struct sockaddr_in groupSock;
//int sd;
//char databuf[1024] = "Multicast test message lol!";
//int datalen = sizeof (databuf);

int main (int argc, char *argv[])
{
	char mcastip[30],localip[30];
	int mcastport=4321;
	char sbuf[1000];
	int len;

	printf(" usage: ms 226.1.1.1 4321 192.168.1.224 sendtest\n");

	strcpy(mcastip,"226.1.1.1");
	strcpy(localip,"192.168.16.21");

	switch(argc){
	case 5:
		strcpy(sbuf,argv[4]);
		strcpy(localip,argv[3]);
		mcastport=atoi(argv[2]);
		strcpy(mcastip,argv[1]);
		break;
	case 4:
		strcpy(localip,argv[3]);
	case 3:
		mcastport=atoi(argv[2]);
	case 2:
		strcpy(mcastip,argv[1]);
	default:
		sprintf(sbuf,"multicast send test %s:%d from ip: %s  :: %s",mcastip,mcastport,localip,sbuf);
		break;
	}
	len = strlen(sbuf);
	rmchar(sbuf,'_');
    ms(mcastip,mcastport,localip,sbuf,len);
    return 0;
}

void rmchar(char *buf,char ch)
{
	int i;
	len = strlen(buf);
	for(i=0;i<len;i++){
		if( (0x0ff&ch)==(0x0ff)&buf[i]) buf[i]=0x20;
	}
	return;
}


// mip: multicast ip
// mport: multicast port
// localip: local eth? ip
// databuf: send buf
// datalen: send data len
int ms(char *mip,int mport,char *localip,char *databuf,int datalen){
    struct in_addr localInterface;
    struct sockaddr_in groupSock;
    int sd;
    //char databuf[1024] = "Multicast test message lol!";
    //int datalen = sizeof (databuf);
    
    //int mport=4321;
    //char mip[30];
    //char localip[30];
	
    //strcpy(mip,"226.1.1.1");
    //strcpy(localip,"192.168.1.224");
	
    //datalen=strlen(databuf);

    /* Create a datagram socket on which to send. */
    sd = socket (AF_INET, SOCK_DGRAM, 0);
    if (sd < 0) {
        perror ("Opening datagram socket error");
		return -1;
    }
    else
        printf ("Opening the datagram socket...OK.\n");
	
    /* Initialize the group sockaddr structure with a */
    /* group address of 225.1.1.1 and port 5555. */
    memset ((char *) &groupSock, 0, sizeof (groupSock));
    groupSock.sin_family = AF_INET;
    groupSock.sin_addr.s_addr = inet_addr (mip);
    groupSock.sin_port = htons (mport);
	
    // Disable loopback so you do not receive your own datagrams.
    /*
    {
	char loopch = 0;
	if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0)
	{
	perror("Setting IP_MULTICAST_LOOP error");
	close(sd);
	exit(1);
	}
	else
	printf("Disabling the loopback...OK.\n");
    }
    */
	
    /* Set local interface for outbound multicast datagrams. */
    /* The IP address specified must be associated with a local, */
    /* multicast capable interface. */
    localInterface.s_addr = inet_addr (localip);
    if (setsockopt (sd, IPPROTO_IP, IP_MULTICAST_IF, (char *) &localInterface,
		sizeof (localInterface)) < 0) {
        perror ("Setting local interface error");
		close(sd);
		return -2;
    }
    else
        printf ("Setting the local interface...OK\n");
    /* Send a message to the multicast group specified by the*/
    /* groupSock sockaddr structure. */
    /*int datalen = 1024;*/
    if (sendto (sd, databuf, datalen, 0, (struct sockaddr *) &groupSock,
		sizeof (groupSock)) < 0) {
        perror ("Sending datagram message error");
    }
    else
        printf ("Sending datagram message...OK\n");
	
		/* Try the re-read from the socket if the loopback is not disable
		if(read(sd, databuf, datalen) < 0)
		{
		perror("Reading datagram message error\n");
		close(sd);
		exit(1);
		}
		else
		{
		printf("Reading datagram message from client...OK\n");
		printf("The message is: %s\n", databuf);
		}
    */
	close(sd);
    return 0;
}


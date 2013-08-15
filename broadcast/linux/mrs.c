#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ifname:eth0 eth1
// MAC_str : 001122334455      return val
// maclong : 00:11:22:33:44:55 reutrn val
void mac_eth(char *ifname,unsigned char *MAC_str,char *maclong)
{
#define HWADDR_len 6
    int s,i;
    struct ifreq ifr;
    s = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifr.ifr_name, ifname);
    ioctl(s, SIOCGIFHWADDR, &ifr);
    for (i=0; i<HWADDR_len; i++){
        sprintf(&MAC_str[i*2],"%02X",((unsigned char*)ifr.ifr_hwaddr.sa_data)[i]);
        sprintf(&maclong[i*3],"%02X:",((unsigned char*)ifr.ifr_hwaddr.sa_data)[i]);
    }
    maclong[17]='\0';
    MAC_str[12]='\0';
}


//int domains[] = { AF_INET, AF_INET6 };
//domain: AF_INET or AF_INET6
// pif: "eth0" or "eth2"
// pip: ip return
int print_addresses(const int domain,char *pif,char *pip)
{
    int s;
    struct ifconf ifconf;
    struct ifreq ifr[50];
    int ifs;
    int i;
    int ret=0;// interface not found
	
    s = socket(domain, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return 0;
    }
	
    ifconf.ifc_buf = (char *) ifr;
    ifconf.ifc_len = sizeof ifr;
	
    if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) {
        perror("ioctl");
        return 0;
    }
	
    ifs = ifconf.ifc_len / sizeof(ifr[0]);
    // interfaces number
    //printf("interfaces = %d:\n", ifs);
    for (i = 0; i < ifs; i++) {
        char ip[INET_ADDRSTRLEN];
        struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;
		
        if (!inet_ntop(domain, &s_in->sin_addr, ip, sizeof(ip))) {
            perror("inet_ntop");
            return 0;
        }
		
        //   printf("%s - %s\n", ifr[i].ifr_name, ip);
        if (strcmp(pif,ifr[i].ifr_name)==0) strcpy(pip,ip);
        ret=1;
    }
	
    close(s);
	
    return ret;
}


int ms(){
    struct in_addr localInterface;
    struct sockaddr_in groupSock;
    int sd;
    char databuf[1024] = "Multicast test message lol!";
    int datalen = sizeof (databuf);
    
    int mport=4321;
    char mip[30];
    char localip[30];
	
    strcpy(mip,"226.1.1.1");
    strcpy(localip,"192.168.1.224");
	
    datalen=strlen(databuf);
    /* Create a datagram socket on which to send. */
    sd = socket (AF_INET, SOCK_DGRAM, 0);
    if (sd < 0) {
        perror ("Opening datagram socket error");
        exit (1);
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
        exit (1);
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
    return 0;
}


// multicast rcv
// rip: local ip
// mip: multicast ip
// rport: multicast port
// databuf: rcv buf
// pnLen: multicast rcv len
int mr(char *rip,char *mip,int rport,char *databuf,int *pnLen)
{
	struct sockaddr_in localSock;
	struct ip_mreq group;
	int sd;
	int datalen;
	//char databuf[1024];
	//int rport=4321;
	//char rip[30];
	//char mip[30];
	
	int rlen=0;
	*pnLen=0;
	
	//strcpy(rip,"192.168.1.223");
	//strcpy(mip,"226.1.1.1");
	
	/* Create a datagram socket on which to receive. */
	sd = socket (AF_INET, SOCK_DGRAM, 0);
	if (sd < 0)
    {
		perror ("Opening datagram socket error");
		return -1;
    }
	else
		printf ("Opening datagram socket....OK.\n");
	
	/* Enable SO_REUSEADDR to allow multiple instances of this */
	/* application to receive copies of the multicast datagrams. */
	{
		int reuse = 1;
		if (setsockopt
			(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof (reuse)) < 0)
		{
			perror ("Setting SO_REUSEADDR error");
			close (sd);
			return -2;
		}
		else
			printf ("Setting SO_REUSEADDR...OK.\n");
	}
	
	/* Bind to the proper port number with the IP address */
	/* specified as INADDR_ANY. */
	memset ((char *) &localSock, 0, sizeof (localSock));
	localSock.sin_family = AF_INET;
	localSock.sin_port = htons (rport);
	localSock.sin_addr.s_addr = INADDR_ANY;   // rip ??????????????
	if (bind (sd, (struct sockaddr *) &localSock, sizeof (localSock)))
    {
		perror ("Binding datagram socket error");
		close (sd);
		return -3;
    }
	else
		printf ("Binding datagram socket...OK.\n");
	
	/* Join the multicast group 226.1.1.1 on the local 203.106.93.94 */
	/* interface. Note that this IP_ADD_MEMBERSHIP option must be */
	/* called for each local interface over which the multicast */
	/* datagrams are to be received. */
	group.imr_multiaddr.s_addr = inet_addr (mip);
	group.imr_interface.s_addr = inet_addr (rip);
	if (setsockopt
		(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &group,
		sizeof (group)) < 0)
    {
		perror ("Adding multicast group error");
		close (sd);
		return -4;
    }
	else
		printf ("Adding multicast group...OK.\n");
	
	/* Read from the socket. */
	datalen = 512;
	
	rlen=read (sd, databuf, datalen);
	if(rlen<0)
    {
		perror ("Reading datagram message error");
		close (sd);
		return -5;
    }
	else
    {
		databuf[rlen]=0;
		printf ("Reading datagram message...OK.");
		printf ("The message from multicast server is: \"%s\"\n", databuf);
    }
	*pnLen=rlen;
	return 0;
}




int main (int argc, char *argv[])
{
	char localip[30],mrip[30];
	int mrport=4321;
	char mrcvbuf[1024];
	int mrLen=0;

	strcpy(localip,"192.168.1.224");
	strcpy(mrip,"226.1.1.1");

	mr(localip,mrip,mrport,mrcvbuf,&mrLen);

	return 0;
}



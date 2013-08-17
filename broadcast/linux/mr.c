
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>


// get if ip : if="eth0" or "eth2"      
//int domains[] = { AF_INET, AF_INET6 };
//domain: AF_INET or AF_INET6
// pif: "eth0" or "eth2"
// pip: ip return
int getIP(const int domain,char *pif,char *pip)
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




struct sockaddr_in localSock;
struct ip_mreq group;
int sd;
int datalen;
char databuf[1024];

int
main (int argc, char *argv[])
{
	char mip[30],ifname[30],localip[30];
	int mport=4322;
	int ret;

	strcpy(mip,"226.1.1.1");
	strcpy(ifname,"eth0");
	printf("usage: mr 226.1.1.1 4321 eth0\n");
	switch(argc){
	case 4:
		strcpy(ifname,argv[3]);
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
	getIP(AF_INET,ifname,localip);
	printf(" multicast rcv test : %s:%d  %s:%s\n",mip,mport,ifname,localip);

	/* Create a datagram socket on which to receive. */
	sd = socket (AF_INET, SOCK_DGRAM, 0);
	if (sd < 0)
    {
		perror ("Opening datagram socket error");
		exit (1);
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
			exit (1);
		}
		else
			printf ("Setting SO_REUSEADDR...OK.\n");
	}
	
	/* Bind to the proper port number with the IP address */
	/* specified as INADDR_ANY. */
	memset ((char *) &localSock, 0, sizeof (localSock));
	localSock.sin_family = AF_INET;
	localSock.sin_port = htons (mport);
	localSock.sin_addr.s_addr = INADDR_ANY;
	if (bind (sd, (struct sockaddr *) &localSock, sizeof (localSock)))
    {
		perror ("Binding datagram socket error");
		close (sd);
		exit (1);
    }
	else
		printf ("Binding datagram socket...OK.\n");
	
	/* Join the multicast group 226.1.1.1 on the local 203.106.93.94 */
	/* interface. Note that this IP_ADD_MEMBERSHIP option must be */
	/* called for each local interface over which the multicast */
	/* datagrams are to be received. */
	group.imr_multiaddr.s_addr = inet_addr (mip);
	group.imr_interface.s_addr = inet_addr (localip);
	if (setsockopt
		(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &group,
		sizeof (group)) < 0)
    {
		perror ("Adding multicast group error");
		close (sd);
		exit (1);
    }
	else
		printf ("Adding multicast group...OK.\n");
	
	/* Read from the socket. */
	datalen = 1000;
	ret = read(sd, databuf, datalen);
	if( ret < 0)
    {
		perror ("Reading datagram message error");
		close (sd);
		exit (1);
    }
	else
    {
		buf[ret]=0;
		printf ("Reading datagram message...OK.\n");
		printf ("The message from multicast server is: \"%s\"\n", databuf);
    }
	return 0;
}


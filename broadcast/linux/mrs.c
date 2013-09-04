#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/reboot.h>


pthread_t tidr,tids;
int threadr_ret,threads_ret;
char para_newIP[30];
char para_netmask[30];
char para_hostname[200];

int nBootNum=100;

char para_ifname[30]; // if name : eth2 eth0 ......
char para_rip[30];   // rcv network interface ip, local if ip addr
char para_mrip[30];   // rcv multicast ip
int para_rport; //      rcv multicast port
char para_rbuf[3000];  // rcv multicast buffer
int para_rlen;         // rcv buf len 2000

char para_msip[30];   // send multicast ip
int para_sport; //      send multicast port
// multicast rcv
// rip: local ip
// mip: multicast ip
// rport: multicast port
// databuf: rcv buf
// pnLen: multicast rcv len
//int mr(char *rip,char *mip,int rport,char *databuf,int *pnLen)






// get if MAC 
// ifname:eth0 eth1
// MAC_str : 001122334455      return val
// maclong : 00:11:22:33:44:55 reutrn val
void getMAC(char *ifname,unsigned char *MAC_str,char *maclong)
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


int ms(char *localip,char *mip,int mport,char *databuf,int nLen)
{
	struct in_addr localInterface;
	struct sockaddr_in groupSock;
	int sd;
	//char databuf[1024] = "Multicast test message lol!";
	int datalen;

	datalen=nLen;
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
			close(sd);
			return -3;
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
// multicast reply  (getip 
// srcip : client ip, using for check , client ip
// msip: reply multicast ip : 226.1.1.2
// msport:                    4322
// ifname: eth2 ?
// sn:  time str, 
int ms_ser2net(char *srcip,char *msip,int msport,char *ifname,char *sn)
{
	char localip[30];
	char hostname[40];
	int ret;

	char msbuf[1024];
	int sLen;

	int n=0;
	char header[30];
	char pip[30];
	int replyPort=0;

	char ifip[30];
	char ifmac[40];
	char tmp[100];

	getMAC(ifname,tmp,ifmac);
	getIP(AF_INET,ifname,ifip);
	gethostname(hostname,30);

	// 1: rgetip
	// 2: src ip request  client ip
	// 3. sn : string time 
	// 4: multicast ip
	// 5: multicast port
	// 6. ser2net if name: "eth2"
	// 7. ser2net if ip:
	// 8. ser2net if MAC: 
	// 9. hostname
	sprintf(msbuf,"rgetip %s %s %s %d %s %s %s %s",srcip,sn,msip,msport,ifname,ifip,ifmac,hostname); 
	sLen=strlen(msbuf);
	printf("ms_ser2net : %s\n",msbuf);

	ms(ifip,msip,msport,msbuf,sLen);

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
void changeIP(char *ifname,char *newIP,char *newMask,char *hostname)
{
	FILE *fp;
	fp=fopen("/root/myinit","wt");
	if(fp==NULL)return;
	fprintf(fp,"#!/bin/sh\n");
	fprintf(fp,"hostname %s\n",hostname);
	//fprintf(fp,"ifconfig %s %s netmask %s\n",ifname,newIP,newMask);
	fprintf(fp,"ifconfig %s %s\n",ifname,newIP);
	fprintf(fp,"ser2net&\n");
	//fprintf(fp,"/root/mrs eth2 100 &\n");
	fflush(fp);
	fclose(fp);
	if(0!=strcmp(ifname,"eth0")){
		reboot(RB_AUTOBOOT);
	}
	return ;
}
// boot broadcast  10min
void* thread_send(void *arg)
{
	int i;
	int s=6;

	printf("\n send thread start return:200\n");
	for(i=0;i<nBootNum;i++){
		//ms_ser2net(para_rip,para_msip,para_sport,para_ifname,sztime);
		ms_ser2net(para_rip,para_msip,para_sport,para_ifname,"00:00");
		sleep(s);
	}
	threads_ret  = 200;
	pthread_exit(&threads_ret);

	return NULL;
}
void* thread_rcv (void *arg)
{
	int ret;

	char sztime[20];
	char clientip[20];
	char msbuf[1024];
	int sLen;

	int n=0;
	char header[30];
	char pip[30];
	int replyPort=0;

	strcpy(para_mrip,"226.1.1.1");
	para_rport=4321;
	para_rlen=2000;

	strcpy(para_msip,"226.1.1.2");
	para_sport=4322;

//char para_newIP[30];
//char para_netmask[30];
//char para_hostname[200];
	strcpy(para_newIP,"192.168.2.88");
	strcpy(para_netmask,"255.255.255.0");
	strcpy(para_hostname,"netport");


	// 1: rgetip
	// 2: src ip request
	// 3: multicast ip
	// 4. multicast port
	// 5. ser2net if name: "eth2"
	// 6. ser2net if ip:
	// 7. ser2net if MAC: 

	// fixme: not localip, use client ip 
	sprintf(msbuf,"rgetip %s %s %d test",para_rip,para_msip,para_sport); 
	sLen=strlen(msbuf);
	printf(" mcast rcv : %s:%d\n",para_mrip,para_rport);
	printf(" mcast send : %s:%d    if:%s(%s)\n",para_msip,para_sport,para_ifname,para_rip);

	for(;;){
		ret = mr(para_rip,para_mrip,para_rport,para_rbuf,&para_rlen);
		if( ret>=0){
			n=sscanf(para_rbuf,"%s",header);
			if(n!=1)continue;
			if(0==strcmp(header,"getip")){   // stricmp ??????????
				n=sscanf(para_rbuf,"%s%s%d%s%s",header,pip,&replyPort,clientip,sztime);
				if(n==5){
					printf(" header: %s\n",header);
					ms_ser2net(clientip,pip,replyPort,para_ifname,sztime);
				}
			}
			else if(0==strcmp(header,"changeip")){   // stricmp ??????????
				n=sscanf(para_rbuf,"%s%s%s",header,para_newIP,para_netmask);
				if(n==3){
					printf(" header: %s\n",header);
					changeIP(para_ifname,para_newIP,para_netmask,para_hostname);
					break;
				}
			}
		}
	}
	threadr_ret  = 100;
	pthread_exit(&threadr_ret);

	return NULL;
}

//char para_rip[30];   // rcv network interface ip, local if ip addr
//char para_mip[30];   // rcv multicast ip
//int para_rport; //      rcv multicast port
//char para_rbuf[3000];  // rcv multicast buffer
//int para_rlen;         // rcv buf len 2000

int main (int argc, char *argv[])
{
    int err;
    int *pr,*ps;

	printf(" !! usage mrs eth0\n");
	printf(" !! usage mrs eth0 10           (boot mcast num)\n");
	printf(" multicast rcv : 226.1.1.1:4321(default) , rcv str format(5 str) : getip replyIP replyPort clientIP szTime\n");
	if(argc<2) return -1;
	if(argc>2) nBootNum=atoi(argv[2]);

	// get local network interface ip addr
	strcpy(para_ifname,argv[1]);
	getIP(AF_INET,para_ifname,para_rip);// local ip
	gethostname(para_hostname,30);  // get hostname
	printf("current hostname: %s\n",para_hostname);

	err = pthread_create(&tidr, NULL, &thread_rcv, NULL);
	if (err != 0){
		printf("\ncan't create thread multicast rcv:[%s]", strerror(err));
		return -1;
	}
	else {
		printf("\n Thread multicast rcv created successfully\n");
	}

	err = pthread_create(&tids, NULL, &thread_send, NULL);
	if (err != 0){
		printf("\ncan't create thread multicast send: [%s]", strerror(err));
		return -2;
	}
	else {
		printf("\n Thread multicast send created successfully\n");
	}

    pthread_join(tids, (void**)&ps);
    printf("\n return value from send thread is [%d]\n", *ps);

    pthread_join(tidr, (void**)&pr);
    printf("\n return value from rcv thread is [%d]\n", *pr);

    return 0;
}


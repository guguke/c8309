#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/reboot.h>


struct port_stat {
	int stat;
	int baud;
	int bits;
	int parity;// p: 0 :no parity  1:odd   2: even,  3:4:?????
	int stop;
	int flowctrl;
	int ip;// host 
} gport_stat[8];

struct in_addr gip,gipmask;
char gszip[20],gszipmask[20];

int baud2n(int baud)
{
	int ret;

	switch(baud){
	case 300: ret=0x0;break;
	case 600: ret=0x01;break;
	case 1200: ret=0x02;break;
	case 2400: ret=0x03;break;
	case 4800: ret=0x04;break;
	case 9600: ret=0x05;break;
	case 19200: ret=0x06;break;
	case 38400: ret=0x07;break;
	case 115200: ret=0x08;break;
	default:
		ret=0x08;
		break;
	}
	return ret;
}
int bits2n(int bits)
{
	int ret;
	switch(bits){
	case 5: ret=0x0;break;
	case 6: ret=0x01;break;
	case 7: ret=0x02;break;
	case 8: ret=0x03;break;
	default: ret=0x03;break;
	}
	return ret;
}
// p: 0 :no parity  1:odd   2: even,  3:4:?????

int getipmask(char *iface,char *pip,char *pmask)
{
	int fd;
	struct ifreq ifr;
	//char iface[] = "eth0";
	fd = socket(AF_INET, SOCK_DGRAM, 0);

	//Type of address to retrieve - IPv4 IP address
	ifr.ifr_addr.sa_family = AF_INET;

	//Copy the interface name in the ifreq structure
	strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);
	//get the ip address
	ioctl(fd, SIOCGIFADDR, &ifr);
	//display ip
	printf("IP address of %s - %s\n" , iface , inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr) );
	strcpy(pip,inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr) );
	//get the netmask ip
	ioctl(fd, SIOCGIFNETMASK, &ifr);
	//display netmask
	printf("Netmask of %s - %s\n" , iface , inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr) );
	strcpy(pmask,inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr) );
	close(fd);
	return 0;
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

void getip_eth0()
{
	struct in_addr nip;// nip.s_addr
	char pip[40],pmask[40];

	getipmask("eth0",pip,pmask);

	//getIP(AF_INET,"eth0",pip);// local ip
	strcpy(gszip,pip);
	strcpy(gszipmask,pmask);

	inet_aton(pip,&gip);
	inet_aton(pmask,&gipmask);
	printf(" mydebug get eth0 ip addr : %s netmask: %s \n",pip,pmask);

	return;
}

// init global data
void init_gd()
{
	int i;
	struct in_addr nip;// nip.s_addr
	char pip[40];

	getip_eth0();

	for(i=0;i<8;i++){
		gport_stat[i].stat=0;
		gport_stat[i].baud=baud2n(115200);
		gport_stat[i].bits=bits2n(8);
		gport_stat[i].parity=0;
		gport_stat[i].stop=1;
		gport_stat[i].flowctrl=0;

		gport_stat[i].ip=0;
		///////////////////////////// for test
		//sprintf(str,"192.168.1.%d",i+1);
		//inet_aton(str,&nip);
		//gport_stat[i].ip=nip.s_addr;
		//printf(" set ip(%d) : %s \n",i,inet_ntoa(nip));
	}

	return;
}
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
int mySaveBAT(char *pip,char *pmask)
{
	FILE *fp;

	fp=fopen("/root/app/batip0","wt");
	if(fp==NULL)return;
	fprintf(fp,"#!/bin/sh\n");
	//fprintf(fp,"hostname %s\n",hostname);
	////// fprintf(fp,"ifconfig %s %s netmask %s\n",ifname,newIP,newMask);
	//fprintf(fp,"ifconfig eth2 down\n");////////////////////////////////////
	//fprintf(fp,"ifconfig eth1 down\n");///////////////////////////////
	fprintf(fp,"ifconfig eth0 %s netmask %s\n",pip,pmask);
	//fprintf(fp,"/root/run/ser2net -c /root/run/ser2net.conf &\n");
	////// fprintf(fp,"/root/mrs eth2 100 &\n");
	//fprintf(fp,"/root/run/mrs eth0 100 &\n");
	fflush(fp);
	fclose(fp);
	sync();

	/// new tmp bat file 
	fp=fopen("/root/run/batnewip","wt");
	if(fp==NULL)return;
	fprintf(fp,"#!/bin/sh\n");
	//fprintf(fp,"hostname %s\n",hostname);
	////// fprintf(fp,"ifconfig %s %s netmask %s\n",ifname,newIP,newMask);
	fprintf(fp,"killall mrs\n");////////////////////////////////////
	fprintf(fp,"killall ser2net\n");///////////////////////////////
	fprintf(fp,"ifconfig eth0 %s netmask %s\n",pip,pmask);
	fprintf(fp,"/root/run/ser2net -c /root/run/ser2net.conf &\n");
	////// fprintf(fp,"/root/mrs eth2 100 &\n");
	fprintf(fp,"/root/run/mrs eth0 100 &\n");
	//fprintf(fp,"/root/run/mrs2s &\n");
	fflush(fp);
	fclose(fp);
	sync();

	system("/root/run/batnewip");

	return 0;
}

int setnewip(char *p,int len)
{
	char ip[40],ipmask[40];
	int ipnl,masknl;
	struct in_addr sip,smask;

	if( (0x0ff & p[0]) != 0x51 ) return -1;
	bcopy(p+4,&ipnl,4);
	bcopy(p+8,&masknl,4);
	sip.s_addr=ipnl;// ntohl
	smask.s_addr=masknl;// ntohl

	printf("  s_addr  ip:0x %08x  mask:0x %08x \n",sip.s_addr,smask.s_addr);
	printf("  s_addr  old ip:0x %08x  old mask:0x %08x \n",gip.s_addr,gipmask.s_addr);
	printf("     newip : %s    ",	inet_ntoa(sip) );
	printf("     newmask : %s \n",	inet_ntoa(smask));
	strcpy(ip,inet_ntoa(sip));
	strcpy(ipmask,inet_ntoa(smask));
	printf("    old ip : %s    ",inet_ntoa(gip) );
	printf("    newmask : %s \n",inet_ntoa(gipmask));

	if( sip.s_addr != gip.s_addr || smask.s_addr != gipmask.s_addr ){
		printf("    !!!! new ip, ifconfig \n");
		mySaveBAT(ip,ipmask);              // 8309  cyx
		////////////// do ifconfig
		gip.s_addr=sip.s_addr;
		gipmask.s_addr = smask.s_addr;
		strcpy(gszip,ip);
		strcpy(gszipmask,ipmask);
	}
	else printf(" do nothing, setup new ip \n");
	return 0;
}
int udpa0(char *pin,int inlen,char *pout,int *poutlen)
{
	int op=-1;
	int nl,hl;
	int sn;// serial port number
	int i;
	char *p;
	int *pi;

	printf("   udpa0  recv in len(%d)  --0x-- : ",inlen);
	for(i=0;i<inlen;i++) printf("%02x ",0x0ff & pin[i]);
	printf("\n");

	switch(0x0ff & pin[1]){
	case 0:
		bcopy(pin+3,&nl,4);
		gip.s_addr=ntohl(nl);
		bcopy(pin+7,&nl,4);
		gipmask.s_addr = ntohl(nl);

		*poutlen=0;
		op=-1;
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
		sn=0x07 & pin[1];
		//pout[0]=0xa0;
		//pout[1]=pin[1];
		//pout[2]=0x0a;
		gport_stat[sn].stat=0x0ff & pin[3];
		gport_stat[sn].baud=0x0ff & pin[4];
		gport_stat[sn].bits=0x0ff & pin[5];
		gport_stat[sn].parity=0x0ff & pin[6];
		gport_stat[sn].stop=0x0ff & pin[7];
		gport_stat[sn].flowctrl=0x0ff & pin[8];

		bcopy(pin+9,&nl,4);
		gport_stat[sn].ip=ntohl(nl);
		printf("         udpa0  sn:%d   nl:0x%08x    hl:0x%08x  ============\n",sn,nl,gport_stat[sn].ip);

		*poutlen=0;
		op=-1;
		break;
	default:
		op=-1;
		*poutlen=0;
		break;
	}
	return op;
}
int udpaf(char *pin,int inlen,char *pout,int *poutlen)
{
	int op=-1;
	int nl,hl;
	int sn;// serial port number
	int i;
	char *p;
	int *pi;
	int fnew=0;
	int s;

	printf("   udpaf  recv in len(%d)  --0x-- : ",inlen);
	for(i=0;i<inlen;i++) printf("%02x ",0x0ff & pin[i]);
	printf("\n");

	for(i=0;i<8;i++){
		s= pin[3+i]  & 0x0ff;
		if( s != gport_stat[i].stat ){
			fnew=1;
			gport_stat[i].stat = s;
		}
	}
	return op;
}
int udp2send(char *pin,int inlen,char *pout,int *poutlen)
{
	int op=0;  // 0: sendback
	int check;
	int i;

	printf("   udp2send  recv in len(%d)  --0x-- : ",inlen);
	for(i=0;i<inlen;i++) printf("%02x ",0x0ff & pin[i]);
	printf("\n");

	if(inlen<=0){
		*poutlen=0;
		op=-1;
		return op;
	}
	check =( 0x0ff & pin[2] ) + 4 -1;
	check = 0x0ff & pin[check];
	if( check != crc(pin) ){
		printf(" crc error ,rcv udp data  \n");
		*poutlen=0;
		return -1;
	}

	switch(0x0ff & pin[0]){
	case 0x0a0:
		op=udpa0(pin,inlen,pout,poutlen);
		break;
	case 0x0af:
		op=udpaf(pin,inlen,pout,poutlen);
		break;
	default:
		*poutlen=0;
		op=-1;
		break;
	}
	pout[0]=0xaf;
	pout[1]=0x20;
	pout[2]=8;
	for(i=0;i<8;i++){
		pout[3+i]=gport_stat[i].stat;
	}
	pout[11]=crc(pout);
	*poutlen=12;
	op=0;
	return op;
}
int tcp51(char *pin,int inlen,char *pout,int *poutlen)
{
	int op=-1;
	getip_eth0();
	pout[0]=0xa1;
	pout[1]=0x00;
	pout[2]=2;
	pout[3]=0;
	pout[4]=0;
	pout[5]=crc(pout);
	*poutlen=6;
	op=0x51;//// set new ip
	return op;
}
int tcp50(char *pin,int inlen,char *pout,int *poutlen)
{
	int op=-1;
	int nl;
	int sn;// serial port number
	int i;
	char *p;
	int *pi;

	switch(0x0ff & pin[1]){
	case 0:
		pout[0]=0xa0;
		pout[1]=0x00;
		pout[2]=8;
		getip_eth0();
		printf(" mydebug ser2net ip : %s    netmask : %s   \n  ",gszip,gszipmask);
		nl=gip.s_addr;//    htonl
		bcopy(&nl,pout+3,4);
		nl=gipmask.s_addr;// htonl
		bcopy(&nl,pout+7,4);
		pout[11]=crc(pout);

		*poutlen=12;
		op=0;
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
		sn=0x07 & pin[1];
		pout[0]=0xa0;
		pout[1]=pin[1];
		pout[2]=0x0a;
		pout[3]=gport_stat[sn].stat;
		pout[4]=gport_stat[sn].baud;
		pout[5]=gport_stat[sn].bits;
		pout[6]=gport_stat[sn].parity;
		pout[7]=gport_stat[sn].stop;
		pout[8]=gport_stat[sn].flowctrl;

		nl=htonl(gport_stat[sn].ip);
		bcopy(&nl,pout+9,4);

		pout[13]=crc(pout);
		*poutlen=14;
		op=0;
		break;
	case 0x20:
		pout[0]=0xa0;
		pout[1]=pin[1];
		pout[2]=0x08;
		for(i=0;i<8;i++) pout[i+3]=gport_stat[i].stat;
		pout[11]=crc(pout);
		*poutlen=12;
		op=0;
		break;
	case 0x0ff:
		p=pout;
		pi=poutlen;
		pin[1]=0x0;
		tcp50(pin,inlen,p,pi);
		for(i=0;i<8;i++){
			pin[1]=0x10+i;
			p+=*pi;
			pi++;
			tcp50(pin,inlen,p,pi);
		}
		op=9;
		break;
	default:
		break;
	}
	return op;
}

int tcp2send(char *pin,int inlen,char *pout,int *poutlen)
{
	int op=0;  // 0: sendback
	int check;

	if(inlen<=0){
		*poutlen=0;
		op=-1;
		return op;
	}
	check =( 0x0ff & pin[2] ) + 4 -1;
	check = 0x0ff & pin[check];
	if( check != crc(pin) ){
		printf(" crc error ,rcv data , tcp client \n");
		*poutlen=0;
		return -1;
	}

	switch(0x0ff & pin[0]){
	case 0x50:
		op=tcp50(pin,inlen,pout,poutlen);
		break;
	case 0x51:
		op=tcp51(pin,inlen,pout,poutlen);
		break;
	default:
		op=-1;
		*poutlen=0;
		break;
	}


	return op;
}






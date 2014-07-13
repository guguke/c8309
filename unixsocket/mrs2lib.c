#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

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


// init global data
void init_gd()
{
	int i;
	struct in_addr nip;// nip.s_addr
	char str[40];

	inet_aton("192.168.1.1",&gip);
	inet_aton("255.255.255.0",&gipmask);

	for(i=0;i<8;i++){
		gport_stat[i].stat=0;
		gport_stat[i].baud=baud2n(115200);
		gport_stat[i].bits=bits2n(8);
		gport_stat[i].parity=0;
		gport_stat[i].stop=1;
		gport_stat[i].flowctrl=0;

		gport_stat[i].ip=0;
		///////////////////////////// for test
		sprintf(str,"192.168.1.%d",i+1);
		inet_aton(str,&nip);
		gport_stat[i].ip=nip.s_addr;
		printf(" set ip(%d) : %s \n",i,inet_ntoa(nip));
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

int setnewip(char *p,int len)
{
	char ip[40],ipmask[40];
	int ipnl,masknl;
	struct in_addr sip,smask;

	if( (0x0ff & p[0]) != 0x51 ) return -1;
	bcopy(p+4,&ipnl,4);
	bcopy(p+8,&masknl,4);
	sip.s_addr=ntohl(ipnl);
	smask.s_addr=ntohl(masknl);

	printf("    ip(hl):0x %08x  mask(hl):0x %08x \n",sip.s_addr,smask.s_addr);
	printf("    old ip(hl):0x %08x  old mask(hl):0x %08x \n",gip.s_addr,gipmask.s_addr);
	printf("     newip : %s    ",	inet_ntoa(sip) );
	printf("     newmask : %s \n",	inet_ntoa(smask));
	printf("    old ip : %s    ",inet_ntoa(gip) );
	printf("    newmask : %s \n",inet_ntoa(gipmask));

	if( sip.s_addr != gip.s_addr || smask.s_addr != gipmask.s_addr ){
		printf("    !!!! new ip, ifconfig \n");
		////////////// do ifconfig
		gip.s_addr=sip.s_addr;
		gipmask.s_addr = smask.s_addr;
	}
	else printf(" do nothing, setup new ip \n");
	return 0;
}

int udp2send(char *pin,int inlen,char *pout,int *poutlen)
{
	int op=0;  // 0: sendback

	memcpy(pout,pin,inlen);
	*poutlen=inlen;

	return 0;
}
int tcp51(char *pin,int inlen,char *pout,int *poutlen)
{
	int op=-1;
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
		nl=htonl(gip.s_addr);
		bcopy(&nl,pout+3,4);
		nl=htonl(gipmask.s_addr);
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
		memcpy(pout,pin,inlen);
		*poutlen=inlen;
		op=0;
		break;
	}


	return op;
}






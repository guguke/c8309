#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <pthread.h>

int wvgpio(int addr,int v)
{
	FILE *fp;
	fp = fopen("/proc/gpio8308","rt");
	fprintf(fp,"a%08x %0x8",addr,v);
	fclose(fp);
	return 0;
}
int wagpio(int addr)
{
	FILE *fp;
	fp = fopen("/proc/gpio8308","rt");
	fprintf(fp,"a%08x",addr);
	fclose(fp);
	return 0;
}

int readgpio()
{
	FILE *fp;
	int v;
	fp = fopen("/proc/gpio8308","rt");
	fscanf(fp,"%x",&v);
	fclose(fp);
	return v;
}

int main(int argc,char *argv[])
{
	int v;
	int n=0;
	int k1=0x800,k2=0x800,k3=0x800;

	wagpio(0x0118);
	v = readgpio();
	wvgpio(0x0118,v|0xfc000000);
	usleep(100000);

	wagpio(0x0c08);
	usleep(100000);

	v = readgpio();
	printf(" 1 read addr(0x0c08): %08x\n",v);
	usleep(100000);
	for(;;){
		v = readgpio();
		if((0x800&v)!=k1){
			printf(" read addr(0x0c08) : %08x\n",v);
			k1 = v & 0x800;
			n++;
			if(n>5)break;
		}
		usleep(100000);
	}

	//usleep(1000000);// 1s

	return 0;
}




















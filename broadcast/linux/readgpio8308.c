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

int main(int argc,char *argv[])
{
	int v;
	int n=0;
	int k1=0x800,k2=0x800,k3=0x800;
	FILE *fp;
	fp=fopen("/proc/gpio8308","w+t");
	if(fp==NULL) return -1;
	fprintf(fp,"a0118");
	fscanf(fp,"%x",&v);
	fprintf(fp,"a0118 %08x",v|0xfc000000);
	for(;;){
		fscanf(fp"%x"&v);
		if((0x800&v)!=k1){
			printf(" read addr(0x0118) : %08x\n",v);
			k1 = v & 0x800;
			n++;
			if(n>5)break;
		}
		usleep(100000);
	}

	//usleep(1000000);// 1s

	fclose(fp);
	return 0;
}




















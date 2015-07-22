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
	FILE *fp;
	fp=fopen("/proc/gpio8308","w+t");
	if(fp==NULL) return -1;
	fprintf(fp,"a0118");
	fscanf(fp,"%x",&v);
	printf(" read addr(0x0118) : 0x%08x\n",v);

	fclose(fp);
	return 0;
}




















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
	fp = fopen("/proc/gpio8308","wt");
	fprintf(fp,"%08x %08x",addr,v);
	fclose(fp);
	return 0;
}
int wagpio(int addr)
{
	FILE *fp;
	fp = fopen("/proc/gpio8308","wt");
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
void loaddefault()
{
	system("/usr/bin/killall ser2net");
	system("/usr/bin/killall mrs");
	system("cp -f /root/app/batip0d /root/app/batip0");
	system("umount /root/app");
	reboot(RB_AUTOBOOT);
}
int main(int argc,char *argv[])
{
	int v;
	int n=0;
	int k1=0x800,k2=0x800,k3=0x800;
	int us=300000; // 300 ms
	char szVer[]="1.01";
	FILE *fp;

	fp=fopen("/root/ver.readgpio8308.txt","wt");
	if(fp!=NULL){
		fprintf(fp,"ver %s readgpio8308\n",szVer);
		fclose(fp);
	}
	printf("readgpio8308 ver %s\n",szVer);

	// init
	wagpio(0x0118);
	v = readgpio();
	wvgpio(0x0118,v|0xfc000000);
	usleep(us);

	wagpio(0x0c08);
	usleep(us);
	for(;;){
		v = readgpio();
		if( (v&0x800) > 0 ){
			n = 0;
			usleep(us);
			//printf(" no key\n");
			continue;
		}
		n++;
		//printf(" key !!\n");
		if(n<2){
			usleep(us);
			continue;
		}
		// load default
		printf(" load default\n");
		loaddefault();
		break;
	}

	return 0;
}




















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
void loaddefault()
{
	system("cp -f /root/app/batip0d /root/app/batip0");
	system("umount /root/app");
	reboot(RB_AUTOBOOT);
}
int main(int argc,char *argv[])
{
	int v;
	int n=0;
	int k1=0x800,k2=0x800,k3=0x800;

	// init
	wagpio(0x0118);
	v = readgpio();
	wvgpio(0x0118,v|0xfc000000);
	//usleep(100000);

	wagpio(0x0c08);
	//usleep(100000);
	for(;;){
		v = readgpio();
		if( (v&0x800) > 0 ){
			n = 0;
			usleep(100000);
			continue;
		}
		n++;
		if(n<2){
			usleep(100000);
			continue;
		}
		// load default
		loaddefault();
		break;
	}

	return 0;
}




















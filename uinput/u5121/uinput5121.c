#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int fduinput;
typedef void (*sighandler_t)(int);
//static int gi=0;
int gpio0=0,gpio1=0,gpio2=0;
int key6[]={KEY_LEFT,KEY_RIGHT,KEY_UP,KEY_DOWN,KEY_ENTER,KEY_ESC};
int keyv[]={KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,
	        KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5,KEY_F6,KEY_F7,KEY_F8,KEY_F9,
        	KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,
			KEY_ENTER,KEY_BACKSPACE,
			KEY_A,KEY_G,KEY_M,KEY_S,KEY_Y,
			KEY_B,KEY_H,KEY_N,KEY_T,KEY_Z,
			KEY_C,KEY_I,KEY_O,KEY_U,
			KEY_D,KEY_J,KEY_P,KEY_V,
			KEY_E,KEY_K,KEY_Q,KEY_W,
			KEY_F,KEY_L,KEY_R,KEY_X
};
//int key6[]={30,31,32,33,34,35};
/*
 press 32      up
 press 30        left
 press 31         right
 press 33         donw
 press 34           enter
 press 35           esc
*/

int udp_broadcast_recv(char *paddr,int nport,char *buf,int bufsize,int nn)
{
	struct sockaddr_in myaddr;	/* our address */
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	int fd;				/* our socket */
	//unsigned char buf[BUFSIZE];	/* receive buffer */
	int n;
	int so_broadcast=1;
        int i;
        char ip[100];

		int key,press;
		int nscanf;

	/* create a UDP socket */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return -1;
	}

	/* bind the socket to any valid IP address and a specific port */
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	//myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(inet_aton(paddr,&myaddr.sin_addr)==0){
		printf("  inet_aton() failed\n");
		close(fd);
		return -2;
	}
	myaddr.sin_port = htons(nport);
	//myaddr.sin_port = htons(SERVICE_PORT);

	//if(-1 == setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&so_broadcast,sizeof so_broadcast)) fprintf(stderr," setsockopt BROADCAST error\n");
	// use SO_REUSEADDR not SO_REUSEPORT
	if(-1 == setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&so_broadcast,sizeof so_broadcast)) fprintf(stderr," setsockopt REUSEADDR error\n");

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		printf(" bind failed");
		return -3;
	}

	printf("waiting on port %d\n", nport);
	for(i=0;;){
		recvlen = recvfrom(fd, buf, bufsize, 0, (struct sockaddr *)&remaddr, &addrlen);
		printf("received %d bytes\n", recvlen);
		if (recvlen > 0) {
			ip[0]=0;
			inet_ntop(AF_INET,&remaddr.sin_addr,ip,90);
			printf(" recv from : ip:%s  port:%d \n",ip,ntohs(remaddr.sin_port));

			//buf[recvlen] = 0;
			printf("received message: ");
			for(n=0;n<recvlen;n++) printf(" %02x",0x0ff & buf[n]);
			printf("\n");
			buf[recvlen]=0;
			nscanf=sscanf(buf,"%d%d",&key,&press);
			if(nscanf!=2)continue;
			keyevent(key,press);//key6[n],0);  // key release
			//i++;
		}
	}
	close(fd);
	return recvlen;
}

int init_gpiokey(void)
{
    //int                    fd;
    struct uinput_user_dev uidev;
    //struct input_event     ev;
    //int                    dx, dy;
    int                    i;

    fduinput = open("/dev/input/uinput", O_WRONLY | O_NONBLOCK);
    if(fduinput < 0) return -1;
    sleep(1);

    if(ioctl(fduinput, UI_SET_EVBIT, EV_KEY) < 0) return -2;
    if(ioctl(fduinput, UI_SET_KEYBIT, BTN_LEFT) < 0) return -3;
    if(ioctl(fduinput, UI_SET_KEYBIT, KEY_L) < 0) return -4;
    if(ioctl(fduinput, UI_SET_KEYBIT, KEY_S) < 0) return -4;
	for(i=0;i<(25+26);i++) ioctl(fduinput, UI_SET_KEYBIT, keyv[i]);
    //if(ioctl(fduinput, UI_SET_KEYBIT, KEY_UP) < 0) return -4;
    //if(ioctl(fduinput, UI_SET_KEYBIT, KEY_LEFT) < 0) return -4;
    //if(ioctl(fduinput, UI_SET_KEYBIT, KEY_RIGHT) < 0) return -4;
    //if(ioctl(fduinput, UI_SET_KEYBIT, KEY_DOWN) < 0) return -5;
    //if(ioctl(fduinput, UI_SET_KEYBIT, KEY_ESC) < 0) return -4;
    //if(ioctl(fduinput, UI_SET_KEYBIT, KEY_ENTER) < 0) return -6;
    if(ioctl(fduinput, UI_SET_EVBIT, EV_SYN) < 0) return -7;

	//ret = ioctl(fd, UI_SET_KEYBIT, KEY_D);

    //if(ioctl(fd, UI_SET_EVBIT, EV_REL) < 0)         die("error: ioctl");
    //if(ioctl(fd, UI_SET_RELBIT, REL_X) < 0)         die("error: ioctl");
    //if(ioctl(fd, UI_SET_RELBIT, REL_Y) < 0)         die("error: ioctl");

    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "uinput-i2c");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1;
    uidev.id.product = 0x1;
    uidev.id.version = 1;

    if(write(fduinput, &uidev, sizeof(uidev)) < 0) return -8;

    sleep(1);
    if(ioctl(fduinput, UI_DEV_CREATE) < 0) return -9;


    //if(ioctl(fd, UI_DEV_DESTROY) < 0)         die("error: ioctl");

    //close(fd);

    return 0;
}



// press 1:press 0:release
void keyevent(int key,int press)
{
    struct input_event     ev;

	//printf(" key press ===\n");
	if(press){
		memset(&ev, 0, sizeof(struct input_event));
		ev.type = EV_SYN;
		ev.code = 0;
		ev.value = 0;
		write(fduinput, &ev, sizeof(struct input_event));

		memset(&ev, 0, sizeof(struct input_event));
		ev.type = EV_KEY;
		ev.code = key;
		ev.value = 1;
		write(fduinput, &ev, sizeof(struct input_event));
	}
	else{
		memset(&ev, 0, sizeof(struct input_event));
		ev.type = EV_SYN;
		ev.code = 0;
		ev.value = 0;
		write(fduinput, &ev, sizeof(struct input_event));

		memset(&ev, 0, sizeof(struct input_event));
		ev.type = EV_KEY;
		ev.code = key;
		ev.value = 0;
		write(fduinput, &ev, sizeof(struct input_event));
	}

	return;
}
void key1(int n)
{
	int i0,i1,i2;
	i0=(gpio0>>(31-n))&1;
	i1=(gpio1>>(31-n))&1;
	i2=(gpio2>>(31-n))&1;
	if( (i0==1) && (i1==0) && (i2==0) ){
		keyevent(key6[n],1);// key press
	}
	else if( (i0==0) && (i1==1) && (i2==1) ){
		keyevent(key6[n],0);  // key release
	}

	return;
}
void keyinput(void)
{
	int i;
	for(i=0;i<6;i++){
		key1(i);
	}

	return;
}

void foo(int theint)
{
    struct timeval tv;
    struct timezone tz;
    FILE *fp;
    int gpio;
    gettimeofday(&tv,&tz);
    fp=fopen("/proc/gpio6","rt");
    fscanf(fp,"%x",&gpio);
    fclose(fp);
	gpio0=gpio1;
	gpio1=gpio2;
	gpio2=gpio;

	keyinput();
    //printf("0x%08x alarm :%d  time: %d %d \n",gpio,gi,(int)tv.tv_sec,(int)tv.tv_usec);
    //gi++;

	return;
}

int main(int argc,char *argv[])
{
	char addr[100];
	int nport;
	char buf[2000];
	int bufsize=1000;
        int nn;

	strcpy(addr,"127.255.255.255");
	nport=9000;
	nn=100;

	printf(" uinput5121 v1.00 \n");
	printf(" usage: uinput5121 ip_addr port\n");
	printf("        uinput5121 \n");
	printf("        uinput5121 127.255.255.255 \n");
	printf("        uinput5121 127.255.255.255 9000 \n");

	switch(argc){
	case 3:
		nport=atoi(argv[2]);
	case 2:
		strcpy(addr,argv[1]);
	default:
		break;
	}
	printf(" udp broadcast recv :   ip:%s  port:%d   \n",addr,nport);

	//udp_broadcast_recv(addr,nport,buf,bufsize,nn);

	if(init_gpiokey()<0) {
		printf(" open uinput dev error\n");
		return -1;
	}
	// loop wait udp , no return
	udp_broadcast_recv(addr,nport,buf,bufsize,nn);
    //for(;;);

    ioctl(fduinput, UI_DEV_DESTROY);
	close(fduinput);
    return 0;
}




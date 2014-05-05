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

int fduinput;
typedef void (*sighandler_t)(int);
//static int gi=0;
int gpio0=0,gpio1=0,gpio2=0;
int key6[]={KEY_LEFT,KEY_RIGHT,KEY_UP,KEY_DOWN,KEY_ENTER,KEY_ESC};
//int key6[]={30,31,32,33,34,35};
/*
 press 32      up
 press 30        left
 press 31         right
 press 33         donw
 press 34           enter
 press 35           esc
*/

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
	for(i=0;i<6;i++) ioctl(fduinput, UI_SET_KEYBIT, key6[i]);
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
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "uinput-gpio");
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
    //int i;
    struct timeval v={0,300000};
    struct timeval iv={0,300000};
    struct itimerval my_timer={iv,v};
	int ms=40;
    FILE *fp;

	printf(" usage: ./gpioinput\n");
	printf(" usage: ./gpioinput 50         gpio scan timer: 50ms (default:40ms)\n");
	if(init_gpiokey()<0) {
		printf(" open uinput dev error\n");
		return -1;
	}
	if(argc>1){
		ms=atoi(argv[1]);
	}
	v.tv_sec=0;
	v.tv_usec=ms*1000;
	iv.tv_sec=0;
	iv.tv_usec=ms*1000;
	my_timer.it_interval=iv;
	my_timer.it_value=v;

    fp=fopen("/proc/gpio6","rt");
    fscanf(fp,"%x",&gpio0);
    fclose(fp);
	gpio1=gpio0;
	gpio2=gpio0;
	
    setitimer(ITIMER_REAL,&my_timer,0);
    signal(SIGALRM,(sighandler_t)foo);
    for(;;);

    ioctl(fduinput, UI_DEV_DESTROY);
	close(fduinput);
    return 0;
}




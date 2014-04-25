#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

typedef void (*sighandler_t)(int);
static int gi=0;
int gpio0=0,gpio1=0,gpio2=0;
int key6[]={30,31,32,33,34,35};

// press 1:press 0:release
void keyevent(int key,int press)
{
	if(press) printf(" press %d\n",key);
	else printf(" release %d\n",key);

	return;
}
void key1(int n)
{
	int i0,i1;i2;
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
    printf("0x%08x alarm :%d  time: %d %d \n",gpio,gi,(int)tv.tv_sec,(int)tv.tv_usec);
    gi++;

	return;
}

int main(int argc,char *argv[])
{
    int i;
    struct timeval v={0,300000};
    struct timeval iv={0,300000};
    struct itimerval my_timer={iv,v};
	int ms=30;
    FILE *fp;

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
    for(;gi<20;)i=5;
    //printf("hello world\n");
    return 0;
}




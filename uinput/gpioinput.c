#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

typedef void (*sighandler_t)(int);
static int gi=0;
foo(int theint)
{
    struct timeval tv;
    struct timezone tz;
    FILE *fp;
    int gpio;
    gettimeofday(&tv,&tz);
    fp=fopen("/proc/gpio6","rt");
    fscanf(fp,"%x",&gpio);
    fclose(fp);
    printf("0x%08x alarm :%d  time: %d %d \n",gpio,gi,(int)tv.tv_sec,(int)tv.tv_usec);
    gi++;
}

int main(int argc,char *argv[])
{
    int i;
    struct timeval v={0,300000};
    struct timeval iv={0,300000};
    struct itimerval my_timer={iv,v};
	int ms=30;

	if(argc>1){
		ms=atoi(argv[1]);
	}
	v.tv_sec=0;
	v.tv_usec=ms*1000;
	iv.tv_sec=0;
	iv.tv_usec=ms*1000;
	my_timer.it_interval=iv;
	my_timer.it_val=v;

    setitimer(ITIMER_REAL,&my_timer,0);
    signal(SIGALRM,(sighandler_t)foo);
    for(;gi<20;)i=5;
    printf("hello world\n");
    return 0;
}

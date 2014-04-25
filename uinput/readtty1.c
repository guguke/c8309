#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/vt.h>
#include <sys/kd.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/vt.h>

#define VTACQSIG SIGUSR1
#define VTRELSIG SIGUSR2
int main()
{
    int fd;
    int n,i;
    char buf[10];
    struct termios termdata;
    struct vt_mode vtMode;

    fd=open("/dev/tty1",O_RDONLY|O_NDELAY,0);
    if(fd<0){
          printf("open tty1 error\n");
         return 1;
    }
    tcgetattr(fd, &termdata);

    ioctl(fd, KDSKBMODE, K_RAW);

    termdata.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
    termdata.c_oflag = 0;
    termdata.c_cflag = CREAD | CS8;
    termdata.c_lflag = 0;
    termdata.c_cc[VTIME]=0;
    termdata.c_cc[VMIN]=1;
    cfsetispeed(&termdata, 9600);
    cfsetospeed(&termdata, 9600);
    tcsetattr(fd, TCSANOW, &termdata);


    //struct vt_mode vtMode;
    ioctl(fd, VT_GETMODE, &vtMode);

    // let us control VT switching
    vtMode.mode = VT_PROCESS;
    vtMode.relsig = VTRELSIG;
    vtMode.acqsig = VTACQSIG;
    ioctl(fd, VT_SETMODE, &vtMode);

    for(i=0;i<5;){
       n=read(fd,buf,1);
        if(n==1){
               printf(" read tty1: 0x%02x\n",buf[0]);
             i++;
              }
     }
    printf("hello world\n");
    return 0;
}

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B115200
#define MODEMDEVICE "/dev/ttyS0"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE; 

main(int argc,char *argv[])
{
	int fd,c, res;
	struct termios oldtio,newtio;
	char buf[355];
	char devname[30];
	int baud;
	int baudrate;
	int i;
        printf(" serial rcv ver 1.01\n");
	printf("usage: serialr /dev/ttyS1 115200\n");
	if(argc<3) return;

	strcpy(devname,argv[1]);
	baud=atoi(argv[2]);
	switch(baud){
		case 9600: baudrate=B9600; break;
		case 19200: baudrate=B19200; break;
		case 38400: baudrate=B38400; break;
		case 57600: baudrate=B57600; break;
		default: baudrate=B115200; break;
	}



	
	fd = open(devname, O_RDWR | O_NOCTTY ); 
	if (fd <0) {perror(devname); exit(-1); }
	
	tcgetattr(fd,&oldtio); /* save current port settings */
	
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = baudrate | CRTSCTS | CS8 | CLOCAL | CREAD;
	//newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	
	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;
	
	newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
	newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */
	
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);
	
	
	while (STOP==FALSE) {       /* loop for input */
		res = read(fd,buf,255);   /* returns after 5 chars have been input */
		buf[res]=0;               /* so we can printf... */
		printf("%s", buf);
		fflush(stdout);
		if (buf[0]=='z') STOP=TRUE;
	}
	tcsetattr(fd,TCSANOW,&oldtio);
}



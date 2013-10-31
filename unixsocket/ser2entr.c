#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
//#define NAME "socket"
/*
 * This program creates a UNIX domain datagram socket, binds a
 * name to it, then reads from the socket.
 */
main(int argc,char *argv[])
{
   int sock, length;
   struct sockaddr_un name;
   char buf[3000];
   char usname[300];
   int ret,i;

   printf(" usage: ser2netr name_unix_socket       (default: /dev/shm/ser2net\n");

   if(argc<2) strcpy(usname,"/dev/shm/ser2net");
   else strcpy(usname,argv[1]);
   unlink(usname);

   /* Create socket from which to read. */
   sock = socket(AF_UNIX, SOCK_DGRAM, 0);
   if (sock < 0) {
      perror("opening datagram socket");
      exit(1);
   }
  
   /* Create name. */
   name.sun_family = AF_UNIX;
   //strcpy(name.sun_path, NAME);
   strcpy(name.sun_path, usname);
   //name.sun_len = strlen(name.sun_path);
   if (bind(sock, (struct sockaddr *)&name, SUN_LEN(&name))) {
      perror("binding name to datagram socket");
      exit(1);
   }
   
   printf("socket (%s) :\n", usname);
   for(;;){
	   /* Read from the socket. */
	   ret = read(sock, buf, 2000);
	   if ( ret < 0){
		   perror("receiving datagram packet");
		   break;
	   }
	   buf[2000]=0;
	   printf(" ret: %d str:%s\n", ret,buf);
	   for(i=0;i<ret;i++) printf("%02d ",0x0ff & buf[i]);
	   printf("\n ==end== \n");
   }
   close(sock);
   //unlink(NAME);
   unlink(usname);
   return;
}

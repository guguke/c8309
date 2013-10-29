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
   char buf[1024];
   printf(" usage: ur name_unix_socket\n");
   if(argc<2)return;
   /* Create socket from which to read. */
   sock = socket(AF_UNIX, SOCK_DGRAM, 0);
   if (sock < 0) {
      perror("opening datagram socket");
      exit(1);
   }
  
   /* Create name. */
   name.sun_family = AF_UNIX;
   //strcpy(name.sun_path, NAME);
   strcpy(name.sun_path, argv[1]);
   //name.sun_len = strlen(name.sun_path);
   if (bind(sock, (struct sockaddr *)&name, SUN_LEN(&name))) {
      perror("binding name to datagram socket");
      exit(1);
   }
   
   printf("socket -->%s\n", NAME);
   /* Read from the socket. */
   if (read(sock, buf, 1024) < 0)
      perror("receiving datagram packet");
   printf("-->%s\n", buf);
   close(sock);
   //unlink(NAME);
   unlink(argv[1]);
}

#include <stdio.h>
#include <netinet/in.h>




//Of course you can also use the newer function inet_pton.





int main()
{
	char ip[]="255.255.255.0";
	struct in_addr inp;

	inet_aton(ip,&inp);
	printf(" 0x %08x    ip:%s\n",inp.s_addr,inet_ntoa(inp));
	printf(" htonl : 0x% 08x \n",htonl(inp.s_addr) );

	return 0;
}



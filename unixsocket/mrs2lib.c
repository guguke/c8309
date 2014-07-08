#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int udp2send(char *pin,int inlen,char *pout,int *poutlen)
{
	int op=0;  // 0: sendback

	memcpy(pout,pin,inlen);
	*poutlen=inlen;

	return 0;
}

int tcp2send(char *pin,int inlen,char *pout,int *poutlen)
{
	int op=0;  // 0: sendback

	memcpy(pout,pin,inlen);
	*poutlen=inlen;

	return 0;
}






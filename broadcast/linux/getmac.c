#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
// ifname:eth0 eth1
// MAC_str : 001122334455      return val
// maclong : 00:11:22:33:44:55 reutrn val
void mac_eth0(char *ifname,unsigned char *MAC_str,char *maclong)
{
#define HWADDR_len 6
    int s,i;
    struct ifreq ifr;
    s = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifr.ifr_name, ifname);
    ioctl(s, SIOCGIFHWADDR, &ifr);
    for (i=0; i<HWADDR_len; i++){
        sprintf(&MAC_str[i*2],"%02X",((unsigned char*)ifr.ifr_hwaddr.sa_data)[i]);
        sprintf(&maclong[i*3],"%02X:",((unsigned char*)ifr.ifr_hwaddr.sa_data)[i]);
    }
    maclong[17]='\0';
    MAC_str[12]='\0';
}

int main(int argc, char *argv[])
{
    unsigned char mac[13];
    char maclong[30];
    char ifname[20];
    if (argc>1) strcpy(ifname,argv[1]);
    else strcpy(ifname,"eth0");

    mac_eth0(ifname,mac,maclong);
    printf("%s: %s\n%s\n",ifname,mac,maclong);

    return 0;
}

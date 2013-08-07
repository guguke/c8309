#include <stdio.h>
#include <stropts.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netdevice.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

//int domains[] = { AF_INET, AF_INET6 };
//domain: AF_INET or AF_INET6
// pif: "eth0" or "eth2"
// pip: ip return
int print_addresses(const int domain,char *pif,char *pip)
{
    int s;
    struct ifconf ifconf;
    struct ifreq ifr[50];
    int ifs;
    int i;
    int ret=0;// interface not found

    s = socket(domain, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return 0;
    }

    ifconf.ifc_buf = (char *) ifr;
    ifconf.ifc_len = sizeof ifr;

    if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) {
        perror("ioctl");
        return 0;
    }

    ifs = ifconf.ifc_len / sizeof(ifr[0]);
    // interfaces number
    //printf("interfaces = %d:\n", ifs);
    for (i = 0; i < ifs; i++) {
        char ip[INET_ADDRSTRLEN];
        struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;

        if (!inet_ntop(domain, &s_in->sin_addr, ip, sizeof(ip))) {
            perror("inet_ntop");
            return 0;
        }

        //   printf("%s - %s\n", ifr[i].ifr_name, ip);
        if (strcmp(pif,ifr[i].ifr_name)==0) strcpy(pip,ip);
        ret=1;
    }

    close(s);

    return ret;
}

int main(int argc, char *argv[])
{
    int domains[] = { AF_INET, AF_INET6 };
    int i;
    char ip[30];
    char ifname[30];

    ip[0]=0;
    if (argc>1)strcpy(ifname,argv[1]);
    else
        strcpy(ifname,"eth0");
    print_addresses(AF_INET,ifname,ip);

    printf("%s : %s\n",ifname,ip);
    return 0;
}


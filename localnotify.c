#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>

#include "localnotify.h"


#if 0
int get_sock(){
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    /*struct sockaddr_ll laddr = {0};*/

    if (sock == -1 || setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) == -1) {
        return -1;
    }

    /*
     * laddr.sll_family = AF_PACKET;
     * laddr.sll_protocol = htons(ETH_P_ALL);
     * laddr.sll_ifindex = if_nametoindex("eth0");
     * laddr.sll_ifindex = if_nametoindex("wlp3s0");
     * laddr.sll_pkttype = PACKET_BROADCAST;
     * laddr.sll_halen = 6;
    */
    /*
     * laddr.sll_addr[0] = 0x08;
     * laddr.sll_addr[1] = 0x11;
     * laddr.sll_addr[2] = 0x96;
     * laddr.sll_addr[3] = 0x99;
     * laddr.sll_addr[4] = 0x37;
     * laddr.sll_addr[5] = 0x90;
    */

    // it works with bind() commented out. prob not needed
    /*printf("bind: %i\n", bind(sock, (struct sockaddr*)&laddr, sizeof(struct sockaddr_ll)));*/
    return sock;
}
#endif

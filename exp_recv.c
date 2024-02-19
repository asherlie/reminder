#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
/*
 * try receiving message sent below
 * then try from multiple threads to see if each one can get a copy
 * this is all the proof of concept needed
*/

uint8_t bcast_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
struct pkt{
    struct ethhdr ehdr;
    uint8_t tag[5];
    char msg[5];
}__attribute__((__packed__));

void p_maddr(uint8_t addr[6]){
    printf("%.2hx", *addr);
    for (uint8_t i = 1; i < 6; ++i) {
        printf(":%.2hx", addr[i]);
    }
    puts("");
}


int main(){
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    printf("socket(): %i\n", sock);
    struct sockaddr_ll laddr = {0};
    printf("sockopt: %i\n", setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)));
    /*int sock = socket(AF_INET, SOCK_STREAM, 0);*/

    laddr.sll_family = AF_PACKET;
    laddr.sll_protocol = htons(ETH_P_ALL);
    laddr.sll_ifindex = if_nametoindex("eth0");
    laddr.sll_ifindex = if_nametoindex("wlp3s0");
    laddr.sll_pkttype = PACKET_BROADCAST;
    laddr.sll_halen = 6;
    laddr.sll_addr[0] = 0x08;
    laddr.sll_addr[1] = 0x11;
    laddr.sll_addr[2] = 0x96;
    laddr.sll_addr[3] = 0x99;
    laddr.sll_addr[4] = 0x37;
    laddr.sll_addr[5] = 0x90;

    // it works with bind() commented out. prob not needed
    printf("bind: %i\n", bind(sock, (struct sockaddr*)&laddr, sizeof(struct sockaddr_ll)));

    struct pkt buf;

    while(1){
        memset(&buf, 0, sizeof(struct pkt));
        if (recvfrom(sock, &buf, sizeof(struct pkt), 0, NULL, NULL) == -1){
            perror("recvfrom()");
        }
        if (!memcmp(buf.ehdr.h_dest, bcast_addr, 6) &&  strstr(buf.msg, "ASHE")) {
            p_maddr(buf.ehdr.h_source);
            p_maddr(buf.ehdr.h_dest);
            p_maddr(buf.tag);
            printf("\n\"%s\"", buf.msg);
        }
    }
}

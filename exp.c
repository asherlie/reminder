#include <sys/socket.h>
#include <string.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>
/*
 * testing:
 *  whether multiple threads can read() the same packet that is sent to broadcast
 *  if i can write a good local network notifier
 */

// put an identifiable thing, like 0xdecaf
struct pkt{
    struct ethhdr ehdr;
    uint8_t tag[5];
    char msg[5];
}__attribute__((__packed__));

int main() {
    errno = 99;
    // try sock raw with sendto (and recvfrom?)
    /*int sock = socket(AF_INET, SOCK_RAW, 0);*/
    // ugh, just try this with sock_dgram. it apparently takes care of the eth header for me
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

    laddr.sll_addr[0] = 0x0d;
    laddr.sll_addr[1] = 0x0e;
    laddr.sll_addr[2] = 0x0c;
    laddr.sll_addr[3] = 0x0a;
    laddr.sll_addr[4] = 0x0f;
    laddr.sll_addr[5] = 0x00;
    // ah, i see, local address is auto filled in during sendto

    // it works with bind() commented out. prob not needed
    printf("bind: %i\n", bind(sock, (struct sockaddr*)&laddr, sizeof(struct sockaddr_ll)));

    struct sockaddr_ll addr = {0};
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_ifindex = if_nametoindex("eth0");
    printf("addr.ifindex: %i\n", addr.sll_ifindex);
    addr.sll_ifindex = if_nametoindex("wlp3s0");
    printf("addr.ifindex: %i\n", addr.sll_ifindex);
    addr.sll_pkttype = PACKET_BROADCAST;
    addr.sll_halen = 6;
    for(uint8_t i = 0; i < 6; ++i) {
        addr.sll_addr[i] = 0xff;
    }

    struct pkt p = {0};
    for(uint8_t i = 0; i < 6; ++i) {
        p.ehdr.h_dest[i] = 0xff;
    }
    
    p.tag[0] = 0xde;
    p.tag[1] = 0xca;
    p.tag[2] = 0xfd;
    p.tag[3] = 0xec;
    p.tag[4] = 0xaf;

    p.ehdr.h_proto = htons(8);

    p.ehdr.h_source[0] = 0x08;
    p.ehdr.h_source[1] = 0x11;
    p.ehdr.h_source[2] = 0x96;
    p.ehdr.h_source[3] = 0x99;
    p.ehdr.h_source[4] = 0x37;
    p.ehdr.h_source[5] = 0x90;
    strcpy(p.msg, "ASHE");


    printf("st: %li\n", sendto(sock, &p, sizeof(struct pkt), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_ll)));
    printf("%i == %i\n", errno, EINVAL);
    perror("");
    /*listen*/
    /*accept*/
}

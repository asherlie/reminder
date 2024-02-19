/*
 * TODO: get interface name programatically
 * I have another project that does this, look around. may be ashnet
 */
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>

const uint8_t bcast_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
const uint8_t tag_bytes[5] = {0xde, 0xca, 0xfd, 0xec, 0xaf};

struct pkt{
    struct ethhdr ehdr;
    uint8_t tag[5];
    char msg[5];
}__attribute__((__packed__));

#define register_ln_payload(name, payload) \
    struct name{ \
        struct pkt _p; \
        payload _pl; \
    }__attribute__((__packed__)); \
\
    _Bool broadcast_##name(payload pl){ \
        struct sockaddr_ll addr = {0}; \
        addr.sll_family = AF_PACKET; \
        addr.sll_protocol = htons(ETH_P_ALL); \
        addr.sll_ifindex = if_nametoindex("eth0"); \
        printf("addr.ifindex: %i\n", addr.sll_ifindex); \
        addr.sll_ifindex = if_nametoindex("wlp3s0"); \
        printf("addr.ifindex: %i\n", addr.sll_ifindex); \
        addr.sll_pkttype = PACKET_BROADCAST; \
        addr.sll_halen = 6; \
        for(uint8_t i = 0; i < 6; ++i) { \
            addr.sll_addr[i] = 0xff; \
        } \
        struct name packet = {0}; \
        packet._pl = pl; \
        memcpy(packet._p.ehdr.h_dest, bcast_addr, 6); \
        memcpy(packet._p.tag, tag_bytes, 6); \
        packet._p.ehdr.h_proto = htons(8); \
        /* I believe source mac addr is auto-set */ \
        return sendto(sock, &packet, sizeof(struct name), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_ll)) == sizeof(struct name); \
    }

void p_maddr(uint8_t addr[6]){
    printf("%.2hX", *addr);
    for (uint8_t i = 1; i < 6; ++i) {
        printf(":%.2hX", addr[i]);
    }
    puts("");
}

int get_sock(){
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    printf("socket(): %i\n", sock);
    struct sockaddr_ll laddr = {0};

    if (sock == -1 || setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) == -1) {
        return -1;
    }
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
    return sock;
}

_Bool broadcast_packet(){
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
}

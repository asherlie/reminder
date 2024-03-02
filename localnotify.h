/*
 * TODO: get interface name programatically
 * I have another project that does this, look around. may be ashnet
 *
 * TODO: figure out how to get this working without root privileges
 */
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stddef.h>

static const uint8_t bcast_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static const uint8_t tag_bytes[5] = {0xde, 0xca, 0xfd, 0xec, 0xaf};

static volatile int socks[2] = {-1, -1};

struct pkt{
    struct ethhdr ehdr;
    uint8_t tag[5];
    char msg[5];
}__attribute__((__packed__));

static inline int get_sock(){
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

// it's up to the user to be consistent with their use of payload_identifiers
#define register_ln_payload(name, iface, payload, payload_identifier) \
    /* ugh, should this just take in arbitrary data? no. i like that it's typed */ \
    struct name{ \
        struct pkt _p; \
        uint8_t _pl_id; \
        payload _pl; \
    }__attribute__((__packed__)); \
\
    _Bool broadcast_##name(payload pl){ \
        struct sockaddr_ll addr = {0}; \
        struct name packet = {0}; \
\
        if (socks[0] == -1) { \
            socks[0] = get_sock(); \
        } \
        addr.sll_family = AF_PACKET; \
        addr.sll_protocol = htons(ETH_P_ALL); \
        addr.sll_ifindex = if_nametoindex(iface); \
        addr.sll_pkttype = PACKET_BROADCAST; \
        addr.sll_halen = 6; \
        for(uint8_t i = 0; i < 6; ++i) { \
            addr.sll_addr[i] = 0xff; \
        } \
        packet._pl = pl; \
        packet._pl_id = payload_identifier; \
        memcpy(packet._p.ehdr.h_dest, bcast_addr, 6); \
        memcpy(packet._p.tag, tag_bytes, 5); \
        packet._p.ehdr.h_proto = htons(8); \
        /* I believe source mac addr is auto-set */ \
        return sendto(socks[0], &packet, sizeof(struct name), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_ll)) == sizeof(struct name); \
    } \
\
    payload recv_##name(_Bool* success, uint8_t* src_addr){ \
        /* there's no need for packet to be on the heap because this is a macro */ \
        struct name packet = {0}; \
        if (socks[1] == -1) { \
            socks[1] = get_sock(); \
        } \
        if (success) { \
            *success = 1; \
        } \
        while (1) { \
            if (recvfrom(socks[1], &packet, sizeof(struct name), 0, NULL, NULL) == -1){ \
                if (success) { \
                    *success = 0; \
                    return packet._pl; \
                } \
            } \
            if (memcmp(packet._p.tag, tag_bytes, 5) || memcmp(packet._p.ehdr.h_dest, bcast_addr, 6) || \
                packet._pl_id != payload_identifier) { \
                continue; \
            } \
            if (src_addr) { \
                memcpy(src_addr, packet._p.ehdr.h_source, 6); \
            } \
            return packet._pl; \
        } \
    }

static inline void p_maddr(uint8_t addr[6]){
    printf("%.2hX", *addr);
    for (uint8_t i = 1; i < 6; ++i) {
        printf(":%.2hX", addr[i]);
    }
}

static inline _Bool get_local_addr(char* iname, uint8_t addr[6]){
    int sock = socket(AF_PACKET, SOCK_RAW, 0);
    struct ifreq ifr = {0};
    struct ifreq if_mac = {0};
    strncpy(ifr.ifr_name, iname, IFNAMSIZ-1);
    strncpy(if_mac.ifr_name, iname, IFNAMSIZ-1);
    if(ioctl(sock, SIOCGIFINDEX, &ifr) == -1)perror("IOCTL");
    if(ioctl(sock, SIOCGIFHWADDR, &if_mac) < 0)perror("HWADDR");
    memcpy(addr, if_mac.ifr_addr.sa_data, 6);
    close(sock);
    return 1;
}

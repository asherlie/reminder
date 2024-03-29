/*
 * TODO: get interface name programatically
 * I have another project that does this, look around. may be ashnet
 *
 * TODO: figure out how to get this working without root privileges
 */
#include <ctype.h>
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
static const uint8_t nuffin[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
static const uint8_t tag_bytes[5] = {0xde, 0xca, 0xfd, 0xec, 0xaf};
static const unsigned short proto = 0xdeca;

static volatile int socks[2] = {-1, -1};
static uint8_t local_addr[6] = {0};
static volatile _Bool local_addr_set = 0;

struct pkt{
    struct ethhdr ehdr;
    uint8_t tag[5];
}__attribute__((__packed__));

static inline int get_sock(){
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    //int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    //struct sockaddr_ll laddr = {0};

    if (sock == -1 || setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) == -1) {
        return -1;
    }

/*
 *      laddr.sll_family = AF_PACKET;
 *      laddr.sll_protocol = htons(ETH_P_ALL);
 *      laddr.sll_ifindex = if_nametoindex("eth0");
 *      laddr.sll_ifindex = if_nametoindex("wlp3s0");
 *      laddr.sll_pkttype = PACKET_BROADCAST;
 *      laddr.sll_halen = 6;
 *      laddr.sll_addr[0] = 0x08;
 *      laddr.sll_addr[1] = 0x11;
 *      laddr.sll_addr[2] = 0x96;
 *      laddr.sll_addr[3] = 0x99;
 *      laddr.sll_addr[4] = 0x37;
 *      laddr.sll_addr[5] = 0x90;
 * 
 *     // it works with bind() commented out. prob not needed
 *     printf("bind: %i\n", bind(sock, (struct sockaddr*)&laddr, sizeof(struct sockaddr_ll)));
*/
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
        if (!local_addr_set) { \
            get_local_addr(iface, local_addr); \
            local_addr_set = 1; \
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
        packet._p.ehdr.h_proto = htons(ETH_P_ALL); \
        local_addr[0] = 0x88; \
        memcpy(packet._p.ehdr.h_source, local_addr, 6); \
        memcpy(packet._p.ehdr.h_source, nuffin, 6); \
        memcpy(packet._p.ehdr.h_dest, bcast_addr, 6); \
        memcpy(packet._p.tag, tag_bytes, 5); \
        /* packet._p.ehdr.h_proto = htons(8);, hmm this isn't needed */ \
        /* I believe source mac addr is auto-set */ \
        return sendto(socks[0], &packet, sizeof(struct name), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_ll)) == sizeof(struct name); \
    } \
\
/* TODO: remove src_addr arg if i can get it copied to header!, wait i obv can. nvm, header not returned */ \
    payload recv_##name(_Bool* success, uint8_t* src_addr){ \
        /* there's no need for packet to be on the heap because this is a macro */ \
        struct name packet = {0}; \
        struct sockaddr_ll r_addr = {0}; \
        ssize_t pktlen; \
        socklen_t r_slen = sizeof(struct sockaddr_ll); \
        if (socks[1] == -1) { \
            socks[1] = get_sock(); \
        } \
        if (success) { \
            *success = 1; \
        } \
        while (1) { \
            if ((pktlen = recvfrom(socks[1], &packet, sizeof(struct name), 0, (struct sockaddr*)&r_addr, &r_slen)) == -1){ \
                if (success) { \
                    *success = 0; \
                    return packet._pl; \
                } \
            } \
            if (memcmp(packet._p.tag, tag_bytes, 5) || memcmp(packet._p.ehdr.h_dest, bcast_addr, 6) || \
                packet._pl_id != payload_identifier) { \
                continue; \
            } \
            puts("FOUND MATCHING PACKET"); \
            if (src_addr) { \
                /* hmm, neither of these seem to work. we can confirmed send bytes, however, 0x69 is being passed along */ \
                memcpy(src_addr, packet._p.ehdr.h_source, 6); \
                memcpy(src_addr, r_addr.sll_addr, 6); \
                printf("lenny: %d\n", r_addr.sll_halen); \
                p_maddr(packet._p.ehdr.h_source); \
                puts(""); \
                p_maddr(r_addr.sll_addr); \
                puts(""); \
                p_buf((uint8_t*)&packet, sizeof(struct name), 1); \
                /* okay, all fields are properly set except for h_source, EVEN h_dest is all 0xff - this
                 * could just be a problem with same-host behavior. testing with rasp pi */ \
                printf("%d should be %d\n", r_addr.sll_family, AF_PACKET); \
                printf("%d should be %d\n", r_addr.sll_halen, 6); \
            } \
            return packet._pl; \
        } \
    }

    /*
	 * unsigned char	h_dest[ETH_ALEN];	[> destination eth addr	<]
	 * unsigned char	h_source[ETH_ALEN];	[> source ether addr	<]
	 * __be16		h_proto;		[> packet type ID field	<]
    */

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
static inline void p_buf(uint8_t* buf, size_t sz, _Bool showchar){
    for (size_t i = 0; i < sz; ++i) {
        if (showchar && isalpha(buf[i])) {
            printf("%c  ", buf[i]);
        }
        else {
            printf("%.2hX ", buf[i]);
        }
        if (!(i % 4)) {
            printf(" ");
        }
        if (!(i % 8)) {
            puts("");
        }
    }
    puts("");
}

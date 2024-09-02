/*
 * TODO: move this to a new repo
 */
#include <ctype.h>
#include <assert.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stddef.h>
#include <netinet/ip.h>

static const uint8_t bcast_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static const uint8_t nuffin[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
static const uint8_t tag_bytes[5] = {0xde, 0xca, 0xfd, 0xec, 0xaf};
static const unsigned short proto = 0xdeca;

static volatile int socks[2] = {-1, -1};
//static uint8_t local_addr[6] = {0};
static volatile _Bool local_addr_set = 0;

struct ll_entry{
    struct in_addr addr;
    char ifname[IFNAMSIZ];

    struct ll_entry* next;
};

struct iface_lookup{
    struct ll_entry* first;
};

struct iface_lookup il = {0};

/* returns a freshly allocated entry upon failure, NULL on success */
static inline struct ll_entry* lookup_iface(char* iface, struct in_addr* result_addr) {
    struct ll_entry* prev_le;

    if (!il.first) {
        il.first = malloc(sizeof(struct ll_entry));
        strncpy(il.first->ifname, iface, sizeof(il.first->ifname));
        il.first->next = NULL;
        return il.first;
    }
    for (struct ll_entry* le = il.first; le; le = le->next) {
        if (!strncmp(le->ifname, iface, IFNAMSIZ)) {
            *result_addr = le->addr;
            return NULL;
        }
        prev_le = le;
    }
    prev_le->next = malloc(sizeof(struct ll_entry));
    strncpy(il.first->ifname, iface, sizeof(il.first->ifname));
    prev_le->next->next = NULL;
    return prev_le->next;
}

static inline _Bool get_broadcast_ip(char* iface, struct in_addr* src_addr) {
    // which one?
    //if (inet_pton(AF_INET, "192.168.86.255", &src_addr->s_addr) <= 0)  {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct ifreq ifr = {0};
    struct ll_entry* ins = lookup_iface(iface, src_addr);

    if (!ins) {
        return 1;
    }

    strncpy(ifr.ifr_name, iface, IFNAMSIZ);

    if (ioctl(sock, SIOCGIFBRDADDR, &ifr) != 0) {
        perror("ioctl");
    }
    if (ifr.ifr_broadaddr.sa_family != AF_INET) {
        return 0;
    }

    close(sock);
    memcpy(src_addr, &((struct sockaddr_in*)&ifr.ifr_broadaddr)->sin_addr, sizeof(struct in_addr));
    ins->addr = *src_addr;
    return 1;
}

static inline int get_inet_sock(_Bool sender) {
    //int sock = socket(AF_INET, SOCK_STREAM, 0);
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {0};

    if (sock == -1 || setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) == -1 || setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) == -1) {
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(0xb11);

    if (sender) {
        printf("bind: %i\n", bind(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)));
        perror("bind");
    } else {
        listen(sock, 0);
    }

    return sock;
}

// it's up to the user to be consistent with their use of payload_identifiers
#define register_ln_payload(name, iface, payload, payload_identifier) \
    /* ugh, should this just take in arbitrary data? no. i like that it's typed */ \
    struct bc_##name{ \
        uint8_t tag[5]; \
        uint8_t _pl_id; \
        payload _pl; \
    }__attribute__((__packed__)); \
\
    struct rcv_##name{ \
        /* struct ethhdr ehdr; */ \
        /* struct iphdr ihdr; */ \
        uint8_t tag[5]; \
        uint8_t _pl_id; \
        payload _pl; \
    }__attribute__((__packed__)); \
\
    _Bool broadcast_##name(payload pl) { \
        struct sockaddr_in addr = {0}; \
        struct bc_##name packet = {0}; \
\
        if (socks[0] == -1) { \
            socks[0] = get_inet_sock(1); \
        } \
        addr.sin_family = AF_INET; \
        addr.sin_port = htons(0xb11); \
        get_broadcast_ip(iface, &addr.sin_addr); \
        /* maybe set address to all 0xff */ \
        packet._pl = pl; \
        packet._pl_id = payload_identifier; \
        memcpy(packet.tag, tag_bytes, 5); \
        return sendto(socks[0], &packet, sizeof(struct bc_##name), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == sizeof(struct bc_##name); \
    } \
\
\
    payload recv_##name(_Bool* success, struct in_addr* src_addr) { \
        struct rcv_##name packet = {0}; \
        struct sockaddr_in r_addr = {0}; \
        ssize_t pktlen; \
        socklen_t r_slen = sizeof(struct sockaddr_in); \
        if (socks[1] == -1) { \
            socks[1] = get_inet_sock(1); \
        } \
        if (success) { \
            *success = 1; \
        } \
        while (1) { \
            if ((pktlen = recvfrom(socks[1], &packet, sizeof(struct rcv_##name), 0, (struct sockaddr*)&r_addr, &r_slen)) == -1){ \
                if (success) { \
                    *success = 0; \
                    return packet._pl; \
                } \
            } \
            if (memcmp(packet.tag, tag_bytes, 5) || /* memcmp(packet._p.ehdr.h_dest, bcast_addr, 6) || */  \
                packet._pl_id != payload_identifier) { \
                /* if (memcmp(prev_tag, packet._p.tag, 1)) { */\
                continue; \
            } \
            if (src_addr) { \
                /* hmm, neither of these seem to work. we can confirmed send bytes, however, 0x69 is being passed along */ \
                memcpy(src_addr, &r_addr.sin_addr, sizeof(struct in_addr)); \
                p_buf((uint8_t*)&packet, sizeof(struct rcv_##name), 1); \
                /* okay, all fields are properly set except for h_source, EVEN h_dest is all 0xff - this
                 * could just be a problem with same-host behavior. testing with rasp pi */ \
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

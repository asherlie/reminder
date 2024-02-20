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

int get_sock();

// it's up to the user to be consistent with their use of payload_identifiers
#define register_ln_payload(name, iface, payload, payload_identifier) \
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
    payload recv_##name(_Bool* success){ \
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
            return packet._pl; \
        } \
    }

void p_maddr(uint8_t addr[6]);

/*
 * this can be used for beep by having a single struct used to send info, store it, and add it locally
 * this struct can be used to spread a message accross local net
*/
#include "localnotify.h"

struct string{
    char str[20];
    float f;
};

register_ln_payload(foo, "wlp3s0", int, 0)
register_ln_payload(str, "wlp3s0", struct string, 1)

int main(int a, char** b){
    struct string s, recv;
    uint8_t addr[6] = {0};
    if (a >= 2) {
        strncpy(s.str, b[1], 19);
        s.f = 94.1129;
        broadcast_foo(1);
        broadcast_foo(2);
        broadcast_foo(3023);
        broadcast_foo(1113);
        broadcast_str(s);
    }
    recv = recv_str(NULL, addr);
    printf("\"%s\", %f\n", recv.str, recv.f);
    p_maddr(addr);
    puts("");
}

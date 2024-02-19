#include "localnotify.h"

register_ln_payload(foo, "wlp3s0", int, 0);
int main(){

    broadcast_foo(3);
    printf("%i\n", recv_foo(NULL));
}

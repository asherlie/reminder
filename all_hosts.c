#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/* first approach: ping 224.0.0.1 */
int main(){
    struct sockaddr_in lookup_sin = {.sin_family = AF_INET, .sin_port = 80};
    struct sockaddr_in sin = {.sin_family = AF_INET, .sin_port = 80, .sin_addr.s_addr = INADDR_ANY};
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    char buf[20] = {0};

    sin.sin_addr.s_addr = inet_addr("224.0.0.1");
    bind(sock, (struct sockaddr*)&sin, sizeof(struct sockaddr_in));

    if(connect(sock, (struct sockaddr*)&lookup_sin, sizeof(struct sockaddr_in))){
        perror("connect()");
        return EXIT_FAILURE;
    }

    write(sock, "ping", 5);
    printf("read %li bytes\n", read(sock, buf, 10));
    puts(buf);
}

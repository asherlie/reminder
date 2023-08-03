#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/* first approach: ping 224.0.0.1 */
int main(){
    struct sockaddr_in sin = {.sin_family = AF_INET, .sin_port = 80};
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sin.sin_addr.s_addr = inet_addr("224.0.0.1");
    /*bind();*/
}

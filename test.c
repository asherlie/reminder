/*
 * this can be used for beep by having a single struct used to send info, store it, and add it locally
 * this struct can be used to spread a message accross local net
*/
#include "localnotify.h"
#include <fcntl.h>
#include <assert.h>

struct string{
    char str[20];
    float f;
};

register_ln_payload(foo, "wlp3s0", int, 0)
register_ln_payload(str, "wlp3s0", struct string, 1)
/*register_ln_payload(foo, "enp0s25", int, 0)*/
/*register_ln_payload(str, "enp0s25", struct string, 1)*/

struct filehdr{
    uint64_t len;
    char fn[32];
};

struct filechunk{
    uint32_t chunklen;
    uint32_t chunkno;
    uint8_t data[1028];
};

register_ln_payload(fhdr, "wlp3s0", struct filehdr, 9)
register_ln_payload(filechunk, "wlp3s0", struct filechunk, 10)

void broadcast_file(char* fn, uint32_t chunksz) {
    int fd = open(fn, O_RDONLY);
    uint64_t len = lseek(fd, 0, SEEK_END), sent = 0, b_read;
    struct filehdr fhdr = {.len = len};
    struct filechunk chunk;

    assert(chunksz <= sizeof(chunk.data));

    lseek(fd, 0, SEEK_SET);
    strncpy(fhdr.fn, fn, sizeof(fhdr.fn));

    printf("broadcasting file %s of length %lu\n", fn, len);
    broadcast_fhdr(fhdr);

    chunk.chunkno = 0;
    while (sent != len) {
        // not strictly necessary
        memset(chunk.data, 0, sizeof(chunk.data));
        b_read = read(fd, chunk.data, chunksz);
        chunk.chunklen = b_read;
        broadcast_filechunk(chunk);
        sent += b_read;
        // printf("sent %lu chunk: \"%s\"\n", b_read, chunk.data);
        ++chunk.chunkno;
    }
    close(fd);
}

void recv_file(char* fn) {
    struct filehdr hdr = recv_fhdr(NULL, NULL);
    uint32_t b_recvd = 0;
    struct filechunk fc;
    char full_fn[FILENAME_MAX] = {0};
    int fd;

    sprintf(full_fn, "%s_.%s", hdr.fn, fn);
    printf("received header for file \"%s\" of size %lu\nwriting to: \"%s\"\n", hdr.fn, hdr.len, full_fn);
    fd = open(full_fn, O_WRONLY | O_CREAT, 0666);

    while (b_recvd < hdr.len) {
        fc = recv_filechunk(NULL, NULL);
        write(fd, fc.data, fc.chunklen);
        b_recvd += fc.chunklen;
    }
    close(fd);
}

int main(int a, char** b){
     if (a >= 2) {
         broadcast_file(b[1], 1000);
     } else {
        recv_file("F_OUT.o");
     }
     return 0;
     struct string s, recv;
     /*uint8_t addr[6] = {0};*/
     struct in_addr addr;
     if (a >= 2) {
         strncpy(s.str, b[1], 19);
         s.f = 94.1129;
         broadcast_foo('a');
         broadcast_foo('b');
         broadcast_foo('C');
         broadcast_foo(1113);
         broadcast_str(s);
     }
     recv = recv_str(NULL, &addr);
     printf("\"%s\", %f\n", recv.str, recv.f);
     /*p_maddr(addr);*/
     puts("");
}

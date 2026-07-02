/* demo_target.c - touches disk and network so syspy has something to see. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(void) {
    /* Disk activity */
    int fd = open("/tmp/syspy_demo.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, "hello from syspy demo\n", 22);
        close(fd);
    }
    fd = open("/tmp/syspy_demo.txt", O_RDONLY);
    if (fd >= 0) {
        char buf[64];
        read(fd, buf, sizeof(buf));
        close(fd);
    }
    unlink("/tmp/syspy_demo.txt");

    /* Network activity: connect to localhost, ignore failure */
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s >= 0) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(9); /* discard service, likely closed */
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        connect(s, (struct sockaddr *)&addr, sizeof(addr));
        close(s);
    }

    printf("demo target finished\n");
    return 0;
}

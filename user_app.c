#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#define MYPROTO NETLINK_USERSOCK
#define NETLINK_GROUP_1 1
#define NETLINK_GROUP_2 17

#define RECV_BUFFER_SIZE 1024

void read_message(int sock) {
    struct sockaddr_nl nladdr;
    struct iovec iov;
    struct msghdr msg;
    char buf[RECV_BUFFER_SIZE];
    int ret;

    iov.iov_base = (void *)buf;
    iov.iov_len = sizeof(buf);
    
    msg.msg_name = (void *)&(nladdr);
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
        
    printf("Listen for message...\n");
    ret = recvmsg(sock, &msg, 0);
    if (ret < 0) {
        printf("Error receiving message: %s\n", strerror(errno));
        return;
    }
    char *payload = NLMSG_DATA((struct nlmsghdr *)&buf);
    printf("Received message: %s\n", payload);
}

int main() {
    struct sockaddr_nl src_addr;
    int sock_fd;

    sock_fd = socket(PF_NETLINK, SOCK_RAW, MYPROTO);
    if (sock_fd < 0) {
        perror("Error creating socket.\n");
        return -1;
    }

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();
    bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

    int group = NETLINK_GROUP_2;
    if (setsockopt(sock_fd, SOL_NETLINK, NETLINK_ADD_MEMBERSHIP, &group, sizeof(group))) {
        printf("setsockopt(NETLINK_ADD_MEMBERSHIP): \n", strerror(errno));
        close(sock_fd);
        return -1;
    }

    while (1) 
        read_message(sock_fd);

    close(sock_fd);

    return 0;
}

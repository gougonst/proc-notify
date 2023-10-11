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

void send_kernel_message(int sock, char *message) {
    struct sockaddr_nl dest_addr;
    struct nlmsghdr *nlh;
    struct msghdr msg;
    struct iovec iov;
    int ret;
    int message_size = strlen(message) + 1;

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(message_size));
    nlh->nlmsg_len = NLMSG_SPACE(message_size);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;
    strncpy(NLMSG_DATA(nlh), message, message_size);

    memset(&iov, 0, sizeof(iov));
    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    printf("Sending...\n");
    ret = sendmsg(sock, &msg, 0);
    if (ret < 0) {
        printf("Error sending message: %s\n", strerror(errno));
        close(sock);
        return;
    }
    printf("Send message: \'%s\' to kernel.\n", message);
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

    send_kernel_message(sock_fd, "Hello from user space!");

    while (1) 
        read_message(sock_fd);

    close(sock_fd);

    return 0;
}

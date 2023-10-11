#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <linux/netlink.h>

#define MYPROTO NETLINK_USERSOCK

#define RECV_BUFFER_SIZE 1024

int sock_fd;
pthread_mutex_t lock;
int exit_flag = 0;

int init_netlink_socket() {
    struct sockaddr_nl src_addr;
    int sock;

    printf("(%d) Create socket...\n", getpid());
    sock = socket(PF_NETLINK, SOCK_RAW, MYPROTO);
    if (sock < 0) {
        perror("Error creating socket.\n");
        return -1;
    }

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();
    bind(sock, (struct sockaddr*)&src_addr, sizeof(src_addr));

    fcntl(sock, F_SETFL, O_NONBLOCK);
    
    return sock;
}

void send_kernel_message(char *message) {
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
    ret = sendmsg(sock_fd, &msg, 0);
    if (ret < 0) {
        printf("Error sending message: %s\n", strerror(errno));
        close(sock_fd);
        return;
    }
    printf("Send message: \'%s\' to kernel.\n", message);
}

void read_message() {
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

    pthread_mutex_lock(&lock);
    ret = recvmsg(sock_fd, &msg, 0);
    pthread_mutex_unlock(&lock);

    if (ret > 0) {
        char *payload = NLMSG_DATA((struct nlmsghdr *)&buf);
        printf("Received message: %s\n", payload);
    }
}

void handle_register() {
    int group;
    scanf("%d", &group);
    
    if (setsockopt(sock_fd, SOL_NETLINK, NETLINK_ADD_MEMBERSHIP, &group, sizeof(group))) {
        printf("setsockopt(NETLINK_ADD_MEMBERSHIP): \n", strerror(errno));
        close(sock_fd);
        return;
    }
}

void handle_send() {
    char message[RECV_BUFFER_SIZE];
    scanf("%s", message);
    
    send_kernel_message(message);
}

void *command_thread(void *arg) {
    char cmd[10];

    while (!exit_flag) {
        printf("-----------------------------------\n");
        printf("| reg  - Register multicast group |\n");
        printf("| send - send message to kernel   |\n");
        printf("| exit                            |\n");
        printf("-----------------------------------\n");
        printf("Enter command: ", cmd);
        
        if (scanf("%s", cmd) == 1) {
            pthread_mutex_lock(&lock);
            if (strcmp(cmd, "reg") == 0) 
                handle_register(sock_fd);
            else if (strcmp(cmd, "send") == 0) 
                handle_send(sock_fd);
            else if (strcmp(cmd, "exit") == 0) 
                exit_flag = 1;
            else 
                printf("Unknown command: %s\n", cmd);
            pthread_mutex_unlock(&lock);
        }

        usleep(100000);
    }
}

void *receive_thread(void *arg) {
    while (!exit_flag) {
        read_message();
        usleep(100000);
    }
}

int main() {
    pthread_t receive_tid, command_tid;
    pthread_mutex_init(&lock, NULL);

    sock_fd = init_netlink_socket();

    pthread_create(&receive_tid, NULL, receive_thread, NULL);
    pthread_create(&command_tid, NULL, command_thread, NULL);

    pthread_join(receive_tid, NULL);
    pthread_join(command_tid, NULL);

    pthread_mutex_destroy(&lock);
    
    close(sock_fd);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <ctype.h>
#include <time.h>


#define NETLINK_USER 31
#define MAX_PAYLOAD 1024 // maximum payload size

struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int netlink_socket_fd;
struct msghdr msg;


void logger(const char *message) {

    FILE *fp;
    fp = fopen("/var/log/pylrkproxy/user_space.log", "a+");

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char *res = (char *) malloc(sizeof(char) * 256);
    sprintf(res, "%d-%02d-%02d %02d:%02d:%02d %s:%d: %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
            tm.tm_min, tm.tm_sec, __FILE__, __LINE__, message);
    printf("%s", res);
    fputs(res, fp);
    fclose(fp);
}


void error(const char *message) {
    logger(message);
    perror(message);
    exit(1);
}


int main() {
    printf("Starting user_space.c");

    int unix_socket_fd, new_unix_socket_fd;
    socklen_t cli_len;
    char buffer[1024];

    struct sockaddr_un serv_addr, cli_addr;
    int n;

    // Create unix socket
    unix_socket_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (unix_socket_fd < 0) {
        error("Error opening unix socket");
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, "/root/sock");
    serv_addr.sun_path[sizeof(serv_addr.sun_path) - 1] = '\0';


    // Create netlink socket
    netlink_socket_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (netlink_socket_fd < 0) {
        error("Socket can't connected to netlink");
    }

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); // self pid

    // Bind netlink socket
    bind(netlink_socket_fd, (struct sockaddr *) &src_addr, sizeof(src_addr));

    memset(&dest_addr, 0, sizeof(dest_addr));
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; // For Linux Kernel
    dest_addr.nl_groups = 0; // unicast

    nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    // Bind unix socket
    if (bind(unix_socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("Error on binding unix socket");
    }

    listen(unix_socket_fd, 5);

    new_unix_socket_fd = accept(unix_socket_fd, (struct sockaddr *) &cli_addr, &cli_len);

    if (new_unix_socket_fd < 0) {
        error("Error on accept to unix socket");
    }

    bzero(buffer, 1024);


    while (1) {
//        printf("Waiting for send");

        n = read(new_unix_socket_fd, buffer, 1024);
        if (n < 0) {
            logger("Error reading from socket");
            usleep(2000*1000);
            continue;
        }
        // printf("Get message of user_space: %s\n", buffer);
        // fflush(stdout);


        strcpy(NLMSG_DATA(nlh), buffer);

        iov.iov_base = (void *) nlh;
        iov.iov_len = nlh->nlmsg_len;
        msg.msg_name = (void *) &dest_addr;
        msg.msg_namelen = sizeof(dest_addr);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        // printf("Sending message to kernel");
        sendmsg(netlink_socket_fd, &msg, 0);

    }

    printf("Starting close socket");
    close(new_unix_socket_fd);
    printf("Close socket of kernel");
    close(unix_socket_fd);
    printf("Close socket of Unix");
    return 0;
}

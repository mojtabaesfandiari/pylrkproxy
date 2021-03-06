#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/netlink.h>
#include <time.h>



#define NETLINK_USER 31
#define MAX_PAYLOAD 1024 /* maximum payload size*/

struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;


void error(const char *msg) {
    perror(msg);
    exit(1);
}


int random_number(int lower, int upper)
{
    return (rand() % (upper - lower + 1)) + lower;
}


int main() {
    srand(time(0));
    printf("Starting user_space.c\n");


    // Create netlink socket
    sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (sock_fd < 0) {
        printf("Socket can't connected for netlink\n");
        return -1;
    }

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* self pid */;

    // bind socket
    bind(sock_fd, (struct sockaddr *) &src_addr, sizeof(src_addr));

    memset(&dest_addr, 0, sizeof(dest_addr));
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */

    nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;




    while (1) {


        int p1 = random_number(20000, 80000);
        int p2 = random_number(20000, 80000);

        char buffer[1024];
        sprintf(buffer, "1041_2 S 192.168.122.166 192.168.122.103 192.168.122.103 192.168.122.1 4004 %d %d 8000 60 615ced9cffca4c1b8a405d3465a5567d zc5MmIxMGY ", p1, p2);

        printf("Get message of user_space: %s\n", buffer);

        strcpy(NLMSG_DATA(nlh), buffer);

        iov.iov_base = (void *) nlh;
        iov.iov_len = nlh->nlmsg_len;
        msg.msg_name = (void *) &dest_addr;
        msg.msg_namelen = sizeof(dest_addr);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        sendmsg(sock_fd, &msg, 0);
        printf("Sent message to kernel %s\n", buffer);
        usleep(2000*1000);

    }

}

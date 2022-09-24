#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <arpa/inet.h>
#include <time.h>
#include <poll.h>
#include <signal.h>


#define NETLINK_USER 31
#define MAX_PAYLOAD 1024 // maximum payload size

struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int netlink_socket_fd;
struct msghdr msg;


void sockbind(char *sdp_srcip, char *dstport, char *cc) {
    int sockfd;
    char tempbuffer[10];
    struct sockaddr_in servaddr, cliaddr;
    struct pollfd fds[1];
    int timeout = 5000;
    int pret, len;
    char cp[15];

    //int port = 11111;
    //sprintf(cp, "%d", port);
    //memcpy(cc, cp, 15);

    //return "44443";





    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        cc = NULL;
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(dstport));

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *) &servaddr,
             sizeof(servaddr)) < 0) {
        perror("bind failed");
        cc = NULL;
        exit(EXIT_FAILURE);
    }

    fds[0].fd = sockfd;
    fds[0].events = 0;
    fds[0].events |= POLLIN;

    pret = poll(fds, 1, timeout);

    if (pret == 0) {
        perror("nothing get....\n");
        //return cc;
    } else {
//        len = recv(sockfd, tempbuffer, sizeof(tempbuffer), 0);
        int cliaddrlen = sizeof(cliaddr);
        len = recvfrom(sockfd, tempbuffer, sizeof(tempbuffer), 0, (struct sockaddr *) &cliaddr,
                       (socklen_t *) &cliaddrlen);
//        (sockfd, tempbuffer, sizeof(tempbuffer), 0);
    }

    if (len != -1) {
        char *src_ip = inet_ntoa(cliaddr.sin_addr);
        int src_port = ntohs(cliaddr.sin_port);
        //printf("The packet is recv from : %s:%d\n", src_ip, src_port);
        if (!strcmp(sdp_srcip, src_ip)) {
            sprintf(cp, "%d", src_port);
    	    memcpy(cc, cp, 15);
            //dstport = cp;
       }
    } 
    
    close(sockfd);
    //return dstport;
}


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
    char buffer[MAX_PAYLOAD];
    int fd[2];

    struct sockaddr_un serv_addr, cli_addr;
    int n;

    // create pipe descriptors
    if (pipe(fd) < 0) {
        perror("pipe descriptor fail error\n");
        exit(1);
    }

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

    bzero((char *) &buffer, MAX_PAYLOAD);

    // set the SIGCHLD signal handler in the parent process to SIG_IGN to have the kernel automatically reap your children.
    signal(SIGCHLD, SIG_IGN);

    while (1) {
//        printf("Waiting for send");

        n = read(new_unix_socket_fd, buffer, MAX_PAYLOAD);
        if (n < 0) {
            logger("Error reading from socket");
            usleep(2000 * 1000);
            continue;
        }

          printf("Get message of user_space: %s\n", buffer);
         //logger("Get message of user_space: %s\n");
        // fflush(stdout);

        write(fd[1], buffer, MAX_PAYLOAD);

        //contineue following step in fork child process and keep waiting for new data in parent process
        pid_t pid = fork();
        if (pid < 0) {
            //perror("Fork fail error");
            logger("Fork fail error\n");
        }

        //Steps in child process
        if (pid == 0) {
            close(fd[1]);
	    char childbuf[MAX_PAYLOAD];
            bzero((char *) &childbuf, sizeof(childbuf));
            read(fd[0], childbuf, MAX_PAYLOAD);
    	    //logger(childbuf);	    
//            memcpy(childbuf, buffer, MAX_PAYLOAD);

            char newchildbuf[MAX_PAYLOAD];
	    char childtemp[MAX_PAYLOAD];
            
            bzero((char *) &newchildbuf, sizeof(newchildbuf));
            //bzero((char *) &childtemp, sizeof(childtemp));
            memcpy(childtemp, childbuf, MAX_PAYLOAD);

	    char *childbufarry[15];
            int i = 0;
            
            //fill buffer with zero
            char *p = strtok(childtemp, " ");
            while (p != NULL) {
                childbufarry[i++] = p;
                p = strtok(NULL, " ");
            }
		            
            if (!(*childbufarry[1] ^ 'S')) {
                //check symetric nat for clients and update received ports
		char leg1[15];
		char leg2[15];
		memcpy(leg1, childbufarry[6], 15);
		memcpy(leg2, childbufarry[9], 15);
			
                sockbind(childbufarry[2], childbufarry[8], leg1);
                sockbind(childbufarry[5], childbufarry[7], leg2);
		
                if(leg1 == NULL || leg2 == NULL)
                	goto final;
                	
		childbufarry[6] = leg1;
                childbufarry[9] = leg2;

                bzero((char *) &newchildbuf, MAX_PAYLOAD);
                for (int j = 0; j < i; ++j) {
                    strcat(newchildbuf, childbufarry[j]);
                    strcat(newchildbuf, " ");
                }
                //send newbuffer to kernel space
                strcpy(NLMSG_DATA(nlh), newchildbuf);
            } else {
                //send received buffer to kernel space
                strcpy(NLMSG_DATA(nlh), childbuf);
            }

            //close(fd[0]);

            iov.iov_base = (void *) nlh;
            iov.iov_len = nlh->nlmsg_len;
            msg.msg_name = (void *) &dest_addr;
            msg.msg_namelen = sizeof(dest_addr);
            msg.msg_iov = &iov;
            msg.msg_iovlen = 1;

            // printf("Sending message to kernel");
            sendmsg(netlink_socket_fd, &msg, 0);
final:            
            exit(0);

        }
    }
    printf("Starting close socket");
    close(new_unix_socket_fd);
    printf("Close socket of kernel");
    close(unix_socket_fd);
    printf("Close socket of Unix");
    return 0;
}

/*
AAKASH JANA AAJ284 11297343
NANDISH JHA NAJ474 11282001
*/

#include <rtthreads.h>
#include <RttQueue.h>
#include <RttCommon.h>
#include <list.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#define MAXLINE 1024

RttSem in_msg_access_sem, out_msg_access_sem;
RttThreadId keyb, spkr, lstnr, displr;

/* We are going to declare a fixed pool of memory for the LISTs and NODEs */
NODE pool_nodes[MAX_NODES];
LIST pool_lists[MAX_LISTS];
int free_node_head = 0;  /* head of free-node of NODEs */
int free_list_head = 0;  /* head of free-list of LISTs */
int initialized = 0;      /* 0 if uninitialized else 1 */
LIST *outgoing_msgs, *incoming_msgs;

/* Struct to packetize the msg information */
typedef struct {
    uint32_t sec;
    uint32_t usec;
    uint16_t msglen;
    char     msg[MAXLINE];
} ChatMessage;

struct sockaddr_in servaddr, cliaddr;
int sock_fd;
volatile int shutdown_flag = 0;

RTTTHREAD keyboard_input() {
    int fd;
    char buffer[MAXLINE];
    ssize_t n;
    struct timeval tv;
    ChatMessage *chat_msg;
    /* ChatMessage *test_msg; */

    fd = STDIN_FILENO; 
    for (;;) {
        n = read(fd, buffer, MAXLINE);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                RttSleep(1);
            } else {
                perror("read error");
                break;
            }
        } else if (n == 0) {
            break; 
        } else {
            gettimeofday(&tv, NULL);
            chat_msg = malloc(sizeof(ChatMessage));
            chat_msg->sec = (uint32_t)tv.tv_sec;
            chat_msg->usec = (uint32_t)tv.tv_usec;
            chat_msg->msglen = (uint16_t)n;
            memcpy(chat_msg->msg, buffer, n);
            RttP( in_msg_access_sem );
            if (ListAppend(outgoing_msgs, chat_msg) == -1) {
                printf("ListAdd failed, message not saved!\n");
            }
            RttV( in_msg_access_sem );
            buffer[n] = '\0';
            printf("You enterred msg : %s", buffer);
            /*
            ListFirst(outgoing_msgs);
            test_msg = (ChatMessage *)ListRemove(outgoing_msgs);
            printf("Removed msg from list: %s", test_msg->msg);
            */
            /* speaker_thread(sock_fd); */
            if (strncmp(buffer, "exit", 4) == 0) {
                shutdown_flag = 1;
                break; 
            }
        }
    }
}

RTTTHREAD listener_thread() {
    char buffer[MAXLINE], temp_text[MAXLINE+10];
    ssize_t n;
    int flags;
    uint32_t sec, usec;
    uint16_t datalen;
    socklen_t len;
    ChatMessage *chat_msg;

    len = sizeof(cliaddr);
    flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("UDP socket fcntl failed");
        return;
    }
    if (fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("UDP socket fcntl (nonblock) failed");
    }

    for (;;) {
        n = recvfrom(sock_fd,
            buffer,
            MAXLINE,
            0, (struct sockaddr *)&cliaddr,
            &len);

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                RttSleep(1);
            } else {
                perror("recvfrom error");
                continue;
            }
        }
        else {
            if (n < 10) {
                printf("Received packet too small \
(%ld bytes), ignoring\n", (long)n);
                continue;
            }
            memcpy(&sec, buffer, 4);
            memcpy(&usec, buffer + 4, 4);
            memcpy(&datalen, buffer + 8, 2);
            sec = ntohl(sec);
            usec = ntohl(usec);
            datalen = ntohs(datalen);
            if ((ssize_t)(10 + datalen) > n) {
                printf("Received Packet too small \
(%ld bytes), ignoring\n", (long)n);
                continue;
            }
            if (datalen > MAXLINE) {
                datalen = MAXLINE;
            }
            memcpy(temp_text, buffer + 10, datalen);
            temp_text[datalen] = '\0';
            chat_msg = malloc(sizeof(ChatMessage));
            chat_msg->sec = sec;
            chat_msg->usec = usec;
            chat_msg->msglen = datalen;
            memcpy(chat_msg->msg, temp_text, datalen);
            RttP( out_msg_access_sem );
            if (ListAppend(incoming_msgs, chat_msg) == -1) {
                printf("ListAdd failed, message not saved!\n");
            }
            RttV( out_msg_access_sem );
        }
    }
    /* fcntl(sock_fd, F_SETFL, initial_flags); */
}

RTTTHREAD display_thread() {
    ChatMessage *chat_msg;
    for (;;) {
        RttP( out_msg_access_sem );
        if (ListCount(incoming_msgs) == 0) {
            RttV( out_msg_access_sem );
            RttSleep(1);
            continue;
        }
        ListFirst(incoming_msgs);
        chat_msg = (ChatMessage *)ListRemove(incoming_msgs);
        printf("\t\t%u sec : %u usec\n", chat_msg->sec, chat_msg->usec);
        printf("\t\tReceived Msg: %s\n", chat_msg->msg);
        free(chat_msg);
        RttV( out_msg_access_sem );
    }
}

RTTTHREAD speaker_thread() {
    ChatMessage *chat_msg;
    ssize_t n, offset;
    socklen_t len;
    uint32_t sec, usec;
    uint16_t msglen;
    char buffer[4+4+2+MAXLINE];

    len = sizeof(cliaddr);
    for (;;) {
        RttP( in_msg_access_sem );
        if (ListCount(outgoing_msgs) == 0) {
            RttV( in_msg_access_sem );
            RttSleep(1);
            continue;
        }
        ListFirst(outgoing_msgs);
        chat_msg = (ChatMessage *)ListRemove(outgoing_msgs);
        /* unpack the ChatMessage into a char buffer */
        offset = 0;
        sec = htonl(chat_msg->sec);
        usec = htonl(chat_msg->usec);
        msglen = htons(chat_msg->msglen);
        memcpy(buffer + offset, &sec, 4);
        offset += 4;
        memcpy(buffer + offset, &usec, 4);
        offset += 4;
        memcpy(buffer + offset, &msglen, 2);
        offset += 2;
        memcpy(buffer + offset, chat_msg->msg, chat_msg->msglen);
        offset += chat_msg->msglen;
        RttV( in_msg_access_sem );
        n = sendto(sock_fd,
            buffer,
            offset,
            0,
            (struct sockaddr *)&cliaddr,
            len);
        if (n < 0) {
            perror("sendto error");
        } else {
            printf("Sent msg: %s", chat_msg->msg);
        }
        free(chat_msg);
    }
}

int mainp(int argc, char *argv[]) {
    int fd, flags, initial_flags;
    uint16_t myPort, peerPort;
    char *peerHost;
    struct addrinfo host_ip, *res;
    RttSchAttr attrs;


    /* Set the scheduling attributes for the threads */
    attrs.startingtime = RTTZEROTIME;
    attrs.priority = RTTNORM;
    attrs.deadline = RTTNODEADLINE;

    /* Creating message semaphores */
    if (RttAllocSem(&in_msg_access_sem, 1, RTTFCFS) != RTTOK) {
        printf("Semaphore (in_msg_access_sem) creation failed!\n");
        return 0;
    }
    if (RttAllocSem(&out_msg_access_sem, 1, RTTFCFS) != RTTOK) {
        printf("Semaphore (out_msg_access_sem) creation failed!\n");
        return 0;
    }

    outgoing_msgs = ListCreate();
    incoming_msgs = ListCreate();

    if (argc != 4) {
        printf("Usage: %s <myPort> <peerHost> <peerPort>\n", argv[0]);
        return 1;
    }

    myPort = (uint16_t)atoi(argv[1]);
    peerHost = argv[2];
    peerPort = (uint16_t)atoi(argv[3]);

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed!");
        return 1;
    }
    
    /* Server Socket connection logic */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(myPort);

    if (bind(sock_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed!");
        close(sock_fd);
        return 1;
    }

    /* Client Socket connection logic */
    memset(&cliaddr, 0, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(peerPort);

    memset(&host_ip, 0, sizeof(host_ip));
    host_ip.ai_family = AF_INET;
    host_ip.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(peerHost, NULL, &host_ip, &res) != 0) {
        printf("getaddrinfo error for host: %s\n", peerHost);
        perror("getaddrinfo failed!");
        close(sock_fd);
        return 1;
    }
    cliaddr.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    freeaddrinfo(res);

    /* Set stdin to non-blocking mode */
    fd = STDIN_FILENO;
    flags = fcntl(fd, F_GETFL, 0);
    initial_flags = flags;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    /* Start listener and keyboard input threads */
    RttCreate(&lstnr, listener_thread,32000,"listener", NULL, attrs, RTTUSR);
    RttCreate(&keyb, keyboard_input,32000,"keyboard",NULL, attrs, RTTUSR);
    RttCreate(&spkr, speaker_thread,32000,"speaker",NULL, attrs, RTTUSR);
    RttCreate(&displr, display_thread,32000,"displayer",NULL, attrs, RTTUSR);
    printf("s-chat: All threads created\n");
    for (;;) {
         RttSleep(2);
         if (shutdown_flag) {
            break;
         } 
    }
    printf("s-chat: Shutting down...\n");
    RttKill(lstnr);
    RttKill(keyb);
    RttKill(spkr);
    RttKill(displr);
    close(sock_fd);
    /* The line below resets the flags of the file descriptor  */
    fcntl(fd, F_SETFL, initial_flags);

    return 0;
}
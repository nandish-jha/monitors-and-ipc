/*
AAKASH JANA AAJ284 11297343
NANDISH JHA NAJ474 11282001
*/

#include <rtthreads.h>
#include <RttQueue.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define MAXLINE 1024

/* Struct to packetize the msg information */
typedef struct {
    uint32_t sec;
    uint32_t usec;
    uint16_t msglen;
    char     msg[MAXLINE];
} ChatMessage;

/* Semaphore declaration for print and quit chat */
static RttSem printSem, quitSem; 
static char peerName[128];
static int sockfd = -1;
static struct sockaddr_in peerAddr;

/* Thread IDs */
static RttThreadId tx, rx;

void printer(const char *msg) {
    RttP(printSem);
    fputs(msg, stdout);
    fflush(stdout);
    RttV(printSem);
}

/* RTT thread function signatures */
RTTTHREAD sender_thread(void *arg) {
    char readBuffer[MAXLINE + 4]; 
    char *newLine; size_t pktLen; 
    int msgSent, textRead, lineLen; 
    ChatMessage quitMsg, msg; 
    RttTimeValue tv; 
    for (;;) { 
        /* Read a line from stdin using RT Threads I/O 
        (maps to non-global-blocking read) */ 
        printf("You: "); 
        textRead = RttRead(0, readBuffer, MAXLINE); 
        if (textRead <= 0) continue; 
        /* Find the newline boundary (treat as one message) */ 
        printf("Read message : %s", readBuffer);
        newLine = memchr(readBuffer, '\n', textRead); 
        lineLen = newLine ? (int)(newLine - readBuffer + 1) : textRead; 
        /* Check for quit command (a line with only a period) */ 
        if (lineLen > MAXLINE) lineLen = MAXLINE;
        if (strcmp(readBuffer,"quit\n") == 0 || \
         strcmp(readBuffer,"QUIT\n") == 0){ 
            memset(&quitMsg, 0, sizeof(quitMsg)); 
            quitMsg.sec = htonl(0); 
            quitMsg.usec = htonl(0); 
            quitMsg.msglen = htons(0); 
            RttSendto(sockfd, (char *) &quitMsg, sizeof(uint32_t)*2 + \
            sizeof(uint16_t), 0, (struct sockaddr*)&peerAddr, \
            sizeof(peerAddr)); 
            printer("[local] quitting...\n"); RttV(quitSem); 
            /* RttExit(); */
            return; 
        } /* Timestamp (binary) */ 
            
        RttGetTimeOfDay(&tv); 
        msg.sec = htonl((uint32_t)tv.seconds); 
        msg.usec = htonl((uint32_t)tv.microseconds); 
        msg.msglen = htons((uint16_t)lineLen); 
        memcpy(msg.msg, readBuffer, lineLen); 
        pktLen = sizeof(msg.sec) + sizeof(msg.usec) + sizeof(msg.msglen) + lineLen; 
        msgSent = RttSendto(sockfd, (char *) &msg, pktLen, 0,
            (struct sockaddr*)&peerAddr, sizeof(peerAddr)); 
        if (msgSent < 0) { 
            printer("[error] Send function failed!\n"); 
        } 
        else { 
            printf("Sent %d bytes\n", msgSent); 
        } 
    } 
}

RTTTHREAD receiver_thread(void *arg)
{
    ChatMessage msg;
    struct sockaddr_in from;
    socklen_t fromlen;
    int n;
    uint32_t sec;
    uint32_t usec;   
    uint16_t msglen;
    char line[MAXLINE+1];

    for (;;) {
        fromlen = sizeof(from);

        n = RttRecvfrom(sockfd, (char*)&msg, sizeof(msg), 0,
                            (struct sockaddr*)&from, &fromlen);
        if (n < (int)(sizeof(uint32_t)*2 + sizeof(uint16_t))) RttExit();

        sec    = ntohl(msg.sec);
        usec   = ntohl(msg.usec);
        msglen = ntohs(msg.msglen);
        if (msglen > MAXLINE) msglen = MAXLINE;
        if ((int)(sizeof(uint32_t)*2 + sizeof(uint16_t) + msglen) > n) RttExit();

        memcpy(line, msg.msg, msglen);
        line[msglen] = '\0';

        printf("%s: %s [t=%u.%02u]\n", peerName, line, sec, usec);
        if (strcmp(msg.msg,"/quit") == 0)
        {
            RttExit();
        }
    }
}


int mainp(int argc, char *argv[]) {

    uint16_t myPort, peerPort;
    char *peerHost;
    struct sockaddr_in userAddr;
    RttSchAttr attrs;
    struct addrinfo recipient, *res = NULL;
    char ipstr[INET_ADDRSTRLEN];
    int rv;

    /* Set the scheduling attributes for the threads */
    attrs.startingtime = RTTZEROTIME;
    attrs.priority = RTTNORM;
    attrs.deadline = RTTNODEADLINE;

    if (argc != 4) {
        printf("Usage: %s <myPort> <peerHost> <peerPort>\n", argv[0]);
        return 1;
    }

    setvbuf(stdout, NULL, _IONBF, 0);
    myPort = (uint16_t)atoi(argv[1]);
    peerHost = argv[2];
    peerPort = (uint16_t)atoi(argv[3]);

    RttNetInit(0, (unsigned int)myPort);

    printf("s-chat: Starting chat on port %d with peer %s:%d\n", 
        myPort, peerHost, peerPort);
    strncpy(peerName, peerHost, sizeof(peerName)-1);
    
    sockfd = RttSocket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("s-chat: Socket creation failed\n");
        return 1;
    }
    else {
        printf("s-chat: Socket created successfully\n");
    }

    /* Binding to socket procedure */
    memset(&userAddr, 0, sizeof(userAddr));
    userAddr.sin_family = AF_INET;
    userAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    userAddr.sin_port = htons(myPort);

    if (RttBind(sockfd, (struct sockaddr *)&userAddr, sizeof(userAddr)) < 0) {
        printf("s-chat: Socket bind failed\n");
        RttClose(sockfd);
        return 1;
    }
    else {
        printf("s-chat: Socket bind successful\n");
    }

    /* Initialize the semaphores */
    /* RttAllocSem(&printSem, 1, RTTFCFS); */
    /* RttAllocSem(&quitSem, 0, RTTFCFS); */

    /* resolve the peer : peerHost, peerPort, peerAddr */
    memset(&peerAddr, 0, sizeof(peerAddr));
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port   = htons(peerPort);

    memset(&recipient, 0, sizeof(recipient));
    recipient.ai_family   = AF_INET;      
    recipient.ai_socktype = SOCK_DGRAM;

    rv = getaddrinfo(peerHost, NULL, &recipient, &res);
    if (rv != 0 || !res) {
        printf("s-chat: getaddrinfo failed for '%s': %s\n",
            peerHost, gai_strerror(rv));
        RttClose(sockfd);
        return 1;
    }
    /* Copy resolved IPv4 into peerAddr, but keep our port */
    peerAddr.sin_addr =
        ((struct sockaddr_in*)res->ai_addr)->sin_addr;
    freeaddrinfo(res);

    if (inet_ntop(AF_INET, &peerAddr.sin_addr, ipstr, sizeof ipstr)) {
        printf("s-chat: peer %s resolved to %s:%d\n", peerHost, ipstr, peerPort);
    }

    /* Create the message threads */
    RttCreate(&tx, sender_thread, 32000, "sender", NULL, attrs, RTTUSR);
    RttCreate(&rx, receiver_thread, 32000, "receiver", NULL, attrs, RTTUSR);
    

    printf("s-chat: All threads created\n");
    for (;;) { RttSleep(200000); } 


    /* Wait for quit signal from either thread */
    RttClose(sockfd);
    /* RttFreeSem(printSem); */
    /* RttFreeSem(quitSem); */

    return 0;
}
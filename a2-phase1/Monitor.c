/*
AAKASH JANA AAJ284 11297343
NANDISH JHA NAJ474 11282001
*/

#include <os.h>
#include <stdio.h>
#include <Monitor.h>
#include <list.h>

#define MAX_CONDS 8

typedef struct {
    int type;
    int arg;
} MonMsg;

static PID MonServer_pid = -1;

/* Server function for the monitor */
PROCESS MonServer(void *arg) {
    while (1) {
        PID sender_pid;
        int msglen;
        int reply;

        MonMsg *msg = (MonMsg *)Receive(&sender_pid, &msglen);
        if (!msg) continue;

        switch (msg->type) {
            case 1: /* MON ENTER */
                printf("[MonitorServer] Received: MonEnter\n");
                break;
            case 2: /* MON LEAVE */
                printf("[MonitorServer] Received: MonLeave\n");
                break;
            case 3: /* MON WAIT */
                printf("[MonitorServer] Received: MonWait(%d)\n", msg->arg);
                break;
            case 4: /* MON SIGNAL */
                printf("[MonitorServer] Received: MonSignal(%d)\n", msg->arg);
                break;
            case 5: /* MON INIT */
                printf("[MonitorServer] Received: MonInit(%d)\n", msg->arg);
                break;
            default:
                printf("[MonitorServer] Received: Unknown message\n");
        }
        reply = 0;
        Reply(sender_pid, &reply, sizeof(reply));
    }
}

void MonInit(int numConds) {
    int msglen;
    MonMsg msg;

    if (MonServer_pid == -1) {
        MonServer_pid = Create(
            MonServer,
            4096,
            "MonitorServer",
            NULL,
            NORM,
            SYS
        );
        /* 4096 is stack size */
    }
    /* (You will send a MON_INIT message in the next step) */
    printf("   MonInit called with %d condition variables\n", numConds);

    msg.type = 5;
    msg.arg = numConds;
    msglen = sizeof(MonMsg);

    Send(MonServer_pid, &msg, &msglen);
}

void MonEnter(void) {
    int msglen;
    MonMsg msg;
    
    printf("   MonEnter called\n");

    msg.type = 1;
    msg.arg = 0;
    msglen = sizeof(MonMsg);

    Send(MonServer_pid, &msg, &msglen);
}

void MonLeave(void) {
    int msglen;
    MonMsg msg;
    
    printf("   MonLeave called\n");

    msg.type = 2;
    msg.arg = 0;
    msglen = sizeof(MonMsg);

    Send(MonServer_pid, &msg, &msglen);
}

void MonWait(int cond) {
    int msglen;
    MonMsg msg;
    
    printf("   MonWait called on condition %d\n", cond);

    msg.type = 3;
    msg.arg = cond;
    msglen = sizeof(MonMsg);

    Send(MonServer_pid, &msg, &msglen);
}

void MonSignal(int cond) {
    int msglen;
    MonMsg msg;
    
    printf("   MonSignal called on condition %d\n", cond);

    msg.type = 4;
    msg.arg = cond;
    msglen = sizeof(MonMsg);

    Send(MonServer_pid, &msg, &msglen);
}
/*
AAKASH JANA AAJ284 11297343
NANDISH JHA NAJ474 11282001
*/

#include <os.h>
#include <stdio.h>
#include <Monitor.h>
#include <list.h>

/* Global variable definitions */
NODE pool_nodes[MAX_NODES];
LIST pool_lists[MAX_LISTS];
int free_node_head = 0;
int free_list_head = 0;
int initialized = 0;

#define MAX_CONDS 8

typedef struct {
    int type;
    int arg;
} MonMsg;

/* Monitor state variables */
static int monitor_locked = 0; /* Is monitor currently locked? */
static PID current_owner = -1; /* PID of process currently in monitor */
static LIST *entry_queue = NULL; /* Queue of processes waiting to enter */
static LIST *urgent_queue = NULL; /* Queue of processes that called signal */
static LIST *condition_queues[MAX_CONDS]; /* Queues for condition variables */
static int num_conditions = 0; /* Number of condition variables */

static PID MonServer_pid = -1;

/* Server function for the monitor */
PROCESS MonServer(void *arg) {
    PID sender_pid;
    int msglen;
    int reply;
    PID *waiting_pid;
    PID next_pid, wakeup_pid;
    int i;
    
    while (1) {
        MonMsg *msg = (MonMsg *)Receive(&sender_pid, &msglen);
        if (!msg) continue;

        switch (msg->type) {
            case 1: /* MON ENTER */
                printf("[MonServer] MonEnter from PID %ld\n", sender_pid);
                if (monitor_locked == 0) {
                    /* Monitor is free, grant access */
                    monitor_locked = 1;
                    current_owner = sender_pid;
                    reply = 0; /* Success */
                    Reply(sender_pid, &reply, sizeof(reply));
                    printf("[MonServer] Granted monitor access to PID %ld\n", \
                        sender_pid);
                } else {
                    /* Monitor is busy, add to entry queue */
                    waiting_pid = (PID *)malloc(sizeof(PID));
                    *waiting_pid = sender_pid;
                    ListAppend(entry_queue, waiting_pid);
                    printf("[MonServer] PID %ld waiting to enter (queue \
                        size: %d)\n", sender_pid, ListCount(entry_queue));
                    /* Don't Reply yet - process will wait */
                }
                break;
                
            case 2: /* MON LEAVE */
                printf("[MonServer] MonLeave from PID %ld\n", sender_pid);
                if (current_owner == sender_pid) {
                    monitor_locked = 0;
                    current_owner = -1;
                    reply = 0;
                    Reply(sender_pid, &reply, sizeof(reply));
                    
                    /* Hoare semantics: urgent queue has
                    priority over entry queue */
                    if (ListCount(urgent_queue) > 0) {
                        waiting_pid = (PID *)ListFirst(urgent_queue);
                        next_pid = *waiting_pid;
                        /* Remove current (first) item */
                        free(ListRemove(urgent_queue));
                        
                        /* Grant monitor to process from urgent queue */
                        monitor_locked = 1;
                        current_owner = next_pid;
                        reply = 0;
                        Reply(next_pid, &reply, sizeof(reply));
                        printf("[MonServer] Granted monitor to \
                            urgent PID %ld\n", next_pid);
                    } else if (ListCount(entry_queue) > 0) {
                        waiting_pid = (PID *)ListFirst(entry_queue);
                        next_pid = *waiting_pid;
                        /* Remove current (first) item */
                        free(ListRemove(entry_queue));
                        
                        /* Grant monitor to next process */
                        monitor_locked = 1;
                        current_owner = next_pid;
                        reply = 0;
                        Reply(next_pid, &reply, sizeof(reply));
                        printf("[MonServer] Granted monitor to \
                            waiting PID %ld\n", next_pid);
                    }
                    printf("[MonServer] Monitor released by \
                        PID %ld\n", sender_pid);
                } else {
                    printf("[MonServer] ERROR: PID %ld tried to leave but \
                        doesn't own monitor\n", sender_pid);
                    reply = -1;
                    Reply(sender_pid, &reply, sizeof(reply));
                }
                break;
                
            case 3: /* MON WAIT */
                printf("[MonServer] MonWait(%d) from \
                    PID %ld\n", msg->arg, sender_pid);
                if (current_owner == sender_pid && msg->arg >= 0 && \
                    msg->arg < num_conditions) {
                    /* Add to condition queue */
                    waiting_pid = (PID *)malloc(sizeof(PID));
                    *waiting_pid = sender_pid;
                    ListAppend(condition_queues[msg->arg], waiting_pid);
                    
                    /* Release monitor */
                    monitor_locked = 0;
                    current_owner = -1;
                    
                    /* Hoare semantics: urgent queue has
                    priority over entry queue */
                    if (ListCount(urgent_queue) > 0) {
                        PID *next_waiting = (PID *)ListFirst(urgent_queue);
                        next_pid = *next_waiting;
                        /* Remove current (first) item */
                        free(ListRemove(urgent_queue));
                        
                        monitor_locked = 1;
                        current_owner = next_pid;
                        reply = 0;
                        Reply(next_pid, &reply, sizeof(reply));
                        printf("[MonServer] Granted monitor to \
                            urgent PID %ld after wait\n", next_pid);
                    } else if (ListCount(entry_queue) > 0) {
                        PID *next_waiting = (PID *)ListFirst(entry_queue);
                        next_pid = *next_waiting;
                        /* Remove current (first) item */
                        free(ListRemove(entry_queue));
                        
                        monitor_locked = 1;
                        current_owner = next_pid;
                        reply = 0;
                        Reply(next_pid, &reply, sizeof(reply));
                        printf("[MonServer] Granted monitor to \
                            PID %ld after wait\n", next_pid);
                    }
                    
                    printf("[MonServer] PID %ld waiting \
                        on condition %d\n", sender_pid, msg->arg);
                    /* Don't Reply - process waits on condition */
                } else {
                    printf("[MonServer] ERROR: Invalid wait \
                        from PID %ld\n", sender_pid);
                    reply = -1;
                    Reply(sender_pid, &reply, sizeof(reply));
                }
                break;
                
            case 4: /* MON SIGNAL */
                printf("[MonServer] MonSignal(%d) from \
                    PID %ld\n", msg->arg, sender_pid);
                if (current_owner == sender_pid && \
                    msg->arg >= 0 && msg->arg < num_conditions) {
                    /* Wake up one process waiting on this condition */
                    if (ListCount(condition_queues[msg->arg]) > 0) {
                        waiting_pid = (PID *)ListFirst(\
                            condition_queues[msg->arg]);
                        wakeup_pid = *waiting_pid;
                        /* Remove current (first) item */
                        free(ListRemove(condition_queues[msg->arg]));
                        
                        /* Hoare semantics: signaling
                        process goes to urgent queue */
                        waiting_pid = (PID *)malloc(sizeof(PID));
                        *waiting_pid = sender_pid;
                        ListAppend(urgent_queue, waiting_pid);
                        
                        /* Give monitor control to the signaled process */
                        current_owner = wakeup_pid;
                        printf("[MonServer] Signaled PID %ld gets \
                            control, signaler %ld goes to urgent \
                            queue\n", wakeup_pid, sender_pid);
                        
                        /* Reply to the signaled process first */
                        reply = 0;
                        Reply(wakeup_pid, &reply, sizeof(reply));
                        
                        /* Don't reply to signaler yet -
                        they're blocked in urgent queue */
                    } else {
                        printf("[MonServer] Signal on condition %d, \
                            but no waiting processes\n", msg->arg);
                        reply = 0;
                        Reply(sender_pid, &reply, sizeof(reply));
                    }
                } else {
                    printf("[MonServer] ERROR: Invalid signal \
                        from PID %ld\n", sender_pid);
                    reply = -1;
                    Reply(sender_pid, &reply, sizeof(reply));
                }
                break;
                
            case 5: /* MON INIT */
                printf("[MonServer] MonInit(%d) from \
                    PID %ld\n", msg->arg, sender_pid);
                num_conditions = (msg->arg > MAX_CONDS) ? MAX_CONDS : msg->arg;
                
                /* Initialize lists */
                if (entry_queue == NULL) {
                    entry_queue = ListCreate();
                }
                if (urgent_queue == NULL) {
                    urgent_queue = ListCreate();
                }
                for (i = 0; i < num_conditions; i++) {
                    if (condition_queues[i] == NULL) {
                        condition_queues[i] = ListCreate();
                    }
                }
                
                reply = 0;
                Reply(sender_pid, &reply, sizeof(reply));
                printf("[MonServer] Initialized with %d \
                    condition variables\n", num_conditions);
                break;
                
            default:
                printf("[MonServer] Unknown message type %d \
                    from PID %ld\n", msg->type, sender_pid);
                reply = -1;
                Reply(sender_pid, &reply, sizeof(reply));
        }
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
        if (MonServer_pid == -1) {
            printf("MonInit: Failed to create Monitor Server\n");
            return;
        }
        printf("MonInit: Monitor Server created with PID %ld\n", MonServer_pid);
    }
    /* (You will send a MON_INIT message in the next step) */
    printf("   MonInit called with %d condition variables\n", numConds);

    msg.type = 5;
    msg.arg = numConds;
    msglen = sizeof(MonMsg);

    if (Send(MonServer_pid, &msg, &msglen) == NULL) {
        printf("MonInit: Send failed\n");
        return;
    }
}

void MonEnter(void) {
    int msglen;
    MonMsg msg;
    
    printf("   MonEnter called\n");

    msg.type = 1;
    msg.arg = 0;
    msglen = sizeof(MonMsg);

    if (Send(MonServer_pid, &msg, &msglen) == NULL) {
        printf("MonEnter: Send failed\n");
        return;
    }
}

void MonLeave(void) {
    int msglen;
    MonMsg msg;
    
    printf("   MonLeave called\n");

    msg.type = 2;
    msg.arg = 0;
    msglen = sizeof(MonMsg);

    if (Send(MonServer_pid, &msg, &msglen) == NULL) {
        printf("MonLeave: Send failed\n");
        return;
    }
}

void MonWait(int cond) {
    int msglen;
    MonMsg msg;
    
    printf("   MonWait called on condition %d\n", cond);

    msg.type = 3;
    msg.arg = cond;
    msglen = sizeof(MonMsg);

    if (Send(MonServer_pid, &msg, &msglen) == NULL) {
        printf("MonWait: Send failed\n");
        return;
    }
}

void MonSignal(int cond) {
    int msglen;
    MonMsg msg;
    
    printf("   MonSignal called on condition %d\n", cond);

    msg.type = 4;
    msg.arg = cond;
    msglen = sizeof(MonMsg);

    if (Send(MonServer_pid, &msg, &msglen) == NULL) {
        printf("MonSignal: Send failed\n");
        return;
    }
}
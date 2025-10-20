/*
AAKASH JANA AAJ284 11297343
NANDISH JHA NAJ474 11282001
*/

#ifndef MONITOR_H
#define MONITOR_H
#include <list.h>

/* Global variables for list management - extern declarations */
extern NODE pool_nodes[MAX_NODES];
extern LIST pool_lists[MAX_LISTS];
extern int free_node_head;
extern int free_list_head;
extern int initialized;

void MonInit(int numConds);
void MonEnter(void);
void MonLeave(void);
void MonWait(int cond);
void MonSignal(int cond);

#endif
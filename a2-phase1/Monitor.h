/*
AAKASH JANA AAJ284 11297343
NANDISH JHA NAJ474 11282001
*/

#ifndef MONITOR_H
#define MONITOR_H

void MonInit(int numConds);
void MonEnter(void);
void MonLeave(void);
void MonWait(int cond);
void MonSignal(int cond);

#endif
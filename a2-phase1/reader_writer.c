/*
AAKASH JANA AAJ284 11297343
NANDISH JHA NAJ474 11282001
*/

/*# Dwight Makaroff   */
/*# student number    */
/*# nsid              */
/*# November 7, 2008  */
/* modified to use RT-Threads October 2018 */
/* modified back for 2019 */
/* No part of this program may be used for any profit or example for any
   purpose other than for help in A2 for CMPT 332 at the University
   of Saskatchewan in the fall term of 2019-20 without the expressed
   written consent of the author.

   *********** reader-writer.c ***********
*/


#include <stdio.h>

#include <os.h>
#include <standards.h>

#include <reader_writer_monitor.h>

#define SLEEPMAX 20

PROCESS reader(void *arg)
{
  long myId;
  
  myId = (long) arg;
  
  /*for(;;)
    {*/
  printf("%ld start read\n", myId);
  StartRead();
  printf("%ld Reading\n", myId);
  Sleep((int) (rand() % SLEEPMAX));
  StopRead();
  printf("%ld stop read\n", myId);
  Sleep((int) (rand() % SLEEPMAX));
    /*}*/
}

PROCESS writer(void *arg)
{
  long myId;
  myId = (long) arg;
  
  /*for(;;)
    {*/
  printf("%ld start write\n", myId);
  StartWrite();
  printf("%ld writing\n", myId);
  Sleep((int) (rand() % SLEEPMAX*5));
  StopWrite();
  printf("%ld stop write\n", myId);
  Sleep((int) (rand() % SLEEPMAX*6));
    /*}*/  
}

int mainp()
{
    PID tempPid, temp2, temp3;
    setbuf(stdout, 0);

    srand(71);

    tempPid = Create((void(*)()) reader, 16000, "R1", (void *) 1000, 
		      NORM, USR );
    if (tempPid == PNUL) perror("Create");
    temp2 = Create(  (void(*)()) writer, 16000, "W1", (void *) 500, 
		       NORM, USR );
    if (temp2 == PNUL) perror("Create");
    temp3 = Create(  (void(*)()) reader, 16000, "R2", (void *) 1001,
		       NORM, USR );
    if (temp3 == PNUL) perror("Create");
    temp3 = Create(  (void(*)()) reader, 16000, "R3", (void *) 1002, 
		       NORM, USR );
    if (temp3 == PNUL) perror("Create");
    temp3 = Create(  (void(*)()) writer, 16000, "W2", (void *) 501, 
		       NORM, USR );
    if (temp3 == PNUL) perror("Create");
    printf("Reader and Writer processes created\n");
    
    return(0);
}

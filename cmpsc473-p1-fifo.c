
/**********************************************************************

   File          : cmpsc473-p1-fifo.c

   Description   : This is FIFO page replacement algorithm

   Last Modified : Jan 11 09:54:33 EST 2023
   By            : Trent Jaeger

***********************************************************************/
/**********************************************************************
Copyright (c) 2023 The Pennsylvania State University
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of The Pennsylvania State University nor the names of its contributors may be used to endorse or promote products derived from this softwiare without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

/* Include Files */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <pthread.h>
#include <sched.h>

/* Project Include Files */
#include "cmpsc473-p1.h"

/* Definitions */

/* fifo list */

typedef struct fifo_entry {  
  unsigned int pid;
  ptentry_t *ptentry;
  struct fifo_entry *next;
} fifo_entry_t;

typedef struct fifo {
  fifo_entry_t *first;
  fifo_entry_t *last;
} fifo_t;

fifo_t *frame_list;

/**********************************************************************

    Function    : init_fifo
    Description : initialize fifo list
    Inputs      : fp - input file of data
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int init_fifo( FILE *fp )
{
  printf("initiate fifo...\n");
  frame_list = (fifo_t *)malloc(sizeof(fifo_t));
  frame_list->first = NULL;
  frame_list->last = NULL;
  return 0;
}


/**********************************************************************

    Function    : replace_fifo
    Description : choose victim from fifo list (first in list is oldest)
    Inputs      : victim - process id of victim frame 
                  frame - frame assigned from fifo replacement
                  ptentry - pt entry mapping frame currently -- to be invalidated
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int replace_fifo( unsigned int *victim, frame_t **frame, ptentry_t **ptentry )
{
  fifo_entry_t *first = frame_list->first;

  /* return info on victim */
  *victim = first->pid;
  *ptentry = first->ptentry;
  unsigned int fnum = first->ptentry->frame;
  *frame = &physical_mem[fnum];

  /* remove from list */
  frame_list->first = first->next;
  free( first );

  return 0;
}


/**********************************************************************

    Function    : update_fifo
    Description : update fifo list on allocation (add entry to end)
    Inputs      : pid - process id
                  ptentry - mapped to frame
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int update_fifo( unsigned int pid, ptentry_t *ptentry)
{
  /* make new list entry */
  //printf("using fifo update...\n");
  fifo_entry_t *list_entry = ( fifo_entry_t *)malloc(sizeof(fifo_entry_t));
  list_entry->pid = pid;
  list_entry->ptentry = ptentry;
  list_entry->next = NULL;
  //printf("so far so good...\n");
  /* put it at the end of the list (beginning if null) */
  if ( frame_list->first == NULL ) {
    //printf("empty frame list...\n");
    frame_list->first = list_entry;
    frame_list->last = list_entry;
  }
  /* or really at end */
  else {
    //printf("normal at end...\n");
    frame_list->last->next = list_entry;
    frame_list->last = list_entry;
  }
  //printf("update finished...\n");

  fifo_entry_t *curr = frame_list->first;
  while (TRUE) {
    curr = curr->next;
    if (curr == NULL)
      break;
  }

  return 0;  
}




/**********************************************************************

   File          : cmpsc473-p1-lru.c

   Description   : This is LRU page replacement algorithm

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

/* lru list */

typedef struct lru_entry {  
  unsigned int pid;
  ptentry_t *ptentry;
  struct lru_entry *next;
  struct lru_entry *prev;
} lru_entry_t;

typedef struct lru {
  lru_entry_t *first;
} lru_t;

lru_t *frame_list;

/**********************************************************************

    Function    : init_lru
    Description : initialize lru list
    Inputs      : fp - input file of data
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int init_lru( FILE *fp )
{
  printf("initiate lru...\n");
  frame_list = (lru_t *)malloc(sizeof(lru_t));
  frame_list->first = NULL;
  return 0;
}


/**********************************************************************

    Function    : replace_lru
    Description : choose victim from lru list (first in list is oldest)
    Inputs      : victim - process id of victim frame 
                  frame - frame assigned from lru replacement
                  ptentry - pt entry mapping frame currently -- to be invalidated
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int replace_lru( unsigned int *victim, frame_t **frame, ptentry_t **ptentry )
{
  /* Task 3(b) */
  lru_entry_t *current_entry = frame_list->first;
  unsigned int fnum = current_entry->ptentry->frame;
  lru_entry_t *min_entry = frame_list->first;
  frame_t *current_frame = &physical_mem[fnum];
  frame_t *min_frame = current_frame;
  while ( current_entry->next != frame_list->first ) {
    // age
    current_frame->lru = current_frame->lru >> 1;
    if ( current_entry->ptentry->ref == 1 ) {
      current_frame->lru = current_frame->lru | 4;
      current_entry->ptentry->ref = 0;
    }

    // find minimum
    if ( current_frame->lru < min_frame->lru ) {
      min_frame = current_frame;
      min_entry = current_entry;
    }

    // advance
    fnum ++;
    current_entry = current_entry->next;
    current_frame = &physical_mem[fnum];

  }

  // evict
  frame_list->first = min_entry->next;
  min_entry->prev->next = min_entry->next;
  min_entry->next->prev = min_entry->prev;

  // set return values
  *victim = min_entry->pid;
  *ptentry = min_entry->ptentry;
  *frame = min_frame;

  free( current_entry );
  return 0;
}


/**********************************************************************

    Function    : update_lru
    Description : update lru list on allocation (add entry to end)
    Inputs      : pid - process id
                  ptentry - mapped to frame
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int update_lru( unsigned int pid, ptentry_t *ptentry )
{
  /* Task 3(b) */
  lru_entry_t *list_entry = ( lru_entry_t *)malloc(sizeof(lru_entry_t) );
  list_entry->pid = pid;
  list_entry->ptentry = ptentry;
  list_entry->next = list_entry;
  list_entry->prev = list_entry;

  if ( frame_list->first == NULL ) {
    frame_list->first = list_entry;
  }
  else {
    lru_entry_t *end = frame_list->first->prev;
    end->next = list_entry;
    list_entry->prev = end;
    list_entry->next = frame_list->first;
    frame_list->first->prev = list_entry;
  }
  list_entry->ptentry->ref = 0;
  
  return 0;
}
/**********************************************************************

   File          : cmpsc473-p1.c

   Description   : This is the main file for page replacement project

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
#define USAGE "cmpsc473-p1 <input.file> <output.file> <replacement.mech>"

/* need a store for all processes */
task_t processes[MAX_PROCESSES];

/* physical memory representation */
frame_t physical_mem[PHYSICAL_FRAMES];

/* tlb */
tlb_t tlb[TLB_ENTRIES];
unsigned int tlb_evict_index = 0;

/* current pagetable */
pdentry_t *current_pt;
int current_pid = 0;

/* overall stats */
int swaps = 0;             /* swaps to disk */
int invalidates = 0;       /* reassign page w/o swap */
int pfs = 0;               /* all page faults */
int memory_accesses = 0;   /* accesses that miss TLB but hit memory */
int total_accesses = 0;    /* all accesses */

/* output file */
FILE *out;                 /* description for writing results */


/* page replacement algorithms */
int (*pt_replace_init[])( FILE *fp ) = { init_fifo,
					 init_second,
					 init_lru
};

int (*pt_choose_victim[])( unsigned int *victim, frame_t **frame, ptentry_t **ptentry ) = { replace_fifo,
											    replace_second,
											    replace_lru
};

/* page replacement -- update state at allocation time */
int (*pt_update_replacement[])( unsigned int pid, ptentry_t *ptentry ) = { update_fifo,
									   update_second,
									   update_lru
};


/**********************************************************************

    Function    : main
    Description : this is the main function for project #1
    Inputs      : argc - number of command line parameters
                  argv - the text of the arguments
    Outputs     : 0 if successful, -1 if failure

***********************************************************************/

/* Functions */
int main( int argc, char **argv ) 
{
  FILE *in;
    // unsigned int op;  /* read (OP_READ) or write (OP_WRITE) */

    /* Check for arguments */
    if ( argc < 4 ) 
    {
        /* Complain, explain, and exit */
        fprintf( stderr, "error: missing or bad command line arguments\n" );
        fprintf( stderr, USAGE );
        exit( -1 );
    }

    /* open the input file and return the file descriptor */
    if (!( in = fopen( argv[1], "r" ))) {
      fprintf( stderr, "error: input file open failure\n" );
      return -1;
    }

    /* open the output file and return the file descriptor */
    if (!( out = fopen( argv[2], "w+" ))) {
      fprintf( stderr, "error: write output info\n" );
      return -1;
    }

    /* Initialization */
    /* for example: build optimal list */
    system_init( in, atoi(argv[3]) );

    fprintf( out, "++++++++++++++++++++ Start Execution ++++++++++++++++++\n" );

    command_loop( in, atoi(argv[3]) );
    
    /* close the input file */
    fclose( in );
    
    write_results( out );
    fclose( out );
    
    exit( 0 );
}

/**********************************************************************

    Function    : write_results
    Description : Write the working set history and memory access performance
    Inputs      : out - file pointer of output file (global)
    Outputs     : 0 if successful, <0 otherwise

***********************************************************************/

int write_results( )
{
  float tlb_hit_ratio, tlb_miss_ratio, pf_ratio, swap_out_ratio;

  fprintf( out, "++++++++++++++++++++ Effective Memory-Access Time ++++++++++++++++++\n" );
  fprintf( out, "Assuming:\n %dns TLB search time and %dns memory access time\n", 
	   TLB_SEARCH_TIME, MEMORY_ACCESS_TIME );
  tlb_miss_ratio = ((total_accesses-pfs) == 0) ? 1.0 : ( (float) memory_accesses / (float) (total_accesses-pfs) );
  tlb_hit_ratio = 1.0 - tlb_miss_ratio;
  fprintf( out, "memory accesses: %d; total memory accesses %d (less page faults)\n", memory_accesses, total_accesses-pfs ); 
  fprintf( out, "TLB hit rate = %f\n", tlb_hit_ratio ); 
  fprintf( out, "Effective memory-access time = %fns\n", 
    ((TLB_SEARCH_TIME+MEMORY_ACCESS_TIME) * tlb_hit_ratio) + 
     (((2 * MEMORY_ACCESS_TIME) + TLB_SEARCH_TIME) * (1 - tlb_hit_ratio)));

  fprintf( out, "++++++++++++++++++++ Effective Access Time ++++++++++++++++++\n" );
  fprintf( out, "Assuming:\n %dms average page-fault service time (w/o swap out), a %dms average swap out time, and %dns memory access time\n", 
	   ( PF_OVERHEAD + SWAP_IN_OVERHEAD + RESTART_OVERHEAD ), SWAP_OUT_OVERHEAD, MEMORY_ACCESS_TIME );
  fprintf( out, "swaps: %d; invalidates: %d; page faults: %d\n", 
	   swaps, invalidates, pfs ); 
  pf_ratio = ( (float)pfs / (float)total_accesses );
  swap_out_ratio = ( (float)swaps / (float)pfs );
  fprintf( out, "Page fault ratio = %f\n", pf_ratio ); 
  fprintf( out, "Effective access time = %fms\n", 
	   ((float) MEMORY_ACCESS_TIME/1000000 * (1-pf_ratio)) + 
     (((float) PF_OVERHEAD + (float) SWAP_IN_OVERHEAD + ((float) swap_out_ratio * SWAP_OUT_OVERHEAD) + (float) RESTART_OVERHEAD ) * (pf_ratio)));

  return 0;
}


/**********************************************************************

    Function    : system_init
    Description : Initialize the system in which we will manage memory
    Inputs      : fp - input file 
                  mech - replacement mechanism
    Outputs     : 0 if successful, <0 otherwise

***********************************************************************/

int system_init( FILE *fp, int mech )
{
  unsigned int i;
  int err;
  char *input = NULL;
  size_t len = 0;
  int nread;
  unsigned int pid, vaddr;
  
  fseek( fp, 0, SEEK_SET );  /* start at beginning */

  /* initialize process table, frame table, and TLB */
  memset( processes, 0, sizeof(task_t) * MAX_PROCESSES );
  memset( physical_mem, 0, sizeof(frame_t) * PHYSICAL_FRAMES );
  tlb_flush( );
  current_pt = 0;

  /* initialize frames with numbers */
  for ( i = 0; i < PHYSICAL_FRAMES ; i++ ) {
    physical_mem[i].frame = i;
    physical_mem[i].lru = FALSE;
    physical_mem[i].valid = INVALID;
    physical_mem[i].page = INVALID;
  }

  /* create processes, including initial page table */
  while (( nread = getline( &input, &len, fp )) >= 0 ) {
      input[nread] = 0;
      if ( sscanf( input, "%d %x", &pid, &vaddr ) == 2 ) {

	if ( processes[pid].pagetable == NULL ) {
	  err = process_create( pid );

	if ( err ) 
	  return -1;
      }
    }
  }

  free( input );
  fseek( fp, 0, SEEK_SET );  /* reset at beginning */
  
  /* init replacement specific data */
  pt_replace_init[mech]( fp );

  return 0;
}


/**********************************************************************

    Function    : process_create
    Description : Initialize process's task structure
    Inputs      : pid - id (and index) of process
    Outputs     : 0 if successful, <0 otherwise

***********************************************************************/

int process_create( unsigned int pid )
{
  assert( pid >= 0 );
  assert( pid < MAX_PROCESSES );

  /* initialize to zero -- particularly for stats */
  memset( &processes[pid], 0, sizeof(task_t) );

  /* set process data */
  processes[pid].pid = pid;
  // page directory
  posix_memalign( (void **)&processes[pid].pagetable, sizeof(pdentry_t) * PAGE_DIR_SIZE,
		  sizeof(pdentry_t) * PAGE_DIR_SIZE );

  if ( processes[pid].pagetable == 0 )
    return -1;

  /* initialize page table */
  memset( processes[pid].pagetable , 0, sizeof(pdentry_t) * PAGE_DIR_SIZE );

  return 0;
}


/**********************************************************************

    Function    : command_loop
    Description : process commands in the input file
    Inputs      : in - input file pointer
                  mech - page replacement mechanism id
    Outputs     : 0 if successful, -1 if failure

***********************************************************************/

int command_loop( FILE *in, unsigned int mech )
{
  unsigned int pf = FALSE;
  unsigned int pid, vaddr, paddr, op;
  char *input = NULL;
  char op_string[LINE_SIZE];
  size_t len = 0;
  int nread;
  int err = 0;

  /* get memory access */
  while (( nread = getline( &input, &len, in )) >= 0 ) {
    input[nread] = 0;
    if ( sscanf( input, "%d %x %s", &pid, &vaddr, op_string ) == 3 ) {

      /* convert op_string to op */
      if (( strncmp( op_string, "read", ((strlen(op_string) < 4)? strlen(op_string) : 4)) == 0 )) {
	op = OP_READ;
      }
      else op = OP_WRITE;

      printf("=== memory_access: pid: %d; vaddr: 0x%x; op: %d\n", pid, vaddr, op);  

      total_accesses++;  // Count each memory request

      /* check if need to context switch */
      if (( !current_pid ) || ( pid != current_pid )) {
	context_switch( pid );
      }

      /* seg fault accessing address in page 0 */
      if ( vaddr < 0x1000 ) {
	segmentation_fault( vaddr, op );
	continue;
      }

      /**** Address Translation ****/
      /* lookup mapping in TLB */
      if (( err = tlb_resolve_addr( vaddr, &paddr, op ))) {  
             if ( err > 0 ) fprintf( out, "th %d 0x%x 0x%x\n", pid, vaddr, paddr );  // TLB hit
      }
      else {  
	/* if miss, lookup in pt */
	if (( err = pt_resolve_addr( vaddr, &paddr, op )) ) {
     if ( err > 0 ) fprintf(out, "ph %d 0x%x 0x%x\n", pid, vaddr, paddr); // PT hit
	  /* collect number of pt accesses */
	  memory_accesses++;  // page table hit count
	}
	else {
	  /* if invalid, handle page fault and update page tables */
	  /* (w/ replacement, if necessary) */
	  pf = TRUE;
	  if ( pt_demand_page( pid, vaddr, &paddr, op, mech ) ) {
	    continue;  // fault - no update
	  }	 
	  fprintf(out, "pd %d 0x%x 0x%x\n", pid, vaddr, paddr);  // demand page
	}
	tlb_update_pageref( vaddr, paddr, op );
      }
      /* emulate hardware update to page - ref/dirty bits */
      ptentry_t *p = PAGE_TO_PTENTRY( vaddr >> 12 );
      hw_update_pageref( p, op );  
      if ( pf ) pt_update_replacement[mech]( pid, p );  // update on PF
      pf = FALSE;
    }
  }

  free( input );
  return 0;
}


/**********************************************************************

    Function    : context_switch
    Description : Switch from one process id to another 
    Inputs      : pid - new process id
    Outputs     : 0 if successful, <0 otherwise

***********************************************************************/

int context_switch( unsigned int pid )
{
  printf("+++ Context switch from process %d to %d - with TLB flush\n", current_pid, pid);
  
  /* flush tlb */
  tlb_flush( );

  /* switch page tables */
  current_pt = processes[pid].pagetable;
  current_pid = pid;

  return 0;
}


/**********************************************************************

    Function    : tlb_flush
    Description : flush TLB entries - reset invalid
    Inputs      : none
    Outputs     : 0 

***********************************************************************/

int tlb_flush( void )
{
  int i;

  for ( i = 0; i < TLB_ENTRIES; i++ ) {
    tlb[i].valid = INVALID;
  }
  
  return 0;
}


/**********************************************************************

    Function    : tlb_resolve_addr
    Description : convert vaddr to paddr if a hit in the tlb
    Inputs      : vaddr - virtual address 
                  paddr - physical address
                  op - 0 for read, 1 for read-write
    Outputs     : 1 if hit, 0 if miss, -1 if error

***********************************************************************/

/* note: normally, the operations associated with a page are based on the address space
   segments in the ELF binary (read-only, read-write, execute-only).  Assume that this is 
   already done */

int tlb_resolve_addr( unsigned int vaddr, unsigned int *paddr, unsigned int op )
{
  /* Task #2(a) */
  int i;
  unsigned int page = ( vaddr / PAGE_SIZE );
  for ( i=0; i<TLB_ENTRIES; i++ ) {
    if ( tlb[i].page == page && tlb[i].valid == TRUE ) {

      if (( tlb[i].page != 0 ) && ( tlb[i].op < op )) {
        protection_fault( vaddr, op );
        return 1;
      }

      vaddr = vaddr << 20 >> 20;
      unsigned int page_offset = tlb[i].frame << 12;
      *paddr = page_offset | vaddr;
      return 1;

    }
  }

  return 0;  /* miss */
}

/**********************************************************************

    Function    : tlb_update_pageref
    Description : associate page and frame in TLB 
    Inputs      : vaddr - virtual address
                  paddr - physical address
                  op - operation - read (0) or write (1)
    Outputs     : return 0

***********************************************************************/

int tlb_update_pageref( unsigned int vaddr, unsigned int paddr, unsigned int op )
{
  unsigned int i;
  unsigned int frame = (paddr >> 12);
  unsigned int page = (vaddr >> 12); 

  /* replace entry for same frame */
  for ( i = 0; i < TLB_ENTRIES; i++ ) {
    if (( tlb[i].valid == VALID ) && ( tlb[i].frame == frame )) {
      fprintf(out, "tr %d %d %d\n", frame, page, tlb[i].page);
      tlb[i].page = page;
      tlb[i].op = op;
      tlb[i].valid = VALID;
      return 0;
    }
  }

  /* or add anywhere in tlb */
  for ( i = 0; i < TLB_ENTRIES; i++ ) {
    if ( tlb[i].valid == INVALID ) {
      fprintf(out, "tu %d %d %d\n", i, frame, page);
      tlb[i].page = page;
      tlb[i].frame = frame;
      tlb[i].op = op;
      tlb[i].valid = VALID;
      return 0;
    }
  }

  /* or pick next entry to evict */
  i = tlb_evict_index++ % TLB_ENTRIES;   // deterministic
  fprintf(out, "te %d %d %d\n", frame, page, tlb[i].page);
  tlb[i].page = page;
  tlb[i].frame = frame;
  tlb[i].op = op;
  tlb[i].valid = VALID;
  return 0;
}


/**********************************************************************
      
    Function    : PAGE_TO_PTENTRY
    Description : Retrieve a page table entry for a page (index)
    Inputs      : page - page number in virtual address space (20 bits of 32-bit addr)
    Outputs     : an allicated page table entry or NULL 
      
***********************************************************************/


ptentry_t *PAGE_TO_PTENTRY( unsigned int page )
{   
  unsigned int vpn = page << 22 >> 22; /* Virtual page number - 10-bit index of page table entry */
  unsigned int dir = page >> 10;       /* Page directory number - 10-bit index of page directory */
  dir = dir & 1023;
  pdentry_t *pdentry;                  /* Page directory entry - corresponds to current process/pt */ 
  pdentry = &current_pt[dir];
  if ( pdentry->pte_page == NULL ) {
    posix_memalign( (void **)&pdentry->pte_page, sizeof(ptentry_t) * PAGE_DIR_SIZE,
		  sizeof(ptentry_t) * PAGE_DIR_SIZE );
    memset( pdentry->pte_page, 0, sizeof(ptentry_t) * PAGE_DIR_SIZE );
  }
    
  return &pdentry->pte_page[vpn];      /* &pdentry->pte_page[vpn]; */
}   


/**********************************************************************

    Function    : pt_resolve_addr
    Description : use the process's page table to determine the address
    Inputs      : vaddr - virtual addr
                  paddr - physical addr
                  valid - valid bit
                  op - read (OP_READ) or read-write (OP_WRITE)
    Outputs     : 1 if hit, 0 if miss, -1 if error

***********************************************************************/

int pt_resolve_addr( unsigned int vaddr, unsigned int *paddr, unsigned int op )
{
  /* Task #2(b) */
  unsigned int page = ( vaddr / PAGE_SIZE );

  ptentry_t *ptentry = PAGE_TO_PTENTRY( page );

  if ( ptentry->valid == 0 ) {
    return 0;
  }
  if (( ptentry->page != 0 ) && ( ptentry->op < op )) {
    protection_fault( vaddr, op );
    return -1;
  }

  vaddr = vaddr << 20;
  vaddr = vaddr >> 20;
  unsigned int page_offset = ptentry->frame << 12;
  *paddr = page_offset + vaddr;

  return 1;
}
  

/**********************************************************************

    Function    : pt_demand_page
    Description : run demand paging, including page replacement
    Inputs      : pid - process pid
                  vaddr - virtual address
                  paddr - physical address of new page
                  op - read (0) or write (1)
                  mech - page replacement mechanism

    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int pt_demand_page( unsigned int pid, unsigned int vaddr, unsigned int *paddr, unsigned int op, unsigned int mech )
{
  int i;
  unsigned int page = ( vaddr / PAGE_SIZE );
  frame_t *f = (frame_t *)NULL;
  unsigned int other_pid;
  ptentry_t *vic_ptentry, *ptentry;

  pfs++;   // Page fault count

  /* get ptentry for this page and allocate ptentry's memory if necessary */
  ptentry = PAGE_TO_PTENTRY( page );

  /* check for protection fault */
  /* if previously assigned a page number, op has been set */
  if (( ptentry->page != 0 ) && ( ptentry->op < op )) {
    protection_fault( vaddr, op );
    return -1;
  }
  
  /* find a free frame */
  for ( i = 0; i < PHYSICAL_FRAMES; i++ ) {
    if ( !physical_mem[i].valid ) { 
      f = &physical_mem[i];

      // store frame number in frame...
      pt_alloc_frame( pid, page, f, op, ptentry, mech );  
      printf("pt_demand_page: free frame -- pid: %d; page: %d; frame num: %d\n", 
	     pid, page, f->frame);
      fprintf(out, "pu %d %d\n", f->frame, page);
      break;
    }
  }

  /* if no free frame, run page replacement */
  if ( f == NULL ) {
    pt_choose_victim[mech]( &other_pid, &f, &vic_ptentry );     // other_pid may be same as current_pid
    fprintf(out, "pe %d %d %d\n", pid, f->frame, vic_ptentry->page);
    pt_invalidate_mapping( other_pid, vic_ptentry );
    pt_alloc_frame( pid, page, f, op, ptentry, mech );  
    fprintf(out, "pu %d %d\n", f->frame, page);
  }
  
  /* compute new physical addr */
  vaddr = vaddr << 20;
  vaddr = vaddr >> 20;
  unsigned int page_offset = ptentry->frame << 12;
  *paddr = page_offset + vaddr; /* Task #1(c) */
  
  return 0;
}

/**********************************************************************

    Function    : pt_invalidate_mapping
    Description : remove mapping between page and frame in pt
    Inputs      : pid - process id (to find page table) 
                  page - number of page in pid's pt
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int pt_invalidate_mapping( unsigned int pid, ptentry_t *ptentry )
{
  assert( processes[pid].pid == pid );    /* a real process */

  if ( ptentry->dirty )
    pt_write_frame( &physical_mem[ptentry->frame] );

  invalidates++;  // Count page mappings invalidated

  ptentry->valid = INVALID;
  return 0;
}


/**********************************************************************

    Function    : pt_write_frame
    Description : write frame to swap
    Inputs      : frame - frame to be swapped
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int pt_write_frame( frame_t *f )
{
  /* collect some stats */
  swaps++;  // count writes to swap space

  return 0;
}


/**********************************************************************

    Function    : pt_alloc_frame
    Description : alloc frame for this virtual page
    Inputs      : pid - process id
                  page - virtual page number
                  frame - frame to use
                  op - operation (read-only = 0; rw = 1)
                  ptentry - page table entry to update
                  mech - replacement mechanism
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int pt_alloc_frame( unsigned int pid, unsigned int page, frame_t *f, unsigned int op,
		    ptentry_t *ptentry,  unsigned int mech )
{
  /* Task #1(b) */
  ptentry->ref = FALSE;
  ptentry->dirty = FALSE;
  ptentry->valid = 1;
  ptentry->op = op;
  ptentry->frame = f->frame;
  ptentry->page = page;

  f->page = page;
  f->valid = 1;
  f->lru = 0;

  return 0;
}


/**********************************************************************

    Function    : kill_process
    Description : stop process execution and free its page table
                  resources and physical resources for others
    Inputs      : pid - process id
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int kill_process( unsigned int pid )
{
  task_t *process = &processes[pid];
  unsigned int freect=0;  /* Page Table Pages Freed */

  /* Task #4 */
  pdentry_t *pdentry;                  /* Page directory entry - corresponds to current process/pt */ 
  pdentry = &current_pt[0];
  int i;
  for ( i = 0; i < PAGE_DIR_SIZE; i++ ) {
    pdentry_t *current_page = current_pt[i].pte_page;
    if ( current_page != NULL ) {
      free( current_page );
      current_page = NULL;
      freect ++;
    }
  }
  fprintf(out, "kp %d %d\n", pid, freect);  
  return 0;
}


/**********************************************************************

    Function    : protection_fault
    Description : report protection fault into log
    Inputs      : vaddr - virtual address
                  op - operation
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int protection_fault( unsigned int vaddr, unsigned int op )
{
  fprintf(out, "pf %d 0x%x\n", current_pid, vaddr);
  kill_process( current_pid );

  return 0;
}


/**********************************************************************

    Function    : segmentation_fault
    Description : report segmentation fault into log
    Inputs      : out - output file descriptor
                  vaddr - virtual address
                  op - operation
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int segmentation_fault( unsigned int vaddr, unsigned int op )
{
  fprintf(out, "sf %d 0x%x\n", current_pid, vaddr);
  kill_process( current_pid );

  return 0;
}


/**********************************************************************

    Function    : hw_update_pageref
    Description : when a memory access occurs, the hardware update the reference 
                  and dirty bits for the appropriate page.  We simulate this by an
                  update when the page is found (in TLB or page table).
    Inputs      : page - page number
                  op - read (0) or write (1)
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int hw_update_pageref( ptentry_t *ptentry, unsigned int op )
{
  ptentry->ref = 1;

  if ( op ) {   /* write */
    ptentry->dirty = 1;
  }

  return 0;
}




/* need a page table entry struct */

#define TRUE             1
#define PAGE_SIZE        0x1000
#define VIRTUAL_PAGES    1024
#define PHYSICAL_FRAMES  16
#define PAGE_DIR_SIZE    1024
#define MAX_PROCESSES    10
#define TLB_ENTRIES      8
#define WRITE_FRAC       15
#define INVALID          0
#define VALID            1 
#define FALSE            0
#define OP_READ          0
#define OP_WRITE         1
#define LINE_SIZE        80

/* bitmasks */
#define VALIDBIT          0x1
#define REFBIT            0x2
#define DIRTYBIT          0x4
#define OPBIT             0x8

/* constants for display */
#define TLB_SEARCH_TIME    20      /* in ns */
#define MEMORY_ACCESS_TIME 100    /* in ns */
#define PF_OVERHEAD        1      /* in ms */
#define SWAP_IN_OVERHEAD   12     /* in ms */
#define SWAP_OUT_OVERHEAD  12     /* in ms */
#define RESTART_OVERHEAD   1      /* in ms */


/* page table entry */
/* 1K per PD entry */
typedef struct ptentry {
#if 0
  int number;
  int frame;
  int bits;  /* ref, dirty */
  int op;
  int ct;
#endif
  unsigned int valid : 1;    // 1 for in-use, 0 for not 
  unsigned int ref : 1;      // 1 for referenced, 0 for not
  unsigned int dirty : 1;    // 1 for written, 0 for not
  unsigned int op : 1;       // 0 for read, 1 for write
  unsigned int frame : 8;    // page frame index space - 2^8 frames max
  unsigned int page : 20;    // page index - 2^20 pages max 
} ptentry_t;

/* page directory entry */
/* one page of these PD entries */
typedef struct pdentry {
  ptentry_t *pte_page;   // a non-zero pte_page implies "valid"
} pdentry_t;


/* physical frame representation */
typedef struct frame {
#if 0
  int number;
  int allocated;
  int page;
  int op;
#endif
  unsigned int valid : 1;    // page frame in use?
  unsigned int frame : 8;    // page frame index space - 2^8 frames max
  unsigned int page : 20;    // page assigned - 2^20 pages max
  unsigned int lru : 3;      // least recently used frame bits
} frame_t;


/* TLB entry */
typedef struct tlbentry {
#if 0
  int page; 
  int frame;
  int op;
#endif
  unsigned int valid : 1;
  unsigned int op : 1;
  unsigned int page : 20;
  unsigned int frame : 10;
} tlb_t;


/* need a process structure */
typedef struct task {
  int pid;                      /* process id */
  pdentry_t *pagetable;         /* process page table */
  int ct;                       /* pte pages ct */
} task_t;


/* need a store for all processes */
task_t processes[MAX_PROCESSES];


extern frame_t physical_mem[PHYSICAL_FRAMES];
extern pdentry_t *current_pt;


/* initialization */
extern int system_init( FILE *fp, int mech );

/* process (task) functions */
extern int process_create( unsigned int pid );
extern int process_frames( unsigned int pid, unsigned int *frames );

/* TLB functions */
extern int tlb_resolve_addr( unsigned int vaddr, unsigned int *paddr, unsigned int op );
extern int tlb_update_pageref( unsigned int vaddr, unsigned int paddr, unsigned int op );
extern int tlb_flush( void );

/* page table functions */
extern ptentry_t *PAGE_TO_PTENTRY( unsigned int page );
extern int pt_resolve_addr( unsigned int vaddr, unsigned int *paddr, unsigned int op );
extern int pt_demand_page( unsigned int pid, unsigned int vaddr, unsigned int *paddr, unsigned int op, unsigned int mech );
extern int pt_write_frame( frame_t *frame );
extern int pt_alloc_frame( unsigned int pid, unsigned int page, frame_t *f, unsigned int op, ptentry_t *ptentry, unsigned int mech );
extern int pt_invalidate_mapping( unsigned int pid, ptentry_t *ptentry );

/* error handling */
extern int protection_fault( unsigned int vaddr, unsigned int op );
extern int segmentation_fault( unsigned int vaddr, unsigned int op );
extern int kill_process( unsigned int pid );

/* external functions */
// extern int get_memory_access( FILE *fp, unsigned int *pid, unsigned int *vaddr, unsigned int *op, int *eof );
extern int command_loop( FILE *in, unsigned int mech );
extern int context_switch( unsigned int pid );
extern int hw_update_pageref( ptentry_t *ptentry, unsigned int op );
extern int write_results(  );


/* fifo - cmpsc473-p3-fifo.c */
extern int init_fifo( FILE *fp );
extern int replace_fifo( unsigned int *victim, frame_t **frame, ptentry_t **ptentry );
extern int update_fifo( unsigned int pid, ptentry_t *ptentry );

/* second - cmpsc473-p3-second.c */
extern int init_second( FILE *fp );
extern int replace_second( unsigned int *victim, frame_t **frame, ptentry_t **ptentry );
extern int update_second( unsigned int pid, ptentry_t *ptenty );

/* enhanced second chance - cmpsc473-p3-enh.c */
extern int init_enh( FILE *fp );
extern int replace_enh( unsigned int *victim, frame_t **frame, ptentry_t **ptentry );
extern int update_enh( unsigned int pid, ptentry_t *ptentry );

/* LRU - cmpsc473-p3-lru.c */
extern int init_lru( FILE *fp );
extern int replace_lru( unsigned int *victim, frame_t **frame, ptentry_t **ptentry );
extern int update_lru( unsigned int pid, ptentry_t *ptentry );

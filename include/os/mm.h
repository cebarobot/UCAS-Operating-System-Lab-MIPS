#ifndef INCLUDE_MM_H_
#define INCLUDE_MM_H_
#include "type.h"
// #include "sched.h"

#define NUM_ENTRY   0x400
#define PGD_START           0xffffffffa1000000
#define PT_START            0xffffffffa1100000
#define KERNEL_PAGE_START   0xffffffffa2000000

// #pragma pack(8)
// typedef struct {
//     uint32_t g    : 1;
//     uint32_t v    : 1;
//     uint32_t d    : 1;
//     uint32_t c    : 3;
//     uint32_t pfn  : 24;
//     uint32_t pfnx : 3;
//     uint32_t      : 29;
//     uint32_t xi   : 1;
//     uint32_t ri   : 1;
// } PTE_t;
// #pragma pack()

typedef uint64_t PTE_t;

typedef struct {
    PTE_t entry[NUM_ENTRY];
} PT_t;     // size: 0x2000

typedef struct {
    PT_t* entry[NUM_ENTRY];
} PGD_t;    // size: 0x2000

extern PGD_t* current_pgd;
extern PT_t * global_empty_pt;

#define ENTRYHI(r, vpn2, asid) (uint64_t) \
    ( (((r) & (uint64_t)0x3) << 62) | (((vpn2) & 0x7FFFFFF) << 13) | (asid & 0xff) )
#define ENTRYLO(pfn, c, d, v, g) (uint64_t) \
    ( (((pfn) & 0xFFFFFF) << 6) | (((c) & 0x7) << 3) | (((d) & 0x1) << 2) | (((v) & 0x1) << 1) |((g) & 0x1) )

/**
 * Initialize global page table base component
 */
void init_page_table();

/**
 * Initialize one global page directory
 */
void init_pgd(PGD_t * pgd);

/**
 * Trigger TLB refill manually (for kernel mode)
 */
void do_TLB_Refill(uint64_t);

/**
 * TLB invalid exception handler
 */
void TLB_Invalid_handler();

void do_page_fault(PTE_t * pte);

void init_TLB(void);
void physical_frame_initial(void);

void set_cp0_entryhi(uint64_t);
uint64_t get_cp0_entryhi();
uint64_t get_cp0_index();
void set_cp0_index(uint64_t);
uint64_t get_cp0_badvaddr();
uint64_t get_cp0_entrylo0();
void set_cp0_entrylo0(uint64_t);
uint64_t get_cp0_entrylo1();
void set_cp0_entrylo1(uint64_t);
void set_cp0_pagemask(uint64_t);
void tlbwr_operation();
void tlbwi_operation();
void tlbp_operation();
void set_cp0_wired(uint64_t);
void set_cp0_context(uint64_t);

/**
 * Allocation of page
 */

#define MAX_FREE_PAGE 124

/** 
 * struct of page allocation control infomation
 */
struct PageCtrl {
    uint64_t start_addr;
    uint64_t next_page_addr;
    uint64_t page_size;
    uint64_t cnt_free_page;
    uint64_t free_pages[MAX_FREE_PAGE];
};

extern struct PageCtrl page_ctrl_kernel_stack;
extern struct PageCtrl page_ctrl_kernel;
extern struct PageCtrl page_ctrl_user;
extern struct PageCtrl page_ctrl_pgd;
extern struct PageCtrl page_ctrl_pt;

/**
 * Initialize page allocation contraller
 * @param page_ctrl pointer to stuct PageAllocCtl
 * @param start_addr start address of memory space
 * @param page_size size of one page
 */
void page_ctrl_init(struct PageCtrl * page_ctrl, uint64_t start_addr, uint64_t page_size);

/**
 * Alloc a page
 * @param page_ctrl pointer to struct PageAllocCtl
 * @return address of page
 */
uint64_t alloc_page(struct PageCtrl * page_ctrl);

/**
 * Free a page
 * @param page_ctrl pointer to struct PageAllocCtl
 * @param addr address of page
 */
void free_page(struct PageCtrl * page_ctrl, uint64_t addr);

extern void * zero_page;

#endif
